#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iterator>
#include <sched.h>
#include <signal.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

// function to run in cloned process
int runtime(void *args) {
  // typecast back to the original type and run execvp
  std::vector<char *> *arg = (std::vector<char *> *)args;

  // working with the ubuntu image
  std::string hostName = "Rocker-Host";
  sethostname(hostName.c_str(), hostName.length());
  chroot("/home/abby/c-pluh-pluh/Low-Level_CPP/rocker/ubuntu_fs");
  chdir("/");
  mount("proc", "proc", "proc", 0, "");

  //

  // to execute the command
  execvp((*arg)[0], (*arg).data()); // execvp replaces the current process with
                                    // a new one specified by its arguments.
  return 1;
}

int main(int argc, char **argv) {
  std::vector<std::string> args(argv + 1, argv + argc); // store the arguments
  std::cout << "Running Argument: ";
  std::copy(args.begin(), args.end(),
            std::ostream_iterator<std::string>(std::cout, " "));
  std::cout << "\n";

  std::vector<char *> run_arg;
  for (auto &str : args) {
    run_arg.push_back(
        const_cast<char *>(str.c_str())); // converting the vector of strings in
                                          // C style char* strings
  }
  run_arg.push_back(nullptr);

  // definig the stack memory
  auto *stack = (char *)malloc(sizeof(char) * 1024 * 1024); // 1 MB of storage
  auto *stackTop =
      stack + sizeof(char) * 1024 *
                  1024; // malloc points to the start of the allocated memory

  void *arg = static_cast<void *>(&run_arg);
  pid_t child_process_id = clone(
      runtime, stackTop, SIGCHLD | CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNS,
      arg); // clone syscall is used to add a new process
  // parameters in clone:
  // 1. Function to run in the cloned process
  // 2. Pointer to stack memory
  // 3. Flags/Namespaces (CLONE_NEWUTC provide new hostname, SIGCHILD flag to
  // notify a signal to the child process)
  //    CLONE_NEWPID for clone with new process ID, CLONE_NEWNS for clone new
  //    namespace
  // 4. Argument Pointer to pass to the function
  //

  waitpid(child_process_id, nullptr,
          0); // wait for the spun-up process to complete

  free(stack);

  return EXIT_SUCCESS;
}
