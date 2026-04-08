namespace vkEngine.EngineCore
{
    public static unsafe class Time
    {
        private static delegate* unmanaged[Cdecl]<float> getTime;
        private static delegate* unmanaged[Cdecl]<float> getDeltaTime;
        private static delegate* unmanaged[Cdecl]<float> getPreviousFrameTime;

        internal static void RegisterNativeFunctions(NativeFunctions* functions)
        {
            getTime = functions->GetTime;
            getDeltaTime = functions->GetDeltaTime;
            getPreviousFrameTime = functions->GetPreviousFrameTime;
        }

        public static float TimeSinceStartup
        {
            get
            {
                EnsureBound();
                return getTime();
            }
        }

        public static float DeltaTime
        {
            get
            {
                EnsureBound();
                return getDeltaTime();
            }
        }

        public static float PreviousFrameTime
        {
            get
            {
                EnsureBound();
                return getPreviousFrameTime();
            }
        }

        private static void EnsureBound()
        {
            if (!NativeFunctionRegistry.IsRegistered)
                throw new InvalidOperationException("Native time functions have not been registered.");
        }
    }
}
