#ifndef SCRIPT_H
#define SCRIPT_H

#include <dotnet/nethost.h>
#include <dotnet/coreclr_delegates.h>
#include <dotnet/hostfxr.h>

#include <string>

namespace vke_common
{
    extern const std::string EngineCSharpPath;

    struct DelegateFunctionPointers
    {
        load_assembly_and_get_function_pointer_fn loadAssemblyAndGetFunctionPointer;
        load_assembly_fn loadAssembly;
        get_function_pointer_fn getFunctionPointer;

        DelegateFunctionPointers()
            : loadAssemblyAndGetFunctionPointer(nullptr),
              loadAssembly(nullptr),
              getFunctionPointer(nullptr) {}
    };

    class ScriptManager
    {
    private:
        static ScriptManager *instance;
        ScriptManager() {}
        ~ScriptManager() {}

    public:
        static ScriptManager *GetInstance()
        {
            return instance;
        }

        static ScriptManager *Init()
        {
            instance = new ScriptManager();
            instance->init();
            return instance;
        }

        static void Dispose()
        {
            delete instance;
        }

    private:
        DelegateFunctionPointers functionPointers;

        void init();
    };
}

#endif