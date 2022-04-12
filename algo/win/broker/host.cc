#include <assert.h>
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
#include "base/values.h"

#define TARGET_ID "targetId"
#define PROCESS_ID "processId"
#define ARGS "args"
#define PACKAGE "packageName"
#define FS_RULES "filesystemRules"
#define PIPE_RULES "pipeRules"
#define EVENT_RULES "eventRules"
#define REG_RULES "registryRules"
#define PATTERN "pattern"
#define RO "readOnly"
#define WIDE_SPACE std::wstring(L" ")
#define SEMICOLON std::string(";")
#define PIPE std::string("|")

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

const std::wstring get_value(const char* key, const base::Optional<base::Value>& node) {
    const std::string* const value = node->FindStringKey(key);
    std::string rule;

    if (value) {
        rule += *value;
    }
    else {
        const base::Value* list_value = node->FindListKey(key);

        for (const auto& entry : list_value->GetList()) {
            if (rule.size()) {
                rule += SEMICOLON;
            }
            if (entry.is_dict()) {
                for (const auto& kv : entry.DictItems()) {
                    std::cerr << kv.first << " is " << kv.second << std::endl;
                }

                base::Optional<bool> ro = entry.FindBoolKey(RO);
                const std::string* const pattern = entry.FindStringKey(PATTERN);
                assert(pattern);
                rule += *pattern;

                if (ro) {
                    rule += PIPE + (ro.value() ? std::string("RO") : std::string("RW"));
                }
            }
            else if (entry.is_string()) {
                rule += entry.GetString();
            }
            else {
                std::cerr << "unknown type of node" << std::endl;
            }
        }
    }

    std::cerr << key << " is " << rule << std::endl;
    return std::wstring(rule.begin(), rule.end());
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

        const auto target = get_value(TARGET_ID, root);
        const auto args = get_value(ARGS, root);
        const auto package_name = get_value(PACKAGE, root);
        const auto fs_rules = get_value(FS_RULES, root);
        const auto pipe_rules = get_value(PIPE_RULES, root);
        const auto event_rules = get_value(EVENT_RULES, root);
        const auto reg_rules = get_value(REG_RULES, root);
        const auto cmd = std::wstring(L"target, ") + target + WIDE_SPACE + args;

        algo::TargetInformation* target_result = new algo::TargetInformation;
        algo::TargetOptions* options = new algo::TargetOptions{
            exe,                         // host_path
            cmd.c_str(),                 // command_line
            package_name.c_str(),        // package_name
            fs_rules.c_str(),            // file rules
            reg_rules.c_str(),           // reg_rules
            pipe_rules.c_str(),          // np_rules
            event_rules.c_str(),         // ev_rules
        };

        Spawn(options, target_result);

        if (target_result != nullptr) {
            std::wcerr << target_result->process_id << " " << target_result->thread_id << std::endl;

            base::DictionaryValue out_root;
            out_root.SetString(TARGET_ID, target);
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
