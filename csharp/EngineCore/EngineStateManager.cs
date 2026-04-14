namespace vkEngine.EngineCore
{
    public enum EngineState
    {
        Running = 0,
        Paused = 1,
        Terminated = 2
    }

    public static unsafe class EngineStateManager
    {
        private static delegate* unmanaged[Cdecl]<int, void> setEngineState;

        internal static void RegisterNativeFunctions(NativeFunctions* functions)
        {
            setEngineState = functions->SetEngineState;
        }

        public static EngineState State
        {
            set
            {
                setEngineState((int)value);
            }
        }

        public static void SetState(EngineState state)
        {
            setEngineState((int)state);
        }
    }
}
