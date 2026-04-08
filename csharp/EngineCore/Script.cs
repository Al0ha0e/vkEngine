using System;
using System.Collections.Generic;
using System.Reflection;

namespace vkEngine.EngineCore
{
    public interface IScriptLifecycle
    {
        void Start();

        void Update();

        void FixedUpdate();

        void LateUpdate();

        void Unload();
    }

    public abstract class EntityScript : IDisposable, IScriptLifecycle
    {
        private readonly ScriptLifecycleMask lifecycleMask;
        private bool disposed;

        private static readonly Dictionary<Type, ScriptLifecycleMask> overrideMaskCache = new();

        protected EntityScript(UInt32 entityId)
        {
            EntityId = entityId;
            lifecycleMask = GetOrAddLifecycleMask(GetType());
        }

        public UInt32 EntityId { get; }

        public void Register()
        {
            if (disposed || lifecycleMask == ScriptLifecycleMask.None)
                return;
            SceneManager.Register(this, lifecycleMask);
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
    }
}
