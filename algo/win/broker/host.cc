#include <tchar.h>
#include <windows.h>

#include "algo/win/broker/algobroker.h"
#include <iostream>

int _tmain(int argc, wchar_t* argv[]) {
  Initialize(L"C:\\src\\chromium\\src\\out\\algo");

  algo::TargetInformation* target_result = new algo::TargetInformation;

  algo::TargetOptions* options = new algo::TargetOptions{
    L"algohost.exe",
    L"",
    L"",
    L""
  };

  Sleep(10000);

  Spawn(options, target_result);

  if (target_result != nullptr) {
    std::cout << target_result->process_id << " " << target_result->thread_id;
  }

  return 0;
}
