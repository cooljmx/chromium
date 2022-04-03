#include <tchar.h>
#include <shlwapi.h>
#include <aclapi.h>
#include <iostream>

#include "algo/win/broker/algobroker.h"
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/sandbox_factory.h"
#include "sandbox/win/src/app_container_profile.h"
#include "sandbox/win/src/app_container_profile_base.h"
#include "sandbox/win/src/restricted_token_utils.h"

using namespace sandbox;

ResultCode SetupProtectedMode(
  const scoped_refptr<TargetPolicy>& target_policy,
  const wchar_t* package_name) {
  ResultCode result;

  // If stdout/stderr point to a Windows console, these calls will
  // have no effect. These calls can fail with SBOX_ERROR_BAD_PARAMS.
  target_policy->SetStdoutHandle(GetStdHandle(STD_OUTPUT_HANDLE));
  target_policy->SetStderrHandle(GetStdHandle(STD_ERROR_HANDLE));

  do {
    result = target_policy->SetTokenLevel(
      USER_RESTRICTED_SAME_ACCESS, USER_LOCKDOWN);
    if (result != SBOX_ALL_OK)
      break;

//  result = target_policy->SetDelayedIntegrityLevel(INTEGRITY_LEVEL_UNTRUSTED);
//  if (result != SBOX_ALL_OK)
//    break;

//  result = target_policy->SetAlternateDesktop(true);
//  if (result != SBOX_ALL_OK)
//    break;

//  result = target_policy->SetJobLevel(JOB_LOCKDOWN, 0);
//  if (result != SBOX_ALL_OK)
//    break;

  //result = target_policy->AddAppContainerProfile(
  //  package_name, true);

  //if (result == SBOX_ERROR_UNSUPPORTED)
  //{
  //  std::wcerr << L"AppContainer profile is not supported" << std::endl;
  //  result = SBOX_ALL_OK;
  //}

    if (result != SBOX_ALL_OK)
      break;
  }
  while (false);

  return result;
}

ResultCode SpawnTarget(const wchar_t* path,
                       const wchar_t* arguments,
                       BrokerServices* broker_services,
                       scoped_refptr<TargetPolicy> target_policy,
                       PROCESS_INFORMATION* process_information) {
  std::wcerr << L"Target path: " << path << std::endl;
  std::wcerr << L"Target arguments: " << arguments << std::endl;

  ResultCode last_warning = SBOX_ALL_OK;
  DWORD last_error = 0;

  const ResultCode result = broker_services->SpawnTarget(path,
                                                         arguments,
                                                         target_policy,
                                                         &last_warning,
                                                         &last_error,
                                                         process_information);
  if (result != SBOX_ALL_OK) {
    if (last_warning != SBOX_ALL_OK) {
      std::wcerr << L"Last warning: " << last_warning << std::endl;
    }
    if (last_error != 0) {
      LPWSTR messageBuffer = nullptr;
      size_t size = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, last_error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&messageBuffer, 0, nullptr);

      const std::wstring message(messageBuffer, size);
      std::wcerr << L"Last error: " << message.c_str() << std::endl;
    }
  }

  return result;
}

std::vector<std::wstring> SplitString(const std::wstring& str, wchar_t ch) {
  std::vector<std::wstring> ret;
  size_t startPos = 0;
  size_t endPos = str.find(ch, startPos);
  while (endPos != std::wstring::npos) {
    ret.push_back(str.substr(startPos, endPos - startPos));
    startPos = endPos + 1;
    endPos = str.find(ch, startPos);
  }
  ret.push_back(str.substr(startPos, str.length() - startPos));
  return ret;
}

ResultCode SetupFileRules(scoped_refptr<TargetPolicy> target_policy,
                          const wchar_t* rules) {
  ResultCode result = SBOX_ALL_OK;

  if (rules == nullptr)
    return result;

  std::wstring rules_string(rules);
  if (rules_string.length() == 0)
    return result;

  std::vector<std::wstring> rules_array = SplitString(rules_string, L';');

  for (std::wstring rule : rules_array) {
    std::vector<std::wstring> rule_desc = SplitString(rule, '|');

    auto rule_path = rule_desc[0];
    if (rule_desc.size() > 1 && rule_desc[1] == L"RW") {
      result = target_policy->AddRule(TargetPolicy::SubSystem::SUBSYS_FILES,
                                      TargetPolicy::Semantics::FILES_ALLOW_ANY,
                                      rule_path.c_str());
    } else {
      result = target_policy->AddRule(TargetPolicy::SubSystem::SUBSYS_FILES,
                                      TargetPolicy::Semantics::FILES_ALLOW_READONLY,
                                      rule_path.c_str());
    }

    if (result != SBOX_ALL_OK)
      break;

    std::wcerr << L"Rule [FileSystem] added: " << rule.c_str() << std::endl;
  }

  return result;
}

