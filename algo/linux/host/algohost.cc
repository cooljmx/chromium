#include <filesystem>
#include <functional>
#include <iostream>
// #include <ostream>

#include <unistd.h>
#include <libgen.h>
#include <stdio.h>
// #include <stdlib.h>
#include <sys/mount.h>
#include <sys/syscall.h>
// #include <sys/types.h>
// #include <sys/wait.h>

#include "algo/linux/host/native_host/nativehost.h"
#include "sandbox/linux/services/credentials.h"
#include "sandbox/linux/services/namespace_sandbox.h"
// #include "sandbox/linux/syscall_broker/broker_process.h"
#include "sandbox/policy/linux/sandbox_linux.h"

using sandbox::syscall_broker::BrokerFilePermission;
using sandbox::syscall_broker::MakeBrokerCommandSet;

namespace fs = std::filesystem;

void CopyRecursive(const fs::path& src, const fs::path& target,
                   const std::function<bool(fs::path)>& predicate);

void check_status(const int value, const sandbox::policy::SandboxLinux::Status status, const char* message) {
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

void run_external() {
    //execl("~/shared/spotware/ubuntu/net-target-process/net-target-process/bin/Debug/net5.0/net-target-process","");
}

//static bool StartBrokerProcessHook(sandbox::policy::SandboxLinux::Options options) {
//    std::cout << "In StartBrokerProcessHook" << std::endl;
//
//    auto* instance = sandbox::policy::SandboxLinux::GetInstance();
//
//    const int status = instance->GetStatus();
//
//    process_status(status);
//
//    if (instance->seccomp_bpf_started()) {
//        std::cout << "seccomp_bpf started" << std::endl;
//    } else {
//        std::cout << "seccomp_bpf not started" << std::endl;
//    }
//
//    return true;
//}
//
//static bool InitializeSandboxHook(sandbox::policy::SandboxLinux::Options options) {
//    std::cout << "In InitializeSandboxHook" << std::endl;
//
//    auto* instance = sandbox::policy::SandboxLinux::GetInstance();
//
//    const int status = instance->GetStatus();
//
//    process_status(status);
//
//    return true;
//}

int prepare_sandbox(int argc, char** argv) {
    new base::AtExitManager();

    auto* instance = sandbox::policy::SandboxLinux::GetInstance();

    std::cout << "InNewPidNamespace: " <<
        sandbox::NamespaceSandbox::InNewPidNamespace() << std::endl;
    std::cout << "InNewUserNamespace: " <<
        sandbox::NamespaceSandbox::InNewUserNamespace() << std::endl;
    std::cout << "InNewNetNamespace: " <<
        sandbox::NamespaceSandbox::InNewNetNamespace() << std::endl;

    if(!sandbox::Credentials::MoveToNewUserNS()) {
        fprintf(stderr, "unable to move to new user namespace: %m\n");
        return EXIT_FAILURE;
    }
    if (unshare(CLONE_NEWNS | CLONE_FILES | CLONE_FS | CLONE_NEWIPC) == -1) {
        fprintf(stderr, "unable to unshare file system: %m\n");
        return EXIT_FAILURE;
    }
    std::vector<sandbox::Credentials::Capability> caps;
    caps.push_back(sandbox::Credentials::Capability::SYS_ADMIN);
    caps.push_back(sandbox::Credentials::Capability::SYS_CHROOT);
    if (!sandbox::Credentials::SetCapabilitiesOnCurrentThread(caps)) {
        fprintf(stderr, "unable to set capabilities: %m\n");
        return EXIT_FAILURE;
    }

    char *old_root = NULL;
    const char box[] = "/mnt/box";
    const char out_pipe[] = "out_pipe";
    const unsigned char byte_size = 255;
    char new_root[byte_size];
    int bytes_written;
    if ((bytes_written = readlink("/proc/self/exe", new_root, byte_size)) == -1) {
        fprintf(stderr, "unable to read exe's path or the path is too long: %m\n");
        return EXIT_FAILURE;
    }
    else {
        if (++bytes_written > byte_size) {
            fprintf(stderr, "exe's path is too long\n");
            return EXIT_FAILURE;
        }
        else {
            new_root[bytes_written] = '\0';
        }
    }
    if (!strcmp(dirname(new_root), ".")) {
        fprintf(stderr, "exe's path is not valid: %s\n", new_root);
        return EXIT_FAILURE;
    }
    if ((strlen(new_root) + strlen(box) + 1) > byte_size) {
        fprintf(stderr, "box path is too long");
        free(old_root);
        return EXIT_FAILURE;
    }
    else {
        strcat(new_root, box);
    }
    if (mkdir("mnt", 0755) == -1) {
        fprintf(stderr, "unable to create mnt dir: %m\n");
        return EXIT_FAILURE;
    }
    if (mkdir("mnt/box", 0755) == -1) {
        fprintf(stderr, "unable to create mnt dir: %m\n");
        return EXIT_FAILURE;
    }
    if (asprintf(&old_root, "%s/old_root_XXXXXX", new_root) == -1) {
        fprintf(stderr, "unable to allocate old_root directory: %m\n");
        return EXIT_FAILURE;
    }
    if (mount("", "/", "", MS_PRIVATE | MS_REC, "") == -1) {
        fprintf(stderr, "unable to make current root private: %m\n");
        return EXIT_FAILURE;
    }
    if (mount(new_root, new_root, "bind", MS_BIND | MS_REC, "") == -1) {
        fprintf(stderr, "unable to turn new root into mountpoint: %m\n");
        return EXIT_FAILURE;
    }
    if (mkdtemp(old_root) == NULL) {
        fprintf(stderr, "unable to create temporary directory for pivot root: %m\n");
        free(old_root);
        return EXIT_FAILURE;
    }
    std::cout << "old_root is " << old_root << std::endl;
    std::cout << "new_root is " << new_root << std::endl;

    if (syscall(__NR_pivot_root, new_root, old_root) == -1) {
        fprintf(stderr, "unable to pivot root to %s: %m\n", new_root);
        rmdir(old_root);
        free(old_root);
        return EXIT_FAILURE;
    }
    if (chdir("/") == -1) {
        fprintf(stderr, "unable to change dir to /: %m\n");
        free(old_root);
        return EXIT_FAILURE;
    }
    if (chroot("/") == -1) {
        fprintf(stderr, "unable to chroot: %m\n");
        free(old_root);
        return EXIT_FAILURE;
    }
    if (mkdir("proc", 0555) == -1) {
        fprintf(stderr, "unable to create proc dir: %m\n");
        free(old_root);
        return EXIT_FAILURE;
    }

    char *p = NULL;
    p = old_root;
    p += strlen(new_root);
    if (umount2(p, MNT_DETACH) == -1) {
        fprintf(stderr, "unable to umount old root: %m\n");
        free(old_root);
        return EXIT_FAILURE;
    }
    if (rmdir(p) == -1) {
        fprintf(stderr, "unable to remove directory for old root: %m\n");
        free(old_root);
        return EXIT_FAILURE;
    }
    if (mkfifo("out_pipe", 0600) == -1) {
        fprintf(stderr, "unable to create a pipe: %m\n");
        free(old_root);
        return EXIT_FAILURE;
    }
    if (mount("/", "/", "bind", MS_REMOUNT | MS_BIND | MS_REC | MS_RDONLY, "") == -1) {
        fprintf(stderr, "unable to turn new root into readonly mountpoint: %m\n");
        free(old_root);
        return EXIT_FAILURE;
    }
    if (mount(out_pipe, out_pipe, "bind", MS_BIND, "") == -1) {
        fprintf(stderr, "unable to turn new root into writable mountpoint: %m\n");
        free(old_root);
        return EXIT_FAILURE;
    }

    PCHECK(sandbox::Credentials::DropAllCapabilitiesOnCurrentThread());

//  std::vector<sandbox::Credentials::Capability> capps;
//  capps.push_back(sandbox::Credentials::Capability::SYS_ADMIN);
//  if (!sandbox::Credentials::SetCapabilitiesOnCurrentThread(capps)) {
//      fprintf(stderr, "unable to set capppabilities: %m\n");
//      return EXIT_FAILURE;
//  }

	const auto root = fs::current_path();
	const auto testing_app = root / "testing_app";
	const auto target = root / "mnt";

	const auto dll_filter = [](const fs::path& p) -> bool { return true };
	CopyRecursive(src, target, dll_filter);

	const auto so_filter = [](const fs::path& p) -> bool
	{
		return p.extension().generic_string().find("so") != std::string::npos
            || p.extension().generic_string().find(".netcore") != std::string::npos;
	};
	CopyRecursive(src, target, so_filter);

	if (base::CommandLine::Init(argc, argv)) {
		instance->PreinitializeSandbox();

        //      int pipe_fd;
        //      if ((pipe_fd = open(out_pipe, O_WRONLY | O_CLOEXEC)) == -1) {
        //          fprintf(stderr, "unable to write to the pipe: %m\n");
        //          return EXIT_FAILURE;
        //      }
        //      if (write(pipe_fd, box, strlen(box) + 1) == -1) {
        //          fprintf(stderr, "unable to write to the pipe: %m\n");
        //          return EXIT_FAILURE;
        //      }
        //      if (open("test", O_WRONLY | O_CLOEXEC) == -1) {
        //          fprintf(stderr, "unable to write to regular file: %m\n");
        //          return EXIT_FAILURE;
        //      }


        //      auto options = sandbox::policy::SandboxLinux::Options();
        //      options.allow_threads_during_sandbox_init = true;
        //      options.check_for_open_directories = false;

        //      instance->StartBrokerProcess(
        //              MakeBrokerCommandSet({
        //                  sandbox::syscall_broker::COMMAND_ACCESS,
        //                  sandbox::syscall_broker::COMMAND_MKDIR,
        //                  sandbox::syscall_broker::COMMAND_OPEN,
        //                  sandbox::syscall_broker::COMMAND_READLINK,
        //                  sandbox::syscall_broker::COMMAND_RENAME,
        //                  sandbox::syscall_broker::COMMAND_RMDIR,
        //                  sandbox::syscall_broker::COMMAND_STAT,
        //                  sandbox::syscall_broker::COMMAND_STAT64,
        //                  sandbox::syscall_broker::COMMAND_UNLINK,
        //                  }),
        //              {
        //                  BrokerFilePermission::ReadWriteCreateRecursive("/"),
        //              },
        //              base::BindOnce(StartBrokerProcessHook),
        //              options);

        //      if (instance->EngageNamespaceSandboxIfPossible()) {
        //          std::cout << "Namespace sandbox engaged" << std::endl;
        //      } else {
        //          std::cout << "Namespace sandbox not engaged" << std::endl;
        //      }

        //      if (instance->InitializeSandbox(
        //                  sandbox::policy::SandboxType::kUtility,
        //                  base::BindOnce(InitializeSandboxHook),
        //                  options)) {
        //          std::cout << "Sandbox initialized" << std::endl;
        //      } else {
        //          std::cout << "Sandbox not initialized" << std::endl;
        //      }
    }
    return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
    std::cout << "AlgoHost started" << std::endl;
    //pid_t pid = fork();

    //if (pid == 0) {
    //   printf("I am the child.\n");
    // system("ip addr");

    int res;

    res = prepare_sandbox(argc, argv);
    if (res == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }

    const char *config = "DotNetLib.runtimeconfig.json"
    const char *dotnet_path = "testing_app.dll";
    const char *dotnet_type = "testing_app.Program, testing_app";
    const char *dotnet_type_method = "DisplayNetworkConfiguration";

    res = launch_dotnet(dotnet_path, dotnet_type, dotnet_type_method, config);

    auto* instance = sandbox::policy::SandboxLinux::GetInstance();
    if (instance->seccomp_bpf_started()) {
        std::cout << "seccomp_bpf started" << std::endl;
    } else {
        std::cout << "seccomp_bpf not started" << std::endl;
    }

    printf("Result is %d\n", res);
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
