#include "algo/win/host/algohost.h"

#include <tchar.h>
#include <windows.h>

#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/sandbox_factory.h"

#include "algo/win/host/coreclr_delegates.h"
#include "algo/win/host/hostfxr.h"
#include "algo/win/host/vars.h"

#define STR_EMPTY L""
#define STR_DOT L'.'
#define PATH_DELIMITER L"\\"

#define HOSTFXR_LIB L"hostfxr.dll"

#define ENDPOINT_DIR L"algohost.netcore"
#define ENDPOINT_ASM L"cTrader.Automate.Host.NetCore.dll"
#define ENDPOINT_CONFIG L"cTrader.Automate.Host.NetCore.runtimeconfig.json"
#define ENDPOINT_TYPE L"cTrader.Automate.Host.NetCore.Endpoint, cTrader.Automate.Host.NetCore"
#define ENDPOINT_METHOD L"Run"

using string_t = std::basic_string<char_t>;

namespace
{
    hostfxr_initialize_for_runtime_config_fn init_fptr;
    hostfxr_get_runtime_delegate_fn get_delegate_fptr;
    hostfxr_close_fn close_fptr;

    bool load_hostfxr(const char_t* hostfxr_path);

    load_assembly_and_get_function_pointer_fn get_dotnet_load_assembly(const char_t* dotnet_root,
                                                                       const char_t* host_path,
                                                                       const char_t* config_path);

    const string_t read_environment_variable(const char_t* name);

    void warmup();
}

int _tmain(int argc, char_t* argv[])
{
  //SleepEx(10000, false);

  warmup();

  sandbox::TargetServices* target_services = sandbox::SandboxFactory::GetTargetServices();

  const string_t product_path = read_environment_variable(CT_ENV_VAR_PRODUCT_PATH);
  const string_t dotnet_path = read_environment_variable(CT_ENV_VAR_DOTNET_PATH);
  const string_t hostfxr_path = read_environment_variable(CT_ENV_VAR_HOSTFXR_PATH);

  const string_t endpoint_dir_path = product_path + PATH_DELIMITER + ENDPOINT_DIR;
  const string_t endpoint_asm_path = endpoint_dir_path + PATH_DELIMITER + ENDPOINT_ASM;
  const string_t endpoint_config_path = endpoint_dir_path + PATH_DELIMITER + ENDPOINT_CONFIG;

  if (target_services != nullptr && target_services->Init() != sandbox::ResultCode::SBOX_ALL_OK)
    return -12;

  if (!load_hostfxr(hostfxr_path.c_str()))
    return -21;

  if (target_services != nullptr)
      target_services->LowerToken();

  load_assembly_and_get_function_pointer_fn load_assembly_and_get_function_pointer_fn =
        get_dotnet_load_assembly(dotnet_path.c_str(), product_path.c_str(), endpoint_config_path.c_str());

  if (load_assembly_and_get_function_pointer_fn == nullptr)
        return ERROR_BAD_ENVIRONMENT;

  component_entry_point_fn entry_point_fn = nullptr;
  if (load_assembly_and_get_function_pointer_fn(
        endpoint_asm_path.c_str(),
        ENDPOINT_TYPE,
        ENDPOINT_METHOD,
        nullptr,
        nullptr,
        reinterpret_cast<void**>(&entry_point_fn)) != 0 || entry_point_fn == nullptr)
        return ERROR_BAD_DLL_ENTRYPOINT;

  return entry_point_fn(nullptr, 0);
}

namespace
{
    const string_t read_environment_variable(const char_t* name)
    {
      DWORD buffer_size = 65535;
      string_t buffer;

      buffer.resize(buffer_size);
      buffer_size = GetEnvironmentVariable(name, &buffer[0], buffer_size);

      if (!buffer_size)
        return L"";

      buffer.resize(buffer_size);
      return buffer;
    }

    void warmup()
    {
      CompareStringEx(LOCALE_NAME_USER_DEFAULT, NORM_IGNORECASE, L"w", 1, L"a", 1, nullptr, nullptr, 0);
      CompareStringEx(LOCALE_NAME_INVARIANT, NORM_IGNORECASE, L"w", 1, L"a", 1, nullptr, nullptr, 0);
      CompareStringEx(LOCALE_NAME_SYSTEM_DEFAULT, NORM_IGNORECASE, L"w", 1, L"a", 1, nullptr, nullptr, 0);
      LoadLibraryEx(L"bcrypt.dll", nullptr, 0);
    }

    void* load_library(const char_t* path)
    {
        HMODULE h = LoadLibrary(path);
        assert(h != nullptr);
        return h;
    }

    void* get_export(void* h, const char* name)
    {
        void* f = reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(h), name));
        assert(f != nullptr);
        return f;
    }

    bool load_hostfxr(const char_t* hostfxr_path)
    {
        void* hostfxr_lib = load_library(hostfxr_path);

        init_fptr = reinterpret_cast<hostfxr_initialize_for_runtime_config_fn>(get_export(hostfxr_lib, "hostfxr_initialize_for_runtime_config"));
        get_delegate_fptr = reinterpret_cast<hostfxr_get_runtime_delegate_fn>(get_export(hostfxr_lib, "hostfxr_get_runtime_delegate"));
        close_fptr = reinterpret_cast<hostfxr_close_fn>(get_export(hostfxr_lib, "hostfxr_close"));

        return init_fptr && get_delegate_fptr && close_fptr;
    }

    load_assembly_and_get_function_pointer_fn get_dotnet_load_assembly(const char_t* dotnet_root,
                                                                       const char_t* host_path,
                                                                       const char_t* config_path)
    {
        hostfxr_initialize_parameters parameters{
            sizeof(hostfxr_initialize_parameters),
            host_path,
            dotnet_root,
        };

        // Load .NET Core
        void* load_assembly_and_get_function_pointer = nullptr;
        hostfxr_handle cxt = nullptr;

        int rc = init_fptr(config_path, &parameters, &cxt);
        if (rc != 0 || cxt == nullptr)
        {
            close_fptr(cxt);
            return nullptr;
        }

        // Get the load assembly function pointer
        rc = get_delegate_fptr(
            cxt,
            hdt_load_assembly_and_get_function_pointer,
            &load_assembly_and_get_function_pointer);
        if (rc != 0 || load_assembly_and_get_function_pointer == nullptr)
        {
            close_fptr(cxt);
            return nullptr;
        }

        close_fptr(cxt);
        return (load_assembly_and_get_function_pointer_fn) (load_assembly_and_get_function_pointer);
    }
}