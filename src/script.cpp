#include <common.hpp>
#include <script.hpp>
#include <logger.hpp>
#include <iostream>
#include <game_config.hpp>

#ifdef _WIN32
#include <windows.h>
#define LOAD_LIBRARY(path) LoadLibraryW(path)
#define GET_PROC_ADDRESS(lib, name) GetProcAddress((HMODULE)lib, name)
#define LIB_HANDLE HMODULE
#else
#include <dlfcn.h>
#define LOAD_LIBRARY(path) dlopen(path, RTLD_LAZY | RTLD_LOCAL)
#define GET_PROC_ADDRESS(lib, name) dlsym(lib, name)
#define LIB_HANDLE void *
#endif

namespace vke_common
{
    const std::string EngineCSharpPath = std::string(REL_DIR) + "/csharp";
    static const char_t *ENGINE_CORE_CSHARP_CONFIG_PATH = REL_DIR_W L"/csharp/EngineCore.runtimeconfig.json";
    static const char_t *ENGINE_CORE_CSHARP_ASSEMBLY_PATH = REL_DIR_W L"/csharp/EngineCore.dll";
    static const char_t *CSHARP_TYPE_NAME = L"vkEngine.EngineCore.EntryPoint, EngineCore";
    static const char_t *CSHARP_GET_EXPORTS_METHOD_NAME = L"GetCSharpExports";

    typedef void(CORECLR_DELEGATE_CALLTYPE *CSharpSideGetExportsFunction)(CSharpExports *);

    static hostfxr_initialize_for_runtime_config_fn initFptr = nullptr;
    static hostfxr_get_runtime_delegate_fn getDelegateFptr = nullptr;
    static hostfxr_close_fn closeFptr = nullptr;

    static bool loadHostFXR()
    {
        char_t buffer[MAX_PATH];
        size_t buffer_size = sizeof(buffer) / sizeof(char_t);
        int rc = get_hostfxr_path(buffer, &buffer_size, nullptr);
        if (rc != 0)
            return false;

        void *lib = LOAD_LIBRARY(buffer);
        initFptr = (hostfxr_initialize_for_runtime_config_fn)GET_PROC_ADDRESS(lib, "hostfxr_initialize_for_runtime_config");
        getDelegateFptr = (hostfxr_get_runtime_delegate_fn)GET_PROC_ADDRESS(lib, "hostfxr_get_runtime_delegate");
        closeFptr = (hostfxr_close_fn)GET_PROC_ADDRESS(lib, "hostfxr_close");

        return (initFptr && getDelegateFptr && closeFptr);
    }

    static void getDotnetLoadAssembly(const char_t *config_path, DelegateFunctionPointers &functionPointers)
    {
        hostfxr_handle cxt = nullptr;
        int rc = initFptr(config_path, nullptr, &cxt);
        VKE_FATAL_IF(rc != 0 || cxt == nullptr, "HostFxr Init failed: {}", rc)

        rc = getDelegateFptr(
            cxt,
            hdt_load_assembly_and_get_function_pointer,
            (void **)&(functionPointers.loadAssemblyAndGetFunctionPointer));
        VKE_FATAL_IF(rc != 0 || functionPointers.loadAssemblyAndGetFunctionPointer == nullptr, "Get loadAssemblyAndGetFunctionPointer failed: {}", rc)

        rc = getDelegateFptr(
            cxt,
            hdt_load_assembly,
            (void **)&(functionPointers.loadAssembly));
        VKE_FATAL_IF(rc != 0 || functionPointers.loadAssembly == nullptr, "Get loadAssembly failed: {}", rc)

        rc = getDelegateFptr(
            cxt,
            hdt_get_function_pointer,
            (void **)&(functionPointers.getFunctionPointer));
        VKE_FATAL_IF(rc != 0 || functionPointers.getFunctionPointer == nullptr, "Get getFunctionPointer failed: {}", rc)

        closeFptr(cxt);
    }

    ScriptManager *ScriptManager::instance = nullptr;

    void ScriptManager::getFunctionPointer(const char_t *typeName, const char_t *methodName, void **func)
    {
        int rc = functionPointers.getFunctionPointer(
            typeName, methodName, UNMANAGEDCALLERSONLY_METHOD, nullptr, nullptr, func);
        VKE_FATAL_IF(rc != 0 || *func == nullptr, "getFunctionPointer failed: {}", rc)
    }

    void ScriptManager::init()
    {
        VKE_FATAL_IF(!loadHostFXR(), "Failed to load hostfxr")
        getDotnetLoadAssembly(ENGINE_CORE_CSHARP_CONFIG_PATH, functionPointers);
        functionPointers.loadAssembly(ENGINE_CORE_CSHARP_ASSEMBLY_PATH, nullptr, nullptr);

        std::string &gameAssemblyPath = GameConfig::GetInstance()->gameScriptPath;
        if (gameAssemblyPath.length() > 0)
            functionPointers.loadAssembly(std::wstring(gameAssemblyPath.begin(), gameAssemblyPath.end()).c_str(), nullptr, nullptr);

        getCSharpExports();
        int rc = csharpExports.init();
        VKE_FATAL_IF(rc != 0, "C# Init failed: {}", rc)
        registerNativeFunctions();
    }

    void ScriptManager::getCSharpExports()
    {
        CSharpSideGetExportsFunction getExportsFunc = nullptr;
        getFunctionPointer(CSHARP_TYPE_NAME,
                           CSHARP_GET_EXPORTS_METHOD_NAME,
                           (void **)&getExportsFunc);
        getExportsFunc(&csharpExports);
    }
}
