#include <tchar.h>
#include <windows.h>

#include <iostream>
#include <string>

#include "algo/win/broker/algobroker.h"
#include "algo/win/host/algohost.h"

int run_broker_main(int argc, wchar_t** argv);

int _tmain(int argc, wchar_t* argv[]) {
    Sleep(15 * 1000);
    if (argc > 1) {
        std::cerr << "host" << std::endl;
        return host_main(argc, argv);
    }
    else {
        std::cerr << "broker" << std::endl;
        return run_broker_main(argc, argv);
    }
}

int run_broker_main(int argc, wchar_t** argv) {
    Initialize();

    wchar_t out_algo_buffer[MAX_PATH];
    if (!GetModuleFileNameW(nullptr, out_algo_buffer, MAX_PATH)) {
        std::cerr << "Get module name has failed: " << GetLastError() << std::endl;
        return -1;
    }
    const std::wstring out_algo = out_algo_buffer;
    const std::wstring host_base = out_algo.substr(0, out_algo.find_last_of(L"/\\"));
    const std::wstring host_path = host_base + std::wstring(L"\\one.netcore.exe");
    const wchar_t* host = host_path.c_str();

    algo::TargetInformation* target_result = new algo::TargetInformation;
    algo::TargetOptions* options = new algo::TargetOptions{
        host,                                 // host_path
        L"exe host",                          // command_line
        L"test_env",                          // package_name
        L"\\??\\C:\\chromium\\src\\out\\testing_app\\testing_app.runtimeconfig.json|RO;"                       // fs_rules
        L"\\??\\C:\\Program Files (x86)|RO;"
        L"\\??\\C:\\Program Files (x86)\\dotnet\\shared|RO;"
        L"\\??\\C:\\Program Files (x86)\\dotnet\\host\\fxr\\6.0.1\\hostfxr.dll|RO;"
        L"\\??\\C:\\Program Files (x86)\\dotnet\\shared\\Microsoft.NETCore.App|RO;"
        L"\\??\\C:\\Program Files (x86)\\dotnet\\shared\\Microsoft.NETCore.App\\|RO;"
        L"\\??\\C:\\Program Files (x86)\\dotnet\\shared\\Microsoft.NETCore.App\\*|RO;"
        L"\\??\\C:\\Program Files (x86)\\dotnet\\shared\\Microsoft.NETCore.App\\6.0.3|RO;"
        L"\\??\\C:\\Program Files (x86)\\dotnet\\shared\\Microsoft.NETCore.App\\6.0.3\\hostpolicy.dll|RO;"
        L"\\??\\C:\\Program Files (x86)\\dotnet\\shared\\Microsoft.NETCore.App\\6.0.3\\Microsoft.NETCore.App.deps.json|RO;"
        L"\\??\\C:\\Program Files (x86)\\dotnet\\shared\\Microsoft.NETCore.App\\6.0.3\\System.Private.CoreLib.dll|RO;"
        L"\\??\\C:\\Program Files (x86)\\dotnet\\shared\\Microsoft.NETCore.App\\6.0.3\\coreclr.dll|RO;"
        L"\\??\\C:\\Program Files (x86)\\dotnet\\shared\\Microsoft.NETCore.App\\6.0.3\\System.Runtime.dll|RO;"
        L"\\??\\C:\\Program Files (x86)\\dotnet\\shared\\Microsoft.NETCore.App\\6.0.3\\mscorrc.dll|RO;"
        L"\\??\\C:\\Program Files (x86)\\dotnet\\shared\\Microsoft.NETCore.App\\6.0.3\\clrjit.dll|RO;"
        L"\\??\\C:\\Program Files\\dotnet\\shared|RO;"
        L"\\??\\C:\\Program Files\\dotnet\\host\\fxr\\6.0.1\\hostfxr.dll|RO;"
        L"\\??\\C:\\Program Files\\dotnet\\shared\\Microsoft.NETCore.App|RO;"
        L"\\??\\C:\\Program Files\\dotnet\\shared\\Microsoft.NETCore.App\\|RO;"
        L"\\??\\C:\\Program Files\\dotnet\\shared\\Microsoft.NETCore.App\\*|RO;"
        L"\\??\\C:\\Program Files\\dotnet\\shared\\Microsoft.NETCore.App\\6.0.3|RO;"
        L"\\??\\C:\\Program Files\\dotnet\\shared\\Microsoft.NETCore.App\\6.0.3\\hostpolicy.dll|RO;"
        L"\\??\\C:\\Program Files\\dotnet\\shared\\Microsoft.NETCore.App\\6.0.3\\Microsoft.NETCore.App.deps.json|RO;"
        L"\\??\\C:\\Program Files\\dotnet\\shared\\Microsoft.NETCore.App\\6.0.3\\System.Private.CoreLib.dll|RO;"
        L"\\??\\C:\\Program Files\\dotnet\\shared\\Microsoft.NETCore.App\\6.0.3\\mscorrc.dll|RO;"
        L"\\??\\C:\\Program Files\\dotnet\\shared\\Microsoft.NETCore.App\\6.0.3\\coreclr.dll|RO;"
        L"\\??\\C:\\Program Files\\dotnet\\shared\\Microsoft.NETCore.App\\6.0.3\\clrjit.dll|RO;"
        L"\\??\\C:\\Windows\\system32\\rpcss.dll|RO;"
        L"\\??\\C:\\chromium\\src\\out\\testing_app\\testing_app.deps.json|RO;"
        L"\\??\\C:\\chromium\\src\\out\\testing_app\\testing_app.dll|RO;"
        L"\\??\\C:\\chromium\\src\\out\\testing_app\\*|RO;"
        L"\\??\\C:\\chromium\\src\\out\\x64_algo\\one.netcore.exe|RW;"
        L"\\??\\C:\\chromium\\src\\out\\x86_algo\\one.netcore.exe|RW;"
        L"\\??\\C:\\Users\\arttr\\Documents\\out.txt|RW;"
        L"\\??\\C:\\Users\\arttr\\Documents|RW",
        L"",                                  // reg_rules
        L"",                                  // np_rules
        L"",                                  // ev_rules
    };

    Spawn(options, target_result);

    if (target_result != nullptr) {
        std::cout << target_result->process_id << " " << target_result->thread_id;
        if (!Resume(target_result)) {
            std::cerr << "Resume target has failed" << std::endl;
            return -2;
        }
        WaitAll();
    }

    return 0;
}
