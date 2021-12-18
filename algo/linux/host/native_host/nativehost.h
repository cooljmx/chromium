#ifndef NATIVEHOST__
#define NATIVEHOST__

#include <coreclr_delegates.h>

component_entry_point_fn launch_dotnet(const char* native_host_path, const char* dll_path, const char* entry_point, const char* config);

#endif