ResultCode SetupRegistryRules(scoped_refptr<TargetPolicy> target_policy,
                          const wchar_t* rules) {
  ResultCode result = SBOX_ALL_OK;

  if (rules == nullptr)
    return result;

  std::wstring rules_string(rules);
  if (rules_string.length() == 0)
    return result;

  std::vector<std::wstring> rules_array = SplitString(rules_string, L';');

  for (std::wstring rule : rules_array) {
    std::vector<std::wstring> rule_desc = SplitString(rule, '|');

    auto rule_path = rule_desc[0];
    if (rule_desc.size() > 1 && rule_desc[1] == L"RW") {
      result = target_policy->AddRule(TargetPolicy::SubSystem::SUBSYS_REGISTRY,
                                      TargetPolicy::Semantics::REG_ALLOW_ANY,
                                      rule_path.c_str());
    } else {
      result = target_policy->AddRule(TargetPolicy::SubSystem::SUBSYS_REGISTRY,
                                      TargetPolicy::Semantics::REG_ALLOW_READONLY,
                                      rule_path.c_str());
    }

    if (result != SBOX_ALL_OK)
      break;

    std::wcerr << L"Rule [Registry] added: " << rule.c_str() << std::endl;
  }

  return result;
}

ResultCode SetupEventRules(scoped_refptr<TargetPolicy> target_policy,
                          const wchar_t* rules) {
  ResultCode result = SBOX_ALL_OK;

  if (rules == nullptr)
    return result;

  std::wstring rules_string(rules);
  if (rules_string.length() == 0)
    return result;

  std::vector<std::wstring> rules_array = SplitString(rules_string, L';');

  for (std::wstring rule : rules_array) {
    std::vector<std::wstring> rule_desc = SplitString(rule, '|');

    auto rule_path = rule_desc[0];
    if (rule_desc.size() > 1 && rule_desc[1] == L"RW") {
      result = target_policy->AddRule(TargetPolicy::SubSystem::SUBSYS_SYNC,
                                      TargetPolicy::Semantics::EVENTS_ALLOW_ANY,
                                      rule_path.c_str());
    } else {
      result = target_policy->AddRule(TargetPolicy::SubSystem::SUBSYS_SYNC,
                                      TargetPolicy::Semantics::EVENTS_ALLOW_READONLY,
                                      rule_path.c_str());
    }

    if (result != SBOX_ALL_OK)
      break;

    std::wcerr << L"Rule [Event] added: " << rule.c_str() << std::endl;
  }

  return result;
}

ResultCode SetupNamedPipeRules(scoped_refptr<TargetPolicy> target_policy,
                               const wchar_t* rules) {
  ResultCode result = SBOX_ALL_OK;

  if (rules == nullptr)
    return result;

  std::wstring rules_string(rules);
  if (rules_string.length() == 0)
    return result;

  std::vector<std::wstring> rules_array = SplitString(rules_string, L';');

  for (std::wstring rule : rules_array) {
    result = target_policy->AddRule(TargetPolicy::SubSystem::SUBSYS_NAMED_PIPES,
                                    TargetPolicy::Semantics::NAMEDPIPES_ALLOW_ANY,
                                    rule.c_str());
    if (result != SBOX_ALL_OK)
      break;

    std::wcerr << L"Rule [NamedPipeSystem] added: " << rule.c_str() << std::
        endl;
  }

  return result;
}

bool Initialize() {

  std::wcerr << L"Broker Services initialize." << std::endl;
  BrokerServices* broker_services = SandboxFactory::GetBrokerServices();

  if (broker_services == nullptr)
    return false;

  LoadLibrary(L"userenv");

  return SBOX_ALL_OK == broker_services->Init();
}

int Spawn(const algo::TargetOptions* options,
          algo::TargetInformation* target_information) {
  ResultCode result_code;
  PROCESS_INFORMATION process_information;

  do {
    BrokerServices* broker_services = SandboxFactory::GetBrokerServices();
    if (broker_services == nullptr) {
      result_code = SBOX_ERROR_GENERIC;
      break;
    }

    scoped_refptr<TargetPolicy> target_policy
        = broker_services->CreatePolicy();

    result_code = SetupProtectedMode(target_policy, options->package_name);
    if (result_code != SBOX_ALL_OK) {
      break;
    }

    result_code = SetupFileRules(target_policy, options->fs_rules);
    if (result_code != SBOX_ALL_OK) {
      break;
    }

    result_code = SetupRegistryRules(target_policy, options->reg_rules);
    if (result_code != SBOX_ALL_OK) {
      break;
    }

    result_code = SetupNamedPipeRules(target_policy, options->np_rules);
    if (result_code != SBOX_ALL_OK) {
      break;
    }

    result_code = SetupEventRules(target_policy, options->ev_rules);
    if (result_code != SBOX_ALL_OK) {
      break;
    }

    result_code = SpawnTarget(options->host_path,
                              options->command_line,
                              broker_services, target_policy,
                              &process_information);

    if (result_code != SBOX_ALL_OK) {
      break;
    }
  }
  while (false);

  if (result_code == SBOX_ALL_OK) {
    target_information->process_handle = process_information.hProcess;
    target_information->thread_handle = process_information.hThread;
    target_information->process_id = process_information.dwProcessId;
    target_information->thread_id = process_information.dwThreadId;
  }

  return result_code;
}

bool Resume(const algo::TargetInformation* target_information) {
  return ResumeThread(target_information->thread_handle) >= 0;
}

void WaitAll() {
  BrokerServices* broker_services =
      SandboxFactory::GetBrokerServices();

  if (broker_services) {
    broker_services->WaitForAllTargets();
  }
}
