#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sched.h>
#include <signal.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

void rule_set(pid_t child_pid) {
    // Attempting to support both Cgroup v1 and v2 is complex in a simple script.
    // Modern systems (Fedora, Ubuntu 22.04+) use Cgroup v2 at /sys/fs/cgroup.
    
    std::filesystem::path cgroup_base = "/sys/fs/cgroup/knocker";
    
    try {
        if (!std::filesystem::exists(cgroup_base)) {
            if (!std::filesystem::create_directory(cgroup_base)) {
                perror("cgroup create_directory failed");
                return;
            }
        }

        // Cgroup v2 logic
        std::ofstream procs(cgroup_base / "cgroup.procs");
        if (procs.is_open()) {
            procs << std::to_string(child_pid);
            procs.close();
        } else {
            perror("Failed to open cgroup.procs (v2)");
        }

        // PIDs limit (v2)
        std::ofstream pids_max(cgroup_base / "pids.max");
        if (pids_max.is_open()) {
            pids_max << "20"; // Increased slightly to allow basic commands
            pids_max.close();
        }

        // Memory limit (v2)
        std::ofstream mem_max(cgroup_base / "memory.max");
        if (mem_max.is_open()) {
            mem_max << "200M"; 
            mem_max.close();
        }
    } catch (const std::exception& e) {
        std::cerr << "Cgroup setup error: " << e.what() << std::endl;
    }
}

int runtime(void *args) {
    std::vector<char *> *arg_ptr = (std::vector<char *> *)args;
    
    // 1. Make the mount namespace private to prevent host freeze
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) == -1) {
        perror("mount private");
        return 1;
    }

    // 2. Set Hostname
    std::string hostName = "Knocker-Host";
    if (sethostname(hostName.c_str(), hostName.length()) == -1) {
        perror("sethostname");
        return 1;
    }

    // 3. Chroot
    // Note: Ensure this directory exists on your host!
    const char* rootfs = "/home/abby/ubuntu_fs";
    if (chroot(rootfs) == -1) {
        perror("chroot failed (Does /home/abby/ubuntu_fs exist?)");
        return 1;
    }
    
    if (chdir("/") == -1) {
        perror("chdir");
        return 1;
    }

    // 4. Mount Proc
    if (mount("proc", "proc", "proc", 0, "") == -1) {
        perror("mount proc");
        return 1;
    }

    // 5. Execute the command
    // We replace the current process with the target command.
    // No need to fork here because clone already created a new process.
    if (execvp((*arg_ptr)[0], (*arg_ptr).data()) == -1) {
        perror("execvp");
        // Cleanup if exec fails
        umount("proc");
        return 1;
    }

    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <command> [args...]" << std::endl;
        return EXIT_FAILURE;
    }

    std::vector<std::string> args(argv + 1, argv + argc);
    std::vector<char *> run_arg;
    for (auto &str : args) {
        run_arg.push_back(const_cast<char *>(str.c_str()));
    }
    run_arg.push_back(nullptr);

    // Allocate stack for clone
    const int STACK_SIZE = 1024 * 1024;
    char *stack = (char *)malloc(STACK_SIZE);
    if (!stack) {
        perror("malloc stack");
        return EXIT_FAILURE;
    }
    char *stackTop = stack + STACK_SIZE;

    // Clone flags:
    // CLONE_NEWUTS: Isolate hostname
    // CLONE_NEWPID: Isolate process IDs
    // CLONE_NEWNS:  Isolate mount points
    int flags = SIGCHLD | CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNS;
    
    pid_t child_process_id = clone(runtime, stackTop, flags, (void *)&run_arg);
    
    if (child_process_id == -1) {
        perror("clone");
        free(stack);
        return EXIT_FAILURE;
    }

    // Apply resource limits
    rule_set(child_process_id);

    // Wait for the container to finish
    if (waitpid(child_process_id, nullptr, 0) == -1) {
        perror("waitpid");
    }

    free(stack);
    std::cout << "Container finished and cleaned up." << std::endl;
    return EXIT_SUCCESS;
}
