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
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"

#define TITLE "title"
#define TARGET_ID "targetId"
#define RESULT "result"
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
#define PIPE std::string("|")

int run_broker_main(int argc, wchar_t** argv);

int _tmain(int argc, wchar_t* argv[]) {
//  Sleep(10 * 1000);
    Sleep(1 * 1000);
    if (argc > 1) {
        return host_main(argc, argv);
    }
    else {
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
                rule += PIPE;
            }
            if (entry.is_dict()) {
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
                LOG(INFO) << "unknown type of node" << std::endl;
            }
        }
    }

    LOG(INFO) << key << " is " << rule << std::endl;
    std::wstring output;
    base::UTF8ToUTF16(rule.c_str(), rule.size(), &output);
    return output;
}

int run_broker_main(int argc, wchar_t** argv) {
    Initialize();
    LOG(INFO) << "broker" << std::endl;

    wchar_t exe[MAX_PATH];
    if (!GetModuleFileNameW(nullptr, exe, MAX_PATH)) {
        LOG(INFO) << "Get module name has failed: " << GetLastError() << std::endl;
        return -1;
    }

    std::ios_base::sync_with_stdio(false);
    freopen(NULL, "wb", stdout);
    _setmode(_fileno(stdout), _O_BINARY);
    const HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

    for (std::string line; std::getline(std::cin, line);) {
        LOG(INFO) << "line size is " << line.size() << std::endl;

        const auto wline = std::wstring( (wchar_t*)line.data(), line.size() / 2);
        LOG(INFO) << "wline size is " << wline.size() << std::endl;

        std::string output;
        if (!base::UTF16ToUTF8(wline.c_str(), wline.size(), &output)) {
            LOG(INFO) << "Couldn't convert UTF16 to UTF8" << std::endl;
            return -2;
        }

        const auto& narrow_line = output;
        base::Optional<base::Value> root = base::JSONReader::Read(narrow_line);
        if (!root || root == base::nullopt) {
            LOG(INFO) << "Bad JSON: " << narrow_line << std::endl;
            continue;
        }

        const auto title = get_value(TITLE, root);
        const auto target = get_value(TARGET_ID, root);
        const auto args = get_value(ARGS, root);
        const auto package_name = get_value(PACKAGE, root);
        const auto fs_rules = get_value(FS_RULES, root);
        const auto pipe_rules = get_value(PIPE_RULES, root);
        const auto event_rules = get_value(EVENT_RULES, root);
        const auto reg_rules = get_value(REG_RULES, root);
        std::wstring quoted_title = L"\"" + title + L"\"";
        const auto cmd = quoted_title + WIDE_SPACE + target + WIDE_SPACE + args;

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

        int result = Spawn(options, target_result);

        if (target_result != nullptr) {
            LOG(INFO) << "launching target with pid " << target_result->process_id << std::endl;

            base::DictionaryValue out_root;
            out_root.SetString(TARGET_ID, target);
            out_root.SetInteger(PROCESS_ID, target_result->process_id);
            out_root.SetInteger(RESULT, result);

            std::string json_string;
            base::JSONWriter::Write(out_root, &json_string);

            auto wide_json = std::wstring(json_string.begin(), json_string.end());
            wide_json += std::wstring(L"\r\n");

            unsigned long bytes_written;
            if (!WriteFile(out, wide_json.c_str(), wide_json.size() * sizeof(wchar_t), &bytes_written, NULL)) {
                LOG(INFO) << "Can't write to pipe" << std::endl;
                return -2;
            }
            LOG(INFO) << bytes_written << " bytes written!" << std::endl;

            if (!Resume(target_result)) {
                LOG(INFO) << "Resume target has failed" << std::endl;
            }
        }
    }

    return 0;
}
