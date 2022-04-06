#include <tchar.h>
#include <windows.h>

#include <iostream>
#include <string>

#include "algo/win/broker/algobroker.h"
#include "algo/win/host/algohost.h"
#include "base/json/json_reader.h"
#include "base/strings/utf_string_conversions.h"

int run_broker_main(int argc, wchar_t** argv);

int _tmain(int argc, wchar_t* argv[]) {
    Sleep(10 * 1000);
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

    wchar_t exe[MAX_PATH];
    if (!GetModuleFileNameW(nullptr, exe, MAX_PATH)) {
        std::cerr << "Get module name has failed: " << GetLastError() << std::endl;
        return -1;
    }

    std::ios_base::sync_with_stdio(false);
    for (std::string line; std::getline(std::cin, line);) {
        std::cerr << "line size is " << line.size() << std::endl;

        const auto wline = std::wstring( (wchar_t*)line.data(), line.size() / 2);
        std::cerr << "wline size is " << wline.size() << std::endl;

        std::string output;
        if (!base::UTF16ToUTF8(wline.c_str(), wline.size(), &output)) {
            std::cerr << "Couldn't convert UTF16 to UTF8" << std::endl;
            return -2;
        }

        const auto& narrow_line = output;
        base::Optional<base::Value> root = base::JSONReader::Read(narrow_line);
        if (!root || root == base::nullopt) {
            std::cerr << "Bad JSON: " << narrow_line << std::endl;
            continue;
        }
        const std::string *target_id = root->FindStringKey("targetId");
        std::cerr << "target_id is " << &target_id << std::endl;

        algo::TargetInformation* target_result = new algo::TargetInformation;
        algo::TargetOptions* options = new algo::TargetOptions{
            exe,                                              // host_path
            L"target, 0B90B2ED-7DBA-4FD6-B17E-C47533300557",  // command_line
            L"test_env",                                      // package_name
            L"",                                              // file rules
            L"",                                              // reg_rules
            L"",                                              // np_rules
            L"",                                              // ev_rules
        };

        Spawn(options, target_result);

        if (target_result != nullptr) {
            std::wcout << target_result->process_id << " " << target_result->thread_id << std::endl;
            if (!Resume(target_result)) {
                std::cerr << "Resume target has failed" << std::endl;
            }
        }
    }

    return 0;
}
