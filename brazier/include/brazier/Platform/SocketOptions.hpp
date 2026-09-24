#pragma once

#include "NativeSocket.hpp"

namespace brazier::platform {

    bool set_reuse_port(NativeSocket fd) noexcept;
    bool set_defer_accept(NativeSocket fd, int seconds) noexcept;

    bool has_reuse_port() noexcept;
    bool has_defer_accept() noexcept;

}