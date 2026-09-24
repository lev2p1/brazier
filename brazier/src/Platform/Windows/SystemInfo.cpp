#include "brazier/Platform/SystemInfo.hpp"

#include <windows.h>

namespace brazier::platform {

    int get_fd_limit() noexcept {
        return 16384;
    }

    int get_system_memory_mb() noexcept {
        MEMORYSTATUSEX ms{};
        ms.dwLength = sizeof(ms);
        if (::GlobalMemoryStatusEx(&ms)) {
            return static_cast<int>(ms.ullTotalPhys / (1024 * 1024));
        }
        return 1024;
    }

    int get_worker_count() noexcept {
        return 1;
    }

    int get_thread_count() noexcept {
        const int n = static_cast<int>(std::thread::hardware_concurrency());
        return (n > 0) ? n : 1;
    }

}