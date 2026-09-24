#include "brazier/Platform/SystemInfo.hpp"

#include <sys/resource.h>
#include <sys/sysctl.h>
#include <sys/types.h>
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
        int mib[2] = { CTL_HW, HW_MEMSIZE };
        uint64_t memsize = 0;
        size_t len = sizeof(memsize);
        if (::sysctl(mib, 2, &memsize, &len, nullptr, 0) == 0) {
            return static_cast<int>(memsize / (1024 * 1024));
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