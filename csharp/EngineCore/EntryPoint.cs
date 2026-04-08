using System.Runtime.InteropServices;

namespace vkEngine.EngineCore
{
    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct CSharpExports
    {
        public delegate* unmanaged<int> Init;
        public delegate* unmanaged<NativeFunctions*, void> RegisterNativeFunctions;
        public SceneManagerFunctions SceneManagerFuncs;
    }

    public static class EntryPoint
    {
        [UnmanagedCallersOnly]
        public unsafe static int Init()
        {
            Console.WriteLine("Hello World");
            return 0;
        }

        [UnmanagedCallersOnly]
        public unsafe static void RegisterNativeFunctions(NativeFunctions* funcs)
        {
            NativeFunctionRegistry.Register(funcs);
        }

        [UnmanagedCallersOnly]
        public unsafe static void GetCSharpExports(CSharpExports* exports)
        {
            *exports = new CSharpExports
            {
                Init = &Init,
                RegisterNativeFunctions = &RegisterNativeFunctions,
                SceneManagerFuncs = SceneManager.GetFunctions()
            };
        }
    }
}
