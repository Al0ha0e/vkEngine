#include <iostream>

#include <dotnet/nethost.h>
#include <dotnet/coreclr_delegates.h>
#include <dotnet/hostfxr.h>

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

static hostfxr_initialize_for_runtime_config_fn init_fptr = nullptr;
static hostfxr_get_runtime_delegate_fn get_delegate_fptr = nullptr;
static hostfxr_close_fn close_fptr = nullptr;

// Using the nethost library, discover the location of hostfxr and get exports
bool load_hostfxr()
{
    // Pre-allocate a large buffer for the path to hostfxr
    char_t buffer[MAX_PATH];
    size_t buffer_size = sizeof(buffer) / sizeof(char_t);
    int rc = get_hostfxr_path(buffer, &buffer_size, nullptr);
    if (rc != 0)
        return false;

    // Load hostfxr and get desired exports
    void *lib = LOAD_LIBRARY(buffer);
    init_fptr = (hostfxr_initialize_for_runtime_config_fn)GET_PROC_ADDRESS(lib, "hostfxr_initialize_for_runtime_config");
    get_delegate_fptr = (hostfxr_get_runtime_delegate_fn)GET_PROC_ADDRESS(lib, "hostfxr_get_runtime_delegate");
    close_fptr = (hostfxr_close_fn)GET_PROC_ADDRESS(lib, "hostfxr_close");

    return (init_fptr && get_delegate_fptr && close_fptr);
}

// Load and initialize .NET Core and get desired function pointer for scenario
load_assembly_and_get_function_pointer_fn get_dotnet_load_assembly(const char_t *config_path)
{
    // Load .NET Core
    void *load_assembly_and_get_function_pointer = nullptr;
    hostfxr_handle cxt = nullptr;
    int rc = init_fptr(config_path, nullptr, &cxt);
    if (rc != 0 || cxt == nullptr)
    {
        std::cerr << "Init failed: " << std::hex << std::showbase << rc << std::endl;
        close_fptr(cxt);
        return nullptr;
    }

    // Get the load assembly function pointer
    rc = get_delegate_fptr(
        cxt,
        hdt_load_assembly_and_get_function_pointer,
        &load_assembly_and_get_function_pointer);
    if (rc != 0 || load_assembly_and_get_function_pointer == nullptr)
        std::cerr << "Get delegate failed: " << std::hex << std::showbase << rc << std::endl;

    close_fptr(cxt);
    return (load_assembly_and_get_function_pointer_fn)load_assembly_and_get_function_pointer;
}

int main()
{
    if (!load_hostfxr())
    {
        std::cerr << "Failed to load hostfxr" << std::endl;
        return EXIT_FAILURE;
    }

    const char_t *config_path = L"./tests/script/test.runtimeconfig.json";

    auto load_assembly_and_get_function_pointer = get_dotnet_load_assembly(config_path);
    if (!load_assembly_and_get_function_pointer)
    {
        std::cerr << "Failed to get load_assembly_and_get_function_pointer" << std::endl;
        return EXIT_FAILURE;
    }

    const char_t *assembly_path = L"./tests/script/test.dll";
    const char_t *type_name = L"EntryPoint, test";
    const char_t *method_name = L"Run";

    // Function pointer to managed delegate
    component_entry_point_fn hello = nullptr;
    int rc = load_assembly_and_get_function_pointer(
        assembly_path,
        type_name,
        method_name,
        nullptr /*delegate_type_name*/,
        nullptr,
        (void **)&hello);

    if (rc != 0 || hello == nullptr)
    {
        std::cerr << "load_assembly_and_get_function_pointer failed: " << rc << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "!!!!!!!!!!!0";
    hello(nullptr, 0);

    return 0;
}