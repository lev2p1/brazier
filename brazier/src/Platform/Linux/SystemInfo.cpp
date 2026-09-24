#include "brazier/Platform/SystemInfo.hpp"

#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <thread>

namespace brazier::platform {

    int get_fd_limit() noexcept {
        struct rlimit rl;
        if (::getrlimit(RLIMIT_NOFILE, &rl) != 0) {
            return 1024;
        }
        if (rl.rlim_cur == RLIM_INFINITY) {
            return (rl.rlim_max == RLIM_INFINITY)
                ? 65536
                : static_cast<int>(rl.rlim_max);
        }
        return static_cast<int>(rl.rlim_cur);
    }

    int get_system_memory_mb() noexcept {
        struct sysinfo si;
        if (::sysinfo(&si) == 0) {
            return static_cast<int>(
                (static_cast<unsigned long long>(si.totalram) * si.mem_unit) /
                (1024 * 1024));
        }
        return 1024;
    }

    int get_worker_count() noexcept {
        const int n = static_cast<int>(std::thread::hardware_concurrency());
        return (n > 0) ? n : 1;
    }

    int get_thread_count() noexcept {
        const int n = static_cast<int>(std::thread::hardware_concurrency());
        return (n > 0) ? n : 1;
    }

}