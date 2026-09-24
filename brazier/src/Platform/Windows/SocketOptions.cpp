#include "brazier/Platform/SocketOptions.hpp"

#include <winsock2.h>

namespace brazier::platform {

    bool set_reuse_port(NativeSocket /*fd*/) noexcept { return false; }
    bool set_defer_accept(NativeSocket /*fd*/, int /*seconds*/) noexcept { return false; }

    bool has_reuse_port() noexcept { return false; }
    bool has_defer_accept() noexcept { return false; }

}