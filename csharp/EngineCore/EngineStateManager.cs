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
                EnsureBound();
                setEngineState((int)value);
            }
        }

        public static void SetState(EngineState state)
        {
            EnsureBound();
            setEngineState((int)state);
        }

        private static void EnsureBound()
        {
            if (!NativeFunctionRegistry.IsRegistered)
                throw new InvalidOperationException("Native engine state functions have not been registered.");
        }
    }
}
