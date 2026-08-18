using System;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;

namespace vkEngine.EngineCore
{
    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct SceneManagerFunctions
    {
        public delegate* unmanaged<ScriptLoadData*, UInt32, void> Load;
        public delegate* unmanaged<void> Start;
        public delegate* unmanaged<void> Update;
        public delegate* unmanaged<void> FixedUpdate;
        public delegate* unmanaged<void> LateUpdate;
        public delegate* unmanaged<void> Unload;
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
        private static readonly HashSet<EntityScript> allScripts = new();
        private static readonly Dictionary<UInt32, List<EntityScript>> startScripts = new();
        private static readonly Dictionary<UInt32, List<EntityScript>> updateScripts = new();
        private static readonly Dictionary<UInt32, List<EntityScript>> fixedUpdateScripts = new();
        private static readonly Dictionary<UInt32, List<EntityScript>> lateUpdateScripts = new();
        private static readonly Dictionary<UInt32, List<EntityScript>> unloadScripts = new();
        private static Assembly? gameAssembly;
        private unsafe delegate EntityScript BinaryParser(
            string className, UInt32 entity, byte* data, Int32 dataSize);
        private static BinaryParser? binaryParser;

        [UnmanagedCallersOnly]
        public unsafe static void Load(ScriptLoadData* data, UInt32 cnt)
        {
            if (data == null || cnt == 0)
                return;

            BinaryParser parser = GetBinaryParser();

            for (UInt32 i = 0; i < cnt; i++)
            {
                ref ScriptLoadData state = ref data[i];
                string? className = Marshal.PtrToStringUTF8((nint)state.ClassName);
                if (string.IsNullOrWhiteSpace(className))
                    throw new InvalidOperationException("Script className is missing.");
                EntityScript script = parser(
                    className, state.Entity, state.Data, state.DataSize);
                Register(script);
            }
        }

        [UnmanagedCallersOnly]
        public static void Start()
        {
            Dispatch(startScripts, script => script.Start());
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
            Dispatch(unloadScripts, script => script.Unload());
            var scripts = CollectAllScripts();
            foreach (var script in scripts)
                script.Dispose();
            RigidBody.ClearRegistered();
            Sensor.ClearRegistered();
            ClearScriptCollections();
            Console.WriteLine("SceneManager.Unload");
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
                Unload = &Unload
            };
        }

        internal static void Register(EntityScript script)
        {
            if (script == null || !allScripts.Add(script))
                return;

            ScriptLifecycleMask mask = script.LifecycleMask;
            if (mask == ScriptLifecycleMask.None)
                return;

            if (mask.HasFlag(ScriptLifecycleMask.Start))
                Add(script, startScripts);
            if (mask.HasFlag(ScriptLifecycleMask.Update))
                Add(script, updateScripts);
            if (mask.HasFlag(ScriptLifecycleMask.FixedUpdate))
                Add(script, fixedUpdateScripts);
            if (mask.HasFlag(ScriptLifecycleMask.LateUpdate))
                Add(script, lateUpdateScripts);
            if (mask.HasFlag(ScriptLifecycleMask.Unload))
                Add(script, unloadScripts);
        }

        internal static void Unregister(EntityScript script)
        {
            if (script == null || !allScripts.Remove(script))
                return;
            Remove(script, startScripts);
            Remove(script, updateScripts);
            Remove(script, fixedUpdateScripts);
            Remove(script, lateUpdateScripts);
            Remove(script, unloadScripts);
        }

        private static void Add(EntityScript script, Dictionary<UInt32, List<EntityScript>> map)
        {
            if (!map.TryGetValue(script.Entity, out var scripts))
            {
                scripts = new List<EntityScript>();
                map.Add(script.Entity, scripts);
            }

            foreach (var existing in scripts)
            {
                if (ReferenceEquals(existing, script))
                    return;
            }

            scripts.Add(script);
        }

        private static void Remove(EntityScript script, Dictionary<UInt32, List<EntityScript>> map)
        {
            if (!map.TryGetValue(script.Entity, out var scripts))
                return;

            for (int i = 0; i < scripts.Count; i++)
            {
                if (!ReferenceEquals(scripts[i], script))
                    continue;

                scripts.RemoveAt(i);
                if (scripts.Count == 0)
                    map.Remove(script.Entity);
                return;
            }
        }

        private static void Dispatch(Dictionary<UInt32, List<EntityScript>> map, Action<EntityScript> callback)
        {
            foreach (var script in CollectScripts(map))
            {
                callback(script);
            }
        }

        private static List<EntityScript> CollectScripts(Dictionary<UInt32, List<EntityScript>> map)
        {
            var scripts = new List<EntityScript>();

            foreach (var entry in map.Values)
            {
                scripts.AddRange(entry);
            }

            return scripts;
        }

        private static List<EntityScript> CollectAllScripts()
        {
            return new List<EntityScript>(allScripts);
        }

        private static void ClearScriptCollections()
        {
            allScripts.Clear();
            startScripts.Clear();
            updateScripts.Clear();
            fixedUpdateScripts.Clear();
            lateUpdateScripts.Clear();
            unloadScripts.Clear();
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
