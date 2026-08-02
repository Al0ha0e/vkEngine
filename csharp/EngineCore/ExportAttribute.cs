using System;

namespace vkEngine.EngineCore
{
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property, AllowMultiple = false, Inherited = true)]
    public sealed class ExportAttribute : Attribute
    {
        public ExportAttribute()
        {
        }

        public ExportAttribute(string name)
        {
            Name = name;
        }

        public string? Name { get; }

        public string? CppType { get; set; }
    }
}
