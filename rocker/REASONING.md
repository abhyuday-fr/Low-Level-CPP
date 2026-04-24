# Root Cause Analysis: System Freeze in Rocker

The system freeze you experienced was caused by a "Mount Propagation" issue combined with a path error and a lack of error handling.

## 1. Shared Mount Propagation (The "Killer" Bug)
On modern Linux distributions (like Fedora), the root filesystem is mounted as **shared** by default. 
- When you use `CLONE_NEWNS` to create a new mount namespace, it inherits the "shared" state.
- In a shared namespace, any mount operation performed inside the container **propagates back to the host**.
- When your code called `mount("proc", "proc", "proc", 0, "")`, it actually mounted a new instance of `/proc` (which only shows the container's processes) over your **host's** `/proc` directory.
- This immediately hid all host processes from the kernel/system services, leading to a total system freeze.

## 2. Incorrect Path & Silent Failure
The code attempted to `chroot` into:
`/home/abby/c-pluh-pluh/Low-Level_CPP/rocker/ubuntu_fs`
However, the directory name on disk uses a hyphen (`Low-Level-CPP`) and you mentioned the actual `ubuntu_fs` is in your home directory (`/home/abby/ubuntu_fs`).
- Because the `chroot` call failed silently (no error check), the process continued running in the host's root directory.
- The subsequent `mount("proc", ...)` then targeted the host's actual `/proc`.

## 3. Recommended Fixes Applied:
1. **Private Mounts**: Added `mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL);`. This ensures that any mounts made inside the container stay inside the container.
2. **Error Handling**: Every syscall now checks for `-1` and uses `perror()` to report issues.
3. **Correct Path**: Updated the `chroot` path to `/home/abby/ubuntu_fs`.
4. **Cleanup**: Added a `umount` call (though `execvp` usually makes cleanup moot if it succeeds, it's good practice for failures).
