using System.Runtime.InteropServices;

namespace vkEngine.EngineCore
{
    public static class EntryPoint
    {
        [UnmanagedCallersOnly]
        public unsafe static int Init()
        {
            Console.WriteLine("Hello World");
            return 0;
        }
    }
}
