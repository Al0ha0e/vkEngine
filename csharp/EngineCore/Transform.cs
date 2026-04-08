namespace vkEngine.EngineCore
{
    public unsafe sealed class Transform
    {
        private readonly UInt32 id;

        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> getLocalPosition;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> setLocalPosition;
        private static delegate* unmanaged[Cdecl]<UInt32, NQuat*, void> getLocalRotation;
        private static delegate* unmanaged[Cdecl]<UInt32, NQuat*, void> setLocalRotation;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> getLocalScale;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> setLocalScale;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> translateLocal;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> translateGlobal;
        private static delegate* unmanaged[Cdecl]<UInt32, float, NVec3*, void> rotateLocal;
        private static delegate* unmanaged[Cdecl]<UInt32, float, NVec3*, void> rotateGlobal;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> scale;

        public Transform(UInt32 id)
        {
            this.id = id;
        }

        internal static void RegisterNativeFunctions(NativeFunctions* functions)
        {
            getLocalPosition = functions->GetTransformLocalPosition;
            setLocalPosition = functions->SetTransformLocalPosition;
            getLocalRotation = functions->GetTransformLocalRotation;
            setLocalRotation = functions->SetTransformLocalRotation;
            getLocalScale = functions->GetTransformLocalScale;
            setLocalScale = functions->SetTransformLocalScale;
            translateLocal = functions->TranslateTransformLocal;
            translateGlobal = functions->TranslateTransformGlobal;
            rotateLocal = functions->RotateTransformLocal;
            rotateGlobal = functions->RotateTransformGlobal;
            scale = functions->ScaleTransform;
        }

        public uint Id
        {
            get { return id; }
        }

        public NVec3 LocalPosition
        {
            get
            {
                EnsureBound();
                NVec3 value = default;
                getLocalPosition(id, &value);
                return value;
            }
            set
            {
                EnsureBound();
                NVec3 nativeValue = value;
                setLocalPosition(id, &nativeValue);
            }
        }

        public NQuat LocalRotation
        {
            get
            {
                EnsureBound();
                NQuat value = default;
                getLocalRotation(id, &value);
                return value;
            }
            set
            {
                EnsureBound();
                NQuat nativeValue = value;
                setLocalRotation(id, &nativeValue);
            }
        }

        public NVec3 LocalScale
        {
            get
            {
                EnsureBound();
                NVec3 value = default;
                getLocalScale(id, &value);
                return value;
            }
            set
            {
                EnsureBound();
                NVec3 nativeValue = value;
                setLocalScale(id, &nativeValue);
            }
        }

        public void TranslateLocal(NVec3 delta)
        {
            EnsureBound();
            NVec3 nativeValue = delta;
            translateLocal(id, &nativeValue);
        }

        public void TranslateGlobal(NVec3 delta)
        {
            EnsureBound();
            NVec3 nativeValue = delta;
            translateGlobal(id, &nativeValue);
        }

        public void RotateLocal(float radians, NVec3 axis)
        {
            EnsureBound();
            NVec3 nativeAxis = axis;
            rotateLocal(id, radians, &nativeAxis);
        }

        public void RotateGlobal(float radians, NVec3 axis)
        {
            EnsureBound();
            NVec3 nativeAxis = axis;
            rotateGlobal(id, radians, &nativeAxis);
        }

        public void Scale(NVec3 scaleFactor)
        {
            EnsureBound();
            NVec3 nativeScale = scaleFactor;
            scale(id, &nativeScale);
        }

        private static void EnsureBound()
        {
            if (!NativeFunctionRegistry.IsRegistered)
            {
                throw new InvalidOperationException("Native transform functions have not been registered.");
            }
        }
    }
}
