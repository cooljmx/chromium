#include <io.h>
#include <fcntl.h>
#include <tchar.h>
#include <windows.h>

#include <iostream>
#include <string>

#include "algo/win/broker/algobroker.h"
#include "algo/win/host/algohost.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/utf_string_conversions.h"

#define TARGET_ID "targetId"
#define PROCESS_ID "processId"

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
    freopen(NULL, "wb", stdout);
    _setmode(_fileno(stdout), _O_BINARY);
    const HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

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
        const std::string* target_id = root->FindStringKey(TARGET_ID);
        const auto wide_target = std::wstring(target_id->begin(), target_id->end());
        std::cerr << "target_id is " << &target_id << std::endl;
        const auto cmd = std::wstring(L"target, ") + wide_target;
        // add args

        algo::TargetInformation* target_result = new algo::TargetInformation;
        algo::TargetOptions* options = new algo::TargetOptions{
            exe,               // host_path
            cmd.c_str(),       // command_line
            L"test_env",       // package_name
            L"",               // file rules
            L"",               // reg_rules
            L"",               // np_rules
            L"",               // ev_rules
        };

        Spawn(options, target_result);

        if (target_result != nullptr) {
            std::wcerr << target_result->process_id << " " << target_result->thread_id << std::endl;

            base::DictionaryValue out_root;
            out_root.SetString(TARGET_ID, *target_id);
            out_root.SetString(PROCESS_ID, std::to_string(target_result->process_id).c_str());

            std::string json_string;
            base::JSONWriter::Write(out_root, &json_string);

            auto wide_json = std::wstring(json_string.begin(), json_string.end());
            wide_json += std::wstring(L"\r\n");

            unsigned long bytes_written;
            if (!WriteFile(out, wide_json.c_str(), wide_json.size() * sizeof(wchar_t), &bytes_written, NULL)) {
                std::cerr << "Can't write to pipe" << std::endl;
                return -2;
            }
            std::cerr << bytes_written << " bytes written!" << std::endl;

            if (!Resume(target_result)) {
                std::cerr << "Resume target has failed" << std::endl;
            }
        }
    }

    return 0;
}
