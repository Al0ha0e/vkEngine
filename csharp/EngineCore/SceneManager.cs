using System;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text.Json;

namespace vkEngine.EngineCore
{
    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct SceneManagerFunctions
    {
        public delegate* unmanaged<byte**, UInt32, void> Load;
        public delegate* unmanaged<void> Start;
        public delegate* unmanaged<void> Update;
        public delegate* unmanaged<void> FixedUpdate;
        public delegate* unmanaged<void> LateUpdate;
        public delegate* unmanaged<void> Unload;
    }

    [Flags]
    internal enum ScriptLifecycleMask
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
        private static readonly Dictionary<UInt32, List<EntityScript>> startScripts = new();
        private static readonly Dictionary<UInt32, List<EntityScript>> updateScripts = new();
        private static readonly Dictionary<UInt32, List<EntityScript>> fixedUpdateScripts = new();
        private static readonly Dictionary<UInt32, List<EntityScript>> lateUpdateScripts = new();
        private static readonly Dictionary<UInt32, List<EntityScript>> unloadScripts = new();
        private static readonly JsonSerializerOptions jsonOptions = new()
        {
            PropertyNameCaseInsensitive = true
        };
        private static Assembly? gameAssembly;

        private sealed class ScriptLoadState
        {
            public UInt32 Id { get; set; }
            public string ClassName { get; set; } = string.Empty;
            public List<ScriptFieldState> Fields { get; set; } = new();
        }

        private sealed class ScriptFieldState
        {
            public string Name { get; set; } = string.Empty;
            public JsonElement Val { get; set; }
        }

        [UnmanagedCallersOnly]
        public unsafe static void Load(byte** data, UInt32 cnt)
        {
            if (data == null || cnt == 0)
                return;

            var assembly = GetGameAssembly();

            for (UInt32 i = 0; i < cnt; i++)
            {
                var json = Marshal.PtrToStringUTF8((nint)data[i]);
                if (string.IsNullOrWhiteSpace(json))
                    continue;

                var state = JsonSerializer.Deserialize<ScriptLoadState>(json, jsonOptions)
                    ?? throw new InvalidOperationException("Failed to deserialize script load state.");

                if (string.IsNullOrWhiteSpace(state.ClassName))
                    throw new InvalidOperationException("Script className is missing.");

                var scriptType = assembly.GetType(state.ClassName, throwOnError: false)
                    ?? throw new InvalidOperationException($"Script type '{state.ClassName}' was not found in {assembly.GetName().Name}.dll.");

                if (!typeof(EntityScript).IsAssignableFrom(scriptType))
                    throw new InvalidOperationException($"Script type '{state.ClassName}' does not inherit from EntityScript.");

                var script = CreateScriptInstance(scriptType, state.Id);
                ApplyFields(scriptType, script, state.Fields);
                script.Register();
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

        internal static void Register(EntityScript script, ScriptLifecycleMask mask)
        {
            if (script == null || mask == ScriptLifecycleMask.None)
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
            Remove(script, startScripts);
            Remove(script, updateScripts);
            Remove(script, fixedUpdateScripts);
            Remove(script, lateUpdateScripts);
            Remove(script, unloadScripts);
        }

        private static void Add(EntityScript script, Dictionary<UInt32, List<EntityScript>> map)
        {
            if (!map.TryGetValue(script.EntityId, out var scripts))
            {
                scripts = new List<EntityScript>();
                map.Add(script.EntityId, scripts);
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
            if (!map.TryGetValue(script.EntityId, out var scripts))
                return;

            for (int i = 0; i < scripts.Count; i++)
            {
                if (!ReferenceEquals(scripts[i], script))
                    continue;

                scripts.RemoveAt(i);
                if (scripts.Count == 0)
                    map.Remove(script.EntityId);
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

        private static Assembly GetGameAssembly()
        {
            if (gameAssembly != null)
                return gameAssembly;

            gameAssembly = Array.Find(AppDomain.CurrentDomain.GetAssemblies(), assembly => assembly.GetName().Name == GameAssemblyName)
                ?? Assembly.Load(GameAssemblyName);
            return gameAssembly;
        }

        private static EntityScript CreateScriptInstance(Type scriptType, UInt32 entityId)
        {
            var ctor = scriptType.GetConstructor(
                BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic,
                binder: null,
                types: new[] { typeof(UInt32) },
                modifiers: null)
                ?? throw new InvalidOperationException($"Script type '{scriptType.FullName}' does not have a constructor with a single UInt32 entity id parameter.");

            return (EntityScript)ctor.Invoke(new object[] { entityId });
        }

        private static void ApplyFields(Type scriptType, EntityScript script, List<ScriptFieldState> fields)
        {
            foreach (var fieldState in fields)
            {
                if (string.IsNullOrWhiteSpace(fieldState.Name))
                    continue;
                var field = scriptType.GetField(fieldState.Name, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic)
                    ?? throw new InvalidOperationException($"Field '{fieldState.Name}' was not found on script type '{scriptType.FullName}'.");

                var value = JsonSerializer.Deserialize(fieldState.Val.GetRawText(), field.FieldType, jsonOptions);
                field.SetValue(script, value);
            }
        }
    }
}
