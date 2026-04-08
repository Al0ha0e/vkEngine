#ifndef SCRIPT_H
#define SCRIPT_H

#include <dotnet/nethost.h>
#include <dotnet/coreclr_delegates.h>
#include <dotnet/hostfxr.h>

#include <interop/native.hpp>

#include <cstdint>
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

    struct CSharpSceneManagerFunctions
    {
        void (*load)(const char **, uint32_t);
        void (*start)();
        void (*update)();
        void (*fixedUpdate)();
        void (*lateUpdate)();
        void (*unload)();

        CSharpSceneManagerFunctions()
            : load(nullptr),
              start(nullptr),
              update(nullptr),
              fixedUpdate(nullptr),
              lateUpdate(nullptr),
              unload(nullptr) {}
    };

    struct CSharpExports
    {
        int (*init)();
        void (*registerNativeFunctions)(vke_interop::NativeFunctions *);
        CSharpSceneManagerFunctions sceneManagerFunctions;

        CSharpExports()
            : init(nullptr),
              registerNativeFunctions(nullptr),
              sceneManagerFunctions() {}
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
            if (instance == nullptr)
                return;
            delete instance;
            instance = nullptr;
        }

        static void Load(const char **data, uint32_t cnt)
        {
            instance->csharpExports.sceneManagerFunctions.load(data, cnt);
        }

        static void Start()
        {
            instance->csharpExports.sceneManagerFunctions.start();
        }

        static void Update()
        {
            instance->csharpExports.sceneManagerFunctions.update();
        }

        static void Unload()
        {
            instance->csharpExports.sceneManagerFunctions.unload();
        }

    private:
        DelegateFunctionPointers functionPointers;
        CSharpExports csharpExports;

        void init();

        void registerNativeFunctions();
        void getCSharpExports();
        void getFunctionPointer(const char_t *typeName, const char_t *methodName, void **func);
    };
}

#endif