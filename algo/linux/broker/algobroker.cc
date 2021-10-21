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
