#include <common.hpp>
#include <script.hpp>
#include <logger.hpp>
#include <iostream>

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
    static const char_t *CSHARP_CONFIG_PATH = REL_DIR_W L"/csharp/EngineCore.runtimeconfig.json";
    static const char_t *CSHARP_ASSEMBLY_PATH = REL_DIR_W L"/csharp/EngineCore.dll";
    static const char_t *CSHARP_TYPE_NAME = L"vkEngine.EngineCore.EntryPoint, EngineCore";
    static const char_t *CSHARP_INIT_METHOD_NAME = L"Init";

    typedef void(CORECLR_DELEGATE_CALLTYPE *CSharpSideInitFunction)();

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

    void ScriptManager::init()
    {
        VKE_FATAL_IF(!loadHostFXR(), "Failed to load hostfxr")

        getDotnetLoadAssembly(CSHARP_CONFIG_PATH, functionPointers);

        CSharpSideInitFunction initFunc = nullptr;
        int rc = functionPointers.loadAssemblyAndGetFunctionPointer(
            CSHARP_ASSEMBLY_PATH, CSHARP_TYPE_NAME, CSHARP_INIT_METHOD_NAME,
            UNMANAGEDCALLERSONLY_METHOD, nullptr, (void **)&initFunc);
        VKE_FATAL_IF(rc != 0 || initFunc == nullptr, "getFunctionPointer failed: {}", rc)
        initFunc();
    }
}