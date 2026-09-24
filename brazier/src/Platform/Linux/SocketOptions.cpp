#include "brazier/Platform/SocketOptions.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

namespace brazier::platform {

    bool set_reuse_port(NativeSocket fd) noexcept {
        int one = 1;
        return ::setsockopt(static_cast<int>(fd), SOL_SOCKET, SO_REUSEPORT,
            &one, sizeof(one)) == 0;
    }

    bool set_defer_accept(NativeSocket fd, int seconds) noexcept {
        return ::setsockopt(static_cast<int>(fd), IPPROTO_TCP, TCP_DEFER_ACCEPT,
            &seconds, sizeof(seconds)) == 0;
    }

    bool has_reuse_port() noexcept { return true; }
    bool has_defer_accept() noexcept { return true; }

}