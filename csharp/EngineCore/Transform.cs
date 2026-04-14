namespace vkEngine.EngineCore
{
    public unsafe sealed class Transform
    {
        private readonly UInt32 entity;

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

        public Transform(UInt32 entity)
        {
            this.entity = entity;
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

        public uint Entity
        {
            get { return entity; }
        }

        public NVec3 LocalPosition
        {
            get
            {
                NVec3 value = default;
                getLocalPosition(entity, &value);
                return value;
            }
            set
            {
                NVec3 nativeValue = value;
                setLocalPosition(entity, &nativeValue);
            }
        }

        public NQuat LocalRotation
        {
            get
            {
                NQuat value = default;
                getLocalRotation(entity, &value);
                return value;
            }
            set
            {
                NQuat nativeValue = value;
                setLocalRotation(entity, &nativeValue);
            }
        }

        public NVec3 LocalScale
        {
            get
            {
                NVec3 value = default;
                getLocalScale(entity, &value);
                return value;
            }
            set
            {
                NVec3 nativeValue = value;
                setLocalScale(entity, &nativeValue);
            }
        }

        public void TranslateLocal(NVec3 delta)
        {
            NVec3 nativeValue = delta;
            translateLocal(entity, &nativeValue);
        }

        public void TranslateGlobal(NVec3 delta)
        {
            NVec3 nativeValue = delta;
            translateGlobal(entity, &nativeValue);
        }

        public void RotateLocal(float radians, NVec3 axis)
        {
            NVec3 nativeAxis = axis;
            rotateLocal(entity, radians, &nativeAxis);
        }

        public void RotateGlobal(float radians, NVec3 axis)
        {
            NVec3 nativeAxis = axis;
            rotateGlobal(entity, radians, &nativeAxis);
        }

        public void Scale(NVec3 scaleFactor)
        {
            NVec3 nativeScale = scaleFactor;
            scale(entity, &nativeScale);
        }
    }
}
