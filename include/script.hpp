#ifndef SCRIPT_H
#define SCRIPT_H

#include <dotnet/nethost.h>
#include <dotnet/coreclr_delegates.h>
#include <dotnet/hostfxr.h>

#include <interop/native.hpp>
#include <reflect/type_info.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace vke_common
{
    extern const std::string EngineCSharpPath;

    struct CSharpScriptLoadData
    {
        uint32_t entity;
        const char *className;
        const std::byte *data;
        int32_t dataSize;
    };

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
        void (*load)(const CSharpScriptLoadData *, uint32_t);
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

        static void Load(const CSharpScriptLoadData *data, uint32_t cnt)
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

        static void FixedUpdate()
        {
            instance->csharpExports.sceneManagerFunctions.fixedUpdate();
        }

        static void Unload()
        {
            instance->csharpExports.sceneManagerFunctions.unload();
        }

        TypeInfoPtr FindTypeInfo(std::string_view name) const
        {
            const auto value = typeInfos.find(std::string(name));
            return value == typeInfos.end() ? nullptr : value->second;
        }

        const std::unordered_map<std::string, TypeInfoPtr> &TypeInfos() const noexcept
        {
            return typeInfos;
        }

    private:
        DelegateFunctionPointers functionPointers;
        CSharpExports csharpExports;
        std::unordered_map<std::string, TypeInfoPtr> typeInfos;

        void init();
        void loadTypeInfos(const std::string &path);

        void registerNativeFunctions();
        void getCSharpExports();
        void getFunctionPointer(const char_t *typeName, const char_t *methodName, void **func);
    };
}

#endif
