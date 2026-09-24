#pragma once

namespace brazier::platform {

    int get_fd_limit() noexcept;

    int get_system_memory_mb() noexcept;

    int get_worker_count() noexcept;

    int get_thread_count() noexcept;

    bool has_reuse_port() noexcept; 

    int get_io_context_count() noexcept;

}