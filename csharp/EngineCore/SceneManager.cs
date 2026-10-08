using System;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;

namespace vkEngine.EngineCore
{
    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct SceneManagerFunctions
    {
        public delegate* unmanaged<ScriptLoadData*, UInt32, void> Load;
        public delegate* unmanaged<UInt32*, UInt32, void> Start;
        public delegate* unmanaged<void> Update;
        public delegate* unmanaged<void> FixedUpdate;
        public delegate* unmanaged<void> LateUpdate;
        public delegate* unmanaged<void> Unload;
        public delegate* unmanaged<UInt32*, UInt32, void> UnloadEntities;
        public delegate* unmanaged<UInt32, Int32, void> UnregisterComponentCallbacks;
        public delegate* unmanaged<void> Reset;
        public delegate* unmanaged<UInt32, void*, delegate* unmanaged<void*, byte*, byte*, Int32, void>, Int32> FillData;
        public delegate* unmanaged<void> StartAll;
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct ScriptLoadData
    {
        public UInt32 Entity;
        public byte* ClassName;
        public byte* Data;
        public Int32 DataSize;
    }

    [Flags]
    public enum ScriptLifecycleMask
    {
        None = 0,
        Start = 1 << 0,
        Update = 1 << 1,
        FixedUpdate = 1 << 2,
        LateUpdate = 1 << 3,
        Unload = 1 << 4
    }

    public static class SceneManager
    {
        private const string GameAssemblyName = "Game";
        private static readonly Dictionary<UInt32, HashSet<EntityScript>> scriptsByEntity = new();
        private static readonly Dictionary<UInt32, List<EntityScript>> updateScripts = new();
        private static readonly Dictionary<UInt32, List<EntityScript>> fixedUpdateScripts = new();
        private static readonly Dictionary<UInt32, List<EntityScript>> lateUpdateScripts = new();
        private static Assembly? gameAssembly;
        private unsafe delegate EntityScript BinaryParser(
            string className, UInt32 entity, byte* data, Int32 dataSize);
        private static BinaryParser? binaryParser;
        private static Func<EntityScript, byte[]>? binaryWriter;

        internal static EntityScriptData FillData(EntityScript script)
        {
            binaryWriter ??= GetGameAssembly().GetType("vkEngine.Generated.EntityScriptBinaryReaders", true)!
                .GetMethod("FillData", BindingFlags.Public | BindingFlags.Static)!
                .CreateDelegate<Func<EntityScript, byte[]>>();
            return new EntityScriptData(script.GetType().FullName!, binaryWriter(script));
        }

        // Buffers are borrowed only for the synchronous callback. No pinned memory escapes.
        [UnmanagedCallersOnly]
        public static unsafe Int32 ExportEntity(UInt32 entity, void* context,
            delegate* unmanaged<void*, byte*, byte*, Int32, void> receive)
        {
            if (receive == null) return 0;
            try
            {
                if (!scriptsByEntity.TryGetValue(entity, out var scripts)) return 1;
                var snapshots = new List<EntityScriptData>();
                foreach (var script in new List<EntityScript>(scripts))
                    if (script.State is not (ScriptState.Unloading or ScriptState.Unloaded) && IsRegistered(script))
                        snapshots.Add(script.FillData());
                foreach (var snapshot in snapshots)
                {
                    byte[] name = Encoding.UTF8.GetBytes(snapshot.ClassName + "\0");
                    fixed (byte* className = name)
                    fixed (byte* data = snapshot.Data)
                        receive(context, className, data, snapshot.Data.Length);
                }
                return 1;
            }
            catch (Exception error)
            {
                Console.Error.WriteLine($"Entity {entity} export failed: {error}");
                return 0;
            }
        }

        public static unsafe void DestroyEntity(UInt32 entity)
        {
            if (NativeFunctionRegistry.IsRegistered)
                NativeFunctionRegistry.Functions.DestroyEntity(entity);
        }

        public static unsafe bool InstantiatePrefab(
            UInt64 prefab, NVec3 position, NQuat rotation, NVec3 scale, UInt32 parent = UInt32.MaxValue)
        {
            return NativeFunctionRegistry.IsRegistered &&
                NativeFunctionRegistry.Functions.InstantiatePrefab(prefab, &position, &rotation, &scale, parent) != 0;
        }

        public static unsafe bool IsPendingDestroy(UInt32 entity) =>
            NativeFunctionRegistry.IsRegistered &&
            NativeFunctionRegistry.Functions.IsEntityPendingDestroy(entity) != 0;

        [UnmanagedCallersOnly]
        public unsafe static void Load(ScriptLoadData* data, UInt32 cnt)
        {
            if (data == null || cnt == 0)
                return;

            for (UInt32 i = 0; i < cnt; i++)
            {
                ref ScriptLoadData state = ref data[i];
                string? className = Marshal.PtrToStringUTF8((nint)state.ClassName);
                if (string.IsNullOrWhiteSpace(className))
                    throw new InvalidOperationException("Script className is missing.");
                if (state.DataSize < 0 || (state.Data == null && state.DataSize != 0))
                    throw new InvalidOperationException("Invalid script data buffer.");
                // Parse synchronously while the native buffer is valid; no pointer is retained.
                EntityScript script = GetBinaryParser()(
                    className, state.Entity, state.Data, state.DataSize);
                Register(script);
            }
        }

        [UnmanagedCallersOnly]
        public static void StartAll()
        {
            var pending = new List<EntityScript>();
            foreach (var scripts in scriptsByEntity.Values)
                pending.AddRange(scripts);
            StartScripts(pending);
        }

        [UnmanagedCallersOnly]
        public unsafe static void Start(UInt32* entities, UInt32 cnt)
        {
            if (entities == null || cnt == 0)
                return;

            var pending = new List<EntityScript>();
            for (UInt32 i = 0; i < cnt; ++i)
                if (scriptsByEntity.TryGetValue(entities[i], out var scripts))
                    pending.AddRange(scripts);
            StartScripts(pending);
        }

        // Snapshot before callbacks so lifecycle dispatch does not enumerate live collections.
        private static void StartScripts(List<EntityScript> pending)
        {
            foreach (var script in pending)
                if (IsRegistered(script) && script.TryBeginStart())
                {
                    if (script.LifecycleMask.HasFlag(ScriptLifecycleMask.Start))
                        Invoke(script, s => s.Start());
                    // Preserve the existing policy: a failed Start still permits updates.
                    script.MarkStarted();
                }
        }

        [UnmanagedCallersOnly]
        public static void Update()
        {
            Dispatch(updateScripts, script => script.Update());
        }

        [UnmanagedCallersOnly]
        public static void FixedUpdate()
        {
            Physics.DispatchContactEvents();
            Dispatch(fixedUpdateScripts, script => script.FixedUpdate());
        }

        [UnmanagedCallersOnly]
        public static void LateUpdate()
        {
            Dispatch(lateUpdateScripts, script => script.LateUpdate());
        }

        [UnmanagedCallersOnly]
        public static void Unload()
        {
            foreach (var entity in new List<UInt32>(scriptsByEntity.Keys))
                UnloadEntityCore(entity);
            RigidBody.ClearRegistered();
            Sensor.ClearRegistered();
            ClearScriptCollections();
            Console.WriteLine("SceneManager.Unload");
        }

        // Bulk reset at a frame boundary: do not execute user Unload hooks.
        [UnmanagedCallersOnly]
        public static void Reset()
        {
            ClearScriptCollections();
            RigidBody.ClearRegistered();
            Sensor.ClearRegistered();
        }

        [UnmanagedCallersOnly]
        public unsafe static void UnloadEntities(UInt32* entities, UInt32 cnt)
        {
            if (entities == null || cnt == 0)
                return;

            for (UInt32 i = 0; i < cnt; i++)
                UnloadEntityCore(entities[i]);
        }

        [UnmanagedCallersOnly]
        public static void UnregisterComponentCallbacks(UInt32 entity, Int32 componentType)
        {
            if ((ComponentType)componentType == ComponentType.RigidBody)
                RigidBody.UnregisterEntity(entity);
            else if ((ComponentType)componentType == ComponentType.Sensor)
                Sensor.UnregisterEntity(entity);
        }

        private static void UnloadEntityCore(UInt32 entity)
        {
            if (scriptsByEntity.TryGetValue(entity, out var registered))
            {
                var scripts = new List<EntityScript>(registered);
                foreach (var script in scripts)
                {
                    UnloadScript(script);
                }

                scriptsByEntity.Remove(entity);
                updateScripts.Remove(entity);
                fixedUpdateScripts.Remove(entity);
                lateUpdateScripts.Remove(entity);
            }
            RigidBody.UnregisterEntity(entity);
            Sensor.UnregisterEntity(entity);
        }

        public static unsafe SceneManagerFunctions GetFunctions()
        {
            return new SceneManagerFunctions
            {
                Load = &Load,
                Start = &Start,
                Update = &Update,
                FixedUpdate = &FixedUpdate,
                LateUpdate = &LateUpdate,
                Unload = &Unload,
                UnloadEntities = &UnloadEntities,
                UnregisterComponentCallbacks = &UnregisterComponentCallbacks,
                Reset = &Reset,
                FillData = &ExportEntity,
                StartAll = &StartAll
            };
        }

        internal static void Register(EntityScript script)
        {
            if (script == null || script.State is ScriptState.Unloading or ScriptState.Unloaded)
                return;

            if (!scriptsByEntity.TryGetValue(script.Entity, out var scripts))
            {
                scripts = new HashSet<EntityScript>(ReferenceEqualityComparer.Instance);
                scriptsByEntity.Add(script.Entity, scripts);
            }

            if (!scripts.Add(script))
                return;

            ScriptLifecycleMask mask = script.LifecycleMask;
            if (mask == ScriptLifecycleMask.None)
                return;

            if (mask.HasFlag(ScriptLifecycleMask.Update))
                Add(script, updateScripts);
            if (mask.HasFlag(ScriptLifecycleMask.FixedUpdate))
                Add(script, fixedUpdateScripts);
            if (mask.HasFlag(ScriptLifecycleMask.LateUpdate))
                Add(script, lateUpdateScripts);
        }

        private static void Add(EntityScript script, Dictionary<UInt32, List<EntityScript>> map)
        {
            if (!map.TryGetValue(script.Entity, out var scripts))
            {
                scripts = new List<EntityScript>();
                map.Add(script.Entity, scripts);
            }

            scripts.Add(script);
        }

        private static void Dispatch(Dictionary<UInt32, List<EntityScript>> map, Action<EntityScript> callback)
        {
            foreach (var script in CollectScripts(map))
            {
                if (CanDispatch(script))
                    Invoke(script, callback);
            }
        }

        private static void UnloadScript(EntityScript script)
        {
            if (IsRegistered(script) && script.TryBeginUnload())
            {
                if (script.LifecycleMask.HasFlag(ScriptLifecycleMask.Unload))
                    Invoke(script, s => s.Unload());
                script.MarkUnloaded();
            }
        }

        private static void Invoke(EntityScript script, Action<EntityScript> callback)
        {
            try { callback(script); }
            catch (Exception error) { Console.Error.WriteLine($"Entity {script.Entity} lifecycle callback failed: {error}"); }
        }

        private static bool CanDispatch(EntityScript script) => script.State == ScriptState.Started && IsRegistered(script);

        private static bool IsRegistered(EntityScript script) =>
            scriptsByEntity.TryGetValue(script.Entity, out var scripts) && scripts.Contains(script);

        private static List<EntityScript> CollectScripts(Dictionary<UInt32, List<EntityScript>> map)
        {
            var scripts = new List<EntityScript>();

            foreach (var entry in map.Values)
            {
                scripts.AddRange(entry);
            }

            return scripts;
        }

        private static void ClearScriptCollections()
        {
            foreach (var scripts in scriptsByEntity.Values)
                foreach (var script in scripts)
                    script.MarkUnloaded();
            scriptsByEntity.Clear();
            updateScripts.Clear();
            fixedUpdateScripts.Clear();
            lateUpdateScripts.Clear();
        }

        private static Assembly GetGameAssembly()
        {
            if (gameAssembly != null)
                return gameAssembly;

            gameAssembly = Array.Find(AppDomain.CurrentDomain.GetAssemblies(), assembly => assembly.GetName().Name == GameAssemblyName)
                ?? Assembly.Load(GameAssemblyName);
            return gameAssembly;
        }

        private static BinaryParser GetBinaryParser()
        {
            if (binaryParser != null)
                return binaryParser;
            Assembly assembly = GetGameAssembly();
            Type readerType = assembly.GetType(
                "vkEngine.Generated.EntityScriptBinaryReaders", throwOnError: false)
                ?? throw new InvalidOperationException(
                    $"Generated EntityScript binary readers were not found in {assembly.GetName().Name}.dll.");
            MethodInfo parseMethod = readerType.GetMethod(
                "Parse", BindingFlags.Public | BindingFlags.Static)
                ?? throw new InvalidOperationException(
                    "Generated EntityScript binary reader Parse method was not found.");
            binaryParser = parseMethod.CreateDelegate<BinaryParser>();
            return binaryParser;
        }
    }
}
