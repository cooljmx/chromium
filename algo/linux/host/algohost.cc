#include <filesystem>
#include <functional>
#include <iostream>

#include <dirent.h>
#include <errno.h>

#include <libgen.h>
#include <limits.h>
#include <nethost.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/syscall.h>

#include "algo/linux/host/native_host/nativehost.h"
#include "base/files/file_util.h"
#include "sandbox/linux/services/credentials.h"
#include "sandbox/linux/services/namespace_sandbox.h"
#include "sandbox/policy/linux/sandbox_linux.h"

using sandbox::syscall_broker::BrokerFilePermission;
using sandbox::syscall_broker::MakeBrokerCommandSet;

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

static bool InitializeSandboxHook(sandbox::policy::SandboxLinux::Options options) {
    std::cout << "In InitializeSandboxHook" << std::endl;
    auto* instance = sandbox::policy::SandboxLinux::GetInstance();
    const int status = instance->GetStatus();

    process_status(status);

    return true;
}

bool copy_lib(const char* lib_path, const base::FilePath lib_dir) {
    char library[PATH_MAX];
    char library_link[PATH_MAX];
    const auto *gnu_lib_dir = "/usr/lib/x86_64-linux-gnu/";
    const auto *library_link_base = lib_path;

    strcpy(library_link, gnu_lib_dir);
    strcat(library_link, library_link_base);

    strcpy(library, gnu_lib_dir);
    const size_t library_dirname_len = strlen(library);

    ssize_t bytes_written;
    if ((bytes_written = readlink(library_link, &library[library_dirname_len], PATH_MAX)) == -1) {
        fprintf(stderr, "unable to read library path: %m\n");
        return false;
    }
    const size_t written_total = bytes_written + library_dirname_len;
    if (written_total >= PATH_MAX) {
        fprintf(stderr, "library path is too long\n");
        return false;
    }
    library[written_total] = '\0';

    const auto library_src =  base::FilePath(library);
    const auto library_dest = lib_dir.Append(&library[library_dirname_len]);
    std::cout << "copying " << library_src << " to " << library_dest << std::endl;
    if (!base::CopyFile(library_src, library_dest)) {
        fprintf(stderr, "unable to copy library to dest path\n");
        return false;
    }
    return true;
}

bool setup_syscall_filter() {
    auto* instance = sandbox::policy::SandboxLinux::GetInstance();
    instance->PreinitializeSandbox();

    auto options = sandbox::policy::SandboxLinux::Options();
    // options.allow_threads_during_sandbox_init = true;
    // options.check_for_open_directories = false;

    if (instance->InitializeSandbox(sandbox::policy::SandboxType::kUtility,
                                    base::BindOnce(InitializeSandboxHook), options)) {
        std::cout << "Sandbox initialized" << std::endl;
    } else {
        std::cout << "Sandbox not initialized" << std::endl;
        return false;
    }
    return true;
}

