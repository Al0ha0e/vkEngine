using System;
using System.Collections.Generic;
using System.Reflection;

namespace vkEngine.EngineCore
{
    public enum ComponentType
    {
        Transform = 0,
        Camera = 1,
        RenderableObject = 2,
        SkeletonAnimator = 3,
        RigidBody = 4,
        Sensor = 5,
        CharacterController = 6,
        DirectionalLight = 7,
        PointLight = 8,
        SpotLight = 9,
        Script = 10
    }

    public interface IScriptLifecycle
    {
        void Start();

        void Update();

        void FixedUpdate();

        void LateUpdate();

        void Unload();
    }

    public abstract unsafe class EntityScript : IDisposable, IScriptLifecycle
    {
        private readonly ScriptLifecycleMask lifecycleMask;
        public ScriptLifecycleMask LifecycleMask { get { return lifecycleMask; } }
        private bool disposed;
        private static unsafe delegate* unmanaged[Cdecl]<UInt32, Int32, Int32> hasComponent;

        private static readonly Dictionary<Type, ScriptLifecycleMask> overrideMaskCache = new();
        private static readonly Dictionary<Type, ComponentType> componentTypeMap = new()
        {
            [typeof(Transform)] = ComponentType.Transform,
            [typeof(RigidBody)] = ComponentType.RigidBody,
            [typeof(Sensor)] = ComponentType.Sensor,
            [typeof(CharacterController)] = ComponentType.CharacterController
        };
        private static readonly Dictionary<ComponentType, Type> componentRuntimeTypeMap = new()
        {
            [ComponentType.Transform] = typeof(Transform),
            [ComponentType.RigidBody] = typeof(RigidBody),
            [ComponentType.Sensor] = typeof(Sensor),
            [ComponentType.CharacterController] = typeof(CharacterController)
        };

        protected EntityScript(UInt32 entity)
        {
            Entity = entity;
            lifecycleMask = GetOrAddLifecycleMask(GetType());
        }

        public UInt32 Entity { get; }

        internal static unsafe void RegisterNativeFunctions(NativeFunctions* functions)
        {
            hasComponent = functions->HasComponent;
        }

        public unsafe bool HasComponent(ComponentType componentType)
        {
            return hasComponent(Entity, (Int32)componentType) != 0;
        }

        public bool HasComponent<T>()
        {
            if (!componentTypeMap.TryGetValue(typeof(T), out ComponentType componentType))
                throw new NotSupportedException($"Component type '{typeof(T).FullName}' is not registered for native queries.");

            return HasComponent(componentType);
        }

        public T? GetComponent<T>() where T : class
        {
            return (T?)GetComponent(typeof(T));
        }

        public object? GetComponent(Type componentType)
        {
            if (componentType == null)
                throw new ArgumentNullException(nameof(componentType));

            if (!componentTypeMap.TryGetValue(componentType, out ComponentType nativeComponentType))
                throw new NotSupportedException($"Component type '{componentType.FullName}' is not registered for native queries.");

            if (!HasComponent(nativeComponentType))
                return null;

            return CreateComponent(componentType);
        }

        public object? GetComponent(ComponentType componentType)
        {
            if (!componentRuntimeTypeMap.TryGetValue(componentType, out Type? runtimeType))
                throw new NotSupportedException($"Component type '{componentType}' is not registered for managed access.");

            if (!HasComponent(componentType))
                return null;

            return CreateComponent(runtimeType);
        }

        public virtual void Start() { }

        public virtual void Update() { }

        public virtual void FixedUpdate() { }

        public virtual void LateUpdate() { }

        public virtual void Unload() { }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (disposed)
                return;

            if (lifecycleMask != ScriptLifecycleMask.None)
            {
                SceneManager.Unregister(this);
            }

            disposed = true;
        }

        ~EntityScript()
        {
            Dispose(false);
        }

        private static ScriptLifecycleMask GetOrAddLifecycleMask(Type type)
        {
            if (overrideMaskCache.TryGetValue(type, out var mask))
            {
                return mask;
            }

            mask = ScriptLifecycleMask.None;
            if (IsMethodOverridden(type, nameof(Start)))
                mask |= ScriptLifecycleMask.Start;
            if (IsMethodOverridden(type, nameof(Update)))
                mask |= ScriptLifecycleMask.Update;
            if (IsMethodOverridden(type, nameof(FixedUpdate)))
                mask |= ScriptLifecycleMask.FixedUpdate;
            if (IsMethodOverridden(type, nameof(LateUpdate)))
                mask |= ScriptLifecycleMask.LateUpdate;
            if (IsMethodOverridden(type, nameof(Unload)))
                mask |= ScriptLifecycleMask.Unload;

            overrideMaskCache[type] = mask;
            return mask;
        }

        private static bool IsMethodOverridden(Type type, string methodName)
        {
            var method = type.GetMethod(methodName, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);
            if (method == null)
                return false;
            return method.GetBaseDefinition().DeclaringType != method.DeclaringType;
        }

        private object CreateComponent(Type componentType)
        {
            ConstructorInfo? ctor = componentType.GetConstructor(new[] { typeof(UInt32) });
            if (ctor == null)
                throw new NotSupportedException($"Component type '{componentType.FullName}' must define a public constructor with a UInt32 entity parameter.");

            return ctor.Invoke(new object[] { Entity });
        }
    }
}
