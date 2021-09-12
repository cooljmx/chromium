#include "algo/linux/broker/algobroker.h"

#include <iostream>
#include <signal.h>

#include "base/at_exit.h"
#include "sandbox/linux/services/namespace_sandbox.h"
#include "sandbox/policy/sandbox.h"
#include "sandbox/policy/sandbox_type.h"
#include "sandbox/policy/linux/sandbox_linux.h"


using sandbox::syscall_broker::BrokerFilePermission;
using sandbox::syscall_broker::MakeBrokerCommandSet;

bool callback(sandbox::policy::SandboxLinux::Options options) {
    return true;
}

int main(int argc, char** argv) {
  std::cout << "AlgoBroker started" << std::endl;

  new base::AtExitManager();

  if (base::CommandLine::Init(argc, argv)) {
    //constexpr auto sandbox_type = sandbox::policy::SandboxType::kNetwork;
    //sandbox::policy::SandboxLinux::PreSandboxHook pre_sandbox_hook =
    //    base::BindOnce(&NetworkPreSandboxHook);

    //auto* instance = sandbox::policy::SandboxLinux::GetInstance();
    //auto options = sandbox::policy::SandboxLinux::Options();

    //instance->InitializeSandbox(
    //    sandbox_type,
    //    std::move(pre_sandbox_hook),
    //    options);

    //instance->PreinitializeSandbox();

    //const auto options = sandbox::policy::SandboxLinux::Options();

    //auto broker_side_hook = sandbox::policy::SandboxLinux::PreSandboxHook();

    //std::vector<BrokerFilePermission> permissions = {};
    //permissions.push_back(BrokerFilePermission::ReadOnlyRecursive("/"));

    //instance->StartBrokerProcess(
    //    MakeBrokerCommandSet({
    //        sandbox::syscall_broker::COMMAND_ACCESS,
    //        sandbox::syscall_broker::COMMAND_MKDIR,
    //        sandbox::syscall_broker::COMMAND_OPEN,
    //        sandbox::syscall_broker::COMMAND_READLINK,
    //        sandbox::syscall_broker::COMMAND_RENAME,
    //        sandbox::syscall_broker::COMMAND_RMDIR,
    //        sandbox::syscall_broker::COMMAND_STAT,
    //        sandbox::syscall_broker::COMMAND_UNLINK,
    //    }),
    //    permissions,
    //    std::move(broker_side_hook),
    //    options);

    const std::string path = argv[1];
    const auto file_path = base::FilePath(path);
    const auto command_line = base::CommandLine(file_path);

    base::LaunchOptions launch_options;
    launch_options.wait = true;

    sandbox::NamespaceSandbox::LaunchProcess(command_line, launch_options);
  }

  std::cout << "AlgoBroker finished" << std::endl;
  return 0;
}
