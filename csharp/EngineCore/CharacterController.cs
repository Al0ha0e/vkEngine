namespace vkEngine.EngineCore
{
    public unsafe sealed class CharacterController
    {
        private readonly UInt32 entity;

        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> setVelocity;
        private static delegate* unmanaged[Cdecl]<UInt32, NVec3*, void> getVelocity;
        private static delegate* unmanaged[Cdecl]<UInt32, Int32> isGrounded;

        public CharacterController(UInt32 entity)
        {
            this.entity = entity;
        }

        internal static void RegisterNativeFunctions(NativeFunctions* functions)
        {
            setVelocity = functions->SetCharacterControllerVelocity;
            getVelocity = functions->GetCharacterControllerVelocity;
            isGrounded = functions->IsCharacterControllerGrounded;
        }

        public UInt32 Entity => entity;

        public NVec3 Velocity
        {
            get
            {
                NVec3 value = default;
                getVelocity(entity, &value);
                return value;
            }
            set
            {
                NVec3 nativeValue = value;
                setVelocity(entity, &nativeValue);
            }
        }

        public bool IsGrounded => isGrounded(entity) != 0;

        public void Move(NVec3 velocity)
        {
            Velocity = velocity;
        }
    }
}
