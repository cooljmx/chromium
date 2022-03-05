#include <tchar.h>
#include <windows.h>

#include <iostream>
#include <string>

#include "algo/win/broker/algobroker.h"
#include "algo/win/host/algohost.h"

int run_broker_main(int argc, wchar_t** argv);

int _tmain(int argc, wchar_t* argv[]) {
	if (argc > 1) {
		return host_main(argc, argv);
	}
	else {
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
		L"host",                              // command_line
		L"test_env",                          // package_name
		L"c:\\chromium\\src\\out\\x64_algo\\libc++.dll|RW",        // fs_rules
		L"",                                  // reg_rules
		L"",                                  // np_rules
		L"",                                  // ev_rules
	};

	Sleep(15 * 1000);

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
