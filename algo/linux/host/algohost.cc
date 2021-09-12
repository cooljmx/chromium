#include <unistd.h>
#include <iostream>
#include <ostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "sandbox/linux/services/namespace_sandbox.h"
#include "sandbox/linux/syscall_broker/broker_process.h"
#include "sandbox/policy/linux/sandbox_linux.h"

void check_status(const int value,
                  const sandbox::policy::SandboxLinux::Status status,
                  const char* message) {
  if ((value & status) == status)
    std::cout << "Status: " << message << std::endl;
}

void process_status(const int status) {
  std::cout << "Status: " << status << std::endl;

  check_status(status, sandbox::policy::SandboxLinux::Status::kInvalid,
               "Invalid sandbox");
  check_status(status, sandbox::policy::SandboxLinux::Status::kNetNS,
               "Sandbox is using a new network namespace");
  check_status(status, sandbox::policy::SandboxLinux::Status::kPIDNS,
               "Sandbox is using a new PID namespace");
  check_status(status, sandbox::policy::SandboxLinux::Status::kSUID,
               "SUID sandbox active");
  check_status(status, sandbox::policy::SandboxLinux::Status::kSeccompBPF,
               "seccomp-bpf sandbox active");
  check_status(status, sandbox::policy::SandboxLinux::Status::kSeccompTSYNC,
               "seccomp-bpf sandbox is active and the kernel supports TSYNC");
  check_status(status, sandbox::policy::SandboxLinux::Status::kUserNS,
               "User namespace sandbox active");
  check_status(status, sandbox::policy::SandboxLinux::Status::kYama,
               "The Yama LSM module is present and enforcing");
}

using sandbox::syscall_broker::BrokerFilePermission;
using sandbox::syscall_broker::MakeBrokerCommandSet;

void run_external() {
  //execl("~/shared/spotware/ubuntu/net-target-process/net-target-process/bin/Debug/net5.0/net-target-process","");

}

static bool StartBrokerProcessHook(
    sandbox::policy::SandboxLinux::Options options) {
  std::cout << "In StartBrokerProcessHook" << std::endl;

  auto* instance = sandbox::policy::SandboxLinux::GetInstance();

  const int status = instance->GetStatus();

  process_status(status);

  if (instance->seccomp_bpf_started()) {
    std::cout << "seccomp_bpf started" << std::endl;
  } else {
    std::cout << "seccomp_bpf not started" << std::endl;
  }

  return true;
}

static bool InitializeSandboxHook(
    sandbox::policy::SandboxLinux::Options options) {
  std::cout << "In InitializeSandboxHook" << std::endl;

  auto* instance = sandbox::policy::SandboxLinux::GetInstance();

  const int status = instance->GetStatus();

  process_status(status);

  return true;
}


void prepare_sandbox(int argc, char** argv) {
  new base::AtExitManager();

  auto* instance = sandbox::policy::SandboxLinux::GetInstance();

  //std::cout << "InNewPidNamespace: " <<
  //    sandbox::NamespaceSandbox::InNewPidNamespace() << std::endl;
  //std::cout << "InNewUserNamespace: " <<
  //    sandbox::NamespaceSandbox::InNewUserNamespace() << std::endl;
  //std::cout << "InNewNetNamespace: " <<
  //    sandbox::NamespaceSandbox::InNewNetNamespace() << std::endl;

  if (base::CommandLine::Init(argc, argv)) {
    instance->PreinitializeSandbox();

    const auto options = sandbox::policy::SandboxLinux::Options();

    instance->StartBrokerProcess(
        MakeBrokerCommandSet({
            sandbox::syscall_broker::COMMAND_ACCESS,
            sandbox::syscall_broker::COMMAND_MKDIR,
            sandbox::syscall_broker::COMMAND_OPEN,
            sandbox::syscall_broker::COMMAND_READLINK,
            sandbox::syscall_broker::COMMAND_RENAME,
            sandbox::syscall_broker::COMMAND_RMDIR,
            sandbox::syscall_broker::COMMAND_STAT,
            sandbox::syscall_broker::COMMAND_UNLINK,
        }),
        {
            BrokerFilePermission::ReadWriteCreateRecursive("/"),
        },
        base::BindOnce(StartBrokerProcessHook),
        options);

    if (instance->EngageNamespaceSandboxIfPossible()) {
      std::cout << "Namespace sandbox engaged" << std::endl;
    } else {
      std::cout << "Namespace sandbox not engaged" << std::endl;
    }

    if (instance->InitializeSandbox(
        sandbox::policy::SandboxType::kZygoteIntermediateSandbox,
        base::BindOnce(InitializeSandboxHook),
        options)) {
      std::cout << "Sandbox initialized" << std::endl;
    } else {
      std::cout << "Sandbox not initialized" << std::endl;
    }

    //const int status = instance->GetStatus();

    //process_status(status);
  }
}

int main(int argc, char** argv) {
  std::cout << "AlgoHost started" << std::endl;
  //pid_t pid = fork();

  //if (pid == 0) {
  //  printf("I am the child.\n");
  prepare_sandbox(argc, argv);
  //int res = execl(
  //    "/home/cooljmx/shared/spotware/ubuntu/net-target-process/net-target-process/bin/Debug/net5.0/net-target-process",
  //    "");
  int res = system("/bin/ls");
  //int res = execl(
  //    "ls",
  //    "");

  printf("Res %d\n", res);
  if (res != 0)
    printf("Error %d \"%s\"\n", errno, strerror(errno));

  //}
  //if (pid > 0) {
  //  printf("I am the parent, the child is %d.\n", pid);
  //  int status;
  //  waitpid(pid, &status, 0);
  //  printf("parent done\n");
  //}
  //if (pid < 0) {
  //  perror("In fork():");
  //}
  std::cout << "AlgoHost finished" << std::endl;
}
