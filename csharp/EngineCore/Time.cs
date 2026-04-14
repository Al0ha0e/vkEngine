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
                return getTime();
            }
        }

        public static float DeltaTime
        {
            get
            {
                return getDeltaTime();
            }
        }

        public static float PreviousFrameTime
        {
            get
            {
                return getPreviousFrameTime();
            }
        }
    }
}