int prepare_sandbox(int argc, char** argv) {
    new base::AtExitManager();

    if (!base::CommandLine::Init(argc, argv)) {
        return EXIT_FAILURE;
    }

    char host_fxr_path[PATH_MAX];
    size_t host_fxr_path_size = sizeof(host_fxr_path) / sizeof(char);
    int rc = get_hostfxr_path(host_fxr_path, &host_fxr_path_size, nullptr);
    if (rc) {
        fprintf(stderr, "unable to get hostfxr path\n");
        return EXIT_FAILURE;
    }

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

    const char box[] = "/mnt/box";
    char out_algo[PATH_MAX];

    ssize_t bytes_written;
    if ((bytes_written = readlink("/proc/self/exe", out_algo, PATH_MAX)) == -1) {
        fprintf(stderr, "unable to read exe's path: %m\n");
        return EXIT_FAILURE;
    }
    if (++bytes_written > PATH_MAX) {
        fprintf(stderr, "exe's path is too long\n");
        return EXIT_FAILURE;
    }
    out_algo[bytes_written] = '\0';

    if (!strcmp(dirname(out_algo), ".")) {
        fprintf(stderr, "exe's path is not valid: %s\n", out_algo);
        return EXIT_FAILURE;
    }

    char home[PATH_MAX];
    if (strcpy(home, getenv("HOME")) == NULL) {
        fprintf(stderr, "unable to get HOME dir\n");
        return EXIT_FAILURE;
    }

    const auto new_root_path = base::FilePath(home).Append(&box[1]);
    const auto old_root_path = new_root_path.Append("old_root");
    const auto *new_root = new_root_path.value().c_str();
    const auto *old_root = old_root_path.value().c_str();
    if (!base::CreateDirectory(new_root_path)) {
        fprintf(stderr, "unable to create a ~/mnt/box dir\n");
        return EXIT_FAILURE;
    }
    if (!base::CreateDirectory(old_root_path)) {
        fprintf(stderr, "unable to create a ~/mnt/box/old_root dir\n");
        return EXIT_FAILURE;
    }

    char host_fxr_base[PATH_MAX];
    strcpy(host_fxr_base, host_fxr_path);
    const char *fxr_base = basename(host_fxr_base);
    const auto lib_dir = new_root_path.Append("lib");
    const auto dest_path = lib_dir.Append(fxr_base);

    if (!base::CreateDirectory(lib_dir)) {
        fprintf(stderr, "unable to create a lib directory\n");
        return EXIT_FAILURE;
    }

//  Disabled for now
//  if (!copy_lib("libicuuc.so", lib_dir)) {
//      return EXIT_FAILURE;
//  }
//  if (!copy_lib("libicudata.so", lib_dir)) {
//      return EXIT_FAILURE;
//  }

    std::cout << "copying " << host_fxr_path << " to " << dest_path << std::endl;
    if (!base::CopyFile(base::FilePath(host_fxr_path), dest_path)) {
        fprintf(stderr, "unable to copy hostfxr lib to dest path\n");
        return EXIT_FAILURE;
    }

    const auto *dotnet = "/usr/share/dotnet/shared";
    std::cout << "copying " << dotnet << " to " << new_root << std::endl;
    if (!base::CopyDirectory(base::FilePath(dotnet), new_root_path, true)) {
        fprintf(stderr, "unable to copy dotnet to new root\n");
        return EXIT_FAILURE;
    }

    std::cout << "copying " << out_algo << " to " << new_root << std::endl;
    if (!base::CopyDirectory(base::FilePath(out_algo), new_root_path, true)) {
        fprintf(stderr, "unable to copy the current dir to new root\n");
        return EXIT_FAILURE;
    }

    char dir_path[255];
    const auto dir_path_obj = new_root_path.Append("proc");
    strcpy(dir_path, dir_path_obj.value().c_str());

    DIR *dir = opendir(dir_path);
    if (dir) {
        closedir(dir);
    } else if (ENOENT == errno) {
        if (mkdir(dir_path, 0777) == -1) {
            fprintf(stderr, "unable to create proc dir: %m\n");
            return EXIT_FAILURE;
        }
    } else {
        fprintf(stderr, "unable to open a proc directory: %m\n");
        return EXIT_FAILURE;
    }
    strcat(dir_path, "/self");
    mkdir(dir_path, 0777);

    char mmaps[PATH_MAX];
    strcpy(mmaps, dir_path);
    strcat(mmaps, "/maps");
    creat(mmaps, 0777);
    if (mount("/proc/self/maps", mmaps, "bind", MS_BIND, "") == -1) {
        fprintf(stderr, "unable to mount maps: %m\n");
        return EXIT_FAILURE;
    }

    char task[PATH_MAX];
    strcpy(task, dir_path);
    strcat(task, "/task");
    mkdir(task, 0777);
    strcat(task, "/dummy");
    mkdir(task, 0777);

    char fd_dir[PATH_MAX];
    strcpy(fd_dir, dir_path);
    strcat(fd_dir, "/fd");
    mkdir(fd_dir, 0777);

    strcat(dir_path, "/exe");
    symlink("/algo/algohost.netcore", dir_path);
    if (mount("", "/", "", MS_PRIVATE | MS_REC, "") == -1) {
        fprintf(stderr, "unable to make current root private: %m\n");
        return EXIT_FAILURE;
    }
    if (mount(new_root, new_root, "bind", MS_BIND | MS_REC, "") == -1) {
        fprintf(stderr, "unable to turn new root into mountpoint: %m\n");
        return EXIT_FAILURE;
    }
    if (syscall(__NR_pivot_root, new_root, old_root) == -1) {
        fprintf(stderr, "unable to pivot root to %s: %m\n", new_root);
        return EXIT_FAILURE;
    }
    if (chdir("/") == -1) {
        fprintf(stderr, "unable to change dir to /: %m\n");
        return EXIT_FAILURE;
    }
    if (chroot("/") == -1) {
        fprintf(stderr, "unable to chroot: %m\n");
        return EXIT_FAILURE;
    }
    if (umount2("old_root", MNT_DETACH) == -1) {
        fprintf(stderr, "unable to umount old root: %m\n");
        return EXIT_FAILURE;
    }

    const auto *out_pipe = "/out_pipe";
    const auto *in_pipe = "/in_pipe";
    if (access(out_pipe, F_OK)) {
        if (mkfifo(out_pipe, 0600) == -1) {
            fprintf(stderr, "unable to create out_pipe: %m\n");
            return EXIT_FAILURE;
        }
    }
    if (access(in_pipe, F_OK)) {
        if (mkfifo(in_pipe, 0600) == -1) {
            fprintf(stderr, "unable to create in_pipe: %m\n");
            return EXIT_FAILURE;
        }
    }
    if (mount("/", "/", "bind", MS_REMOUNT | MS_BIND | MS_REC | MS_RDONLY, "") == -1) {
        fprintf(stderr, "unable to turn new root into readonly mountpoint: %m\n");
        return EXIT_FAILURE;
    }
    if (mount(out_pipe, out_pipe, "bind", MS_BIND, "") == -1) {
        fprintf(stderr, "unable to turn out_pipe into writable mountpoint: %m\n");
        return EXIT_FAILURE;
    }

    PCHECK(sandbox::Credentials::DropAllCapabilitiesOnCurrentThread());

    assert(!access("algo/algohost.netcore", F_OK));
    assert(access("/home", F_OK));
    assert(access("/usr/bin/bash", F_OK));

    return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
    std::cout << "AlgoHost started" << std::endl;

    int res;
    res = prepare_sandbox(argc, argv);
    if (res == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }

    const char *config = "algo/testing_app/DotNetLib.runtimeconfig.json";
    const char *dotnet_path = "algo/testing_app/testing_app.dll";
    const char *dotnet_type = "testing_app.Program, testing_app";
    const char *dotnet_type_method = "ReverseLine";

    component_entry_point_fn entry_fn;
    entry_fn = launch_dotnet(dotnet_path, dotnet_type, dotnet_type_method, config);

    struct lib_args
    {
        const char *message;
        int number;
    };

    lib_args args
    {
        "from host!",
        1
    };

    if (!setup_syscall_filter()) {
        fprintf(stderr, "unable to insert a syscall filter");
        return EXIT_FAILURE;
    }

    entry_fn(&args, sizeof(args));

    auto* instance = sandbox::policy::SandboxLinux::GetInstance();
    if (instance->seccomp_bpf_started()) {
        std::cout << "seccomp_bpf started" << std::endl;
    } else {
        std::cout << "seccomp_bpf not started" << std::endl;
    }

    std::cout << "AlgoHost finished" << std::endl;
}
