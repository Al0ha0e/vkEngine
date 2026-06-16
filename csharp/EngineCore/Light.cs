using System;

namespace vkEngine.EngineCore
{
    public unsafe abstract class Light
    {
        private readonly UInt32 entity;

        protected Light(UInt32 entity)
        {
            this.entity = entity;
        }

        public UInt32 Entity => entity;

        internal static void RegisterNativeFunctions(NativeFunctions* functions)
        {
            DirectionalLight.RegisterDirectionalLightNativeFunctions(functions);
            PointLight.RegisterPointLightNativeFunctions(functions);
            SpotLight.RegisterSpotLightNativeFunctions(functions);
        }
    }

    public unsafe sealed class DirectionalLight : Light
    {
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> getColor;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> setColor;
        private static delegate* unmanaged[Cdecl]<UInt32, float> getIntensity;
        private static delegate* unmanaged[Cdecl]<UInt32, float, void> setIntensity;

        public DirectionalLight(UInt32 entity) : base(entity) { }

        internal static void RegisterDirectionalLightNativeFunctions(NativeFunctions* functions)
        {
            getColor = functions->GetDirectionalLightColor;
            setColor = functions->SetDirectionalLightColor;
            getIntensity = functions->GetDirectionalLightIntensity;
            setIntensity = functions->SetDirectionalLightIntensity;
        }

        public NVec3 Color
        {
            get
            {
                NVec3 value = default;
                getColor(Entity, &value);
                return value;
            }
            set
            {
                NVec3 nativeValue = value;
                setColor(Entity, &nativeValue);
            }
        }

        public float Intensity
        {
            get => getIntensity(Entity);
            set => setIntensity(Entity, value);
        }
    }

    public unsafe sealed class PointLight : Light
    {
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> getColor;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> setColor;
        private static delegate* unmanaged[Cdecl]<UInt32, float> getIntensity;
        private static delegate* unmanaged[Cdecl]<UInt32, float, void> setIntensity;
        private static delegate* unmanaged[Cdecl]<UInt32, float> getRadius;
        private static delegate* unmanaged[Cdecl]<UInt32, float, void> setRadius;

        public PointLight(UInt32 entity) : base(entity) { }

        internal static void RegisterPointLightNativeFunctions(NativeFunctions* functions)
        {
            getColor = functions->GetPointLightColor;
            setColor = functions->SetPointLightColor;
            getIntensity = functions->GetPointLightIntensity;
            setIntensity = functions->SetPointLightIntensity;
            getRadius = functions->GetPointLightRadius;
            setRadius = functions->SetPointLightRadius;
        }

        public NVec3 Color
        {
            get
            {
                NVec3 value = default;
                getColor(Entity, &value);
                return value;
            }
            set
            {
                NVec3 nativeValue = value;
                setColor(Entity, &nativeValue);
            }
        }

        public float Intensity
        {
            get => getIntensity(Entity);
            set => setIntensity(Entity, value);
        }

        public float Radius
        {
            get => getRadius(Entity);
            set => setRadius(Entity, value);
        }
    }

    public unsafe sealed class SpotLight : Light
    {
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> getColor;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> setColor;
        private static delegate* unmanaged[Cdecl]<UInt32, float> getIntensity;
        private static delegate* unmanaged[Cdecl]<UInt32, float, void> setIntensity;
        private static delegate* unmanaged[Cdecl]<UInt32, float> getRadius;
        private static delegate* unmanaged[Cdecl]<UInt32, float, void> setRadius;
        private static delegate* unmanaged[Cdecl]<UInt32, float> getInnerCone;
        private static delegate* unmanaged[Cdecl]<UInt32, float, void> setInnerCone;
        private static delegate* unmanaged[Cdecl]<UInt32, float> getOuterCone;
        private static delegate* unmanaged[Cdecl]<UInt32, float, void> setOuterCone;

        public SpotLight(UInt32 entity) : base(entity) { }

        internal static void RegisterSpotLightNativeFunctions(NativeFunctions* functions)
        {
            getColor = functions->GetSpotLightColor;
            setColor = functions->SetSpotLightColor;
            getIntensity = functions->GetSpotLightIntensity;
            setIntensity = functions->SetSpotLightIntensity;
            getRadius = functions->GetSpotLightRadius;
            setRadius = functions->SetSpotLightRadius;
            getInnerCone = functions->GetSpotLightInnerCone;
            setInnerCone = functions->SetSpotLightInnerCone;
            getOuterCone = functions->GetSpotLightOuterCone;
            setOuterCone = functions->SetSpotLightOuterCone;
        }

        public NVec3 Color
        {
            get
            {
                NVec3 value = default;
                getColor(Entity, &value);
                return value;
            }
            set
            {
                NVec3 nativeValue = value;
                setColor(Entity, &nativeValue);
            }
        }

        public float Intensity
        {
            get => getIntensity(Entity);
            set => setIntensity(Entity, value);
        }

        public float Radius
        {
            get => getRadius(Entity);
            set => setRadius(Entity, value);
        }

        public float InnerCone
        {
            get => getInnerCone(Entity);
            set => setInnerCone(Entity, value);
        }

        public float OuterCone
        {
            get => getOuterCone(Entity);
            set => setOuterCone(Entity, value);
        }
    }
}
