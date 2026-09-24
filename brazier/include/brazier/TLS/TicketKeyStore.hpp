#pragma once

#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/opensslv.h>

#if OPENSSL_VERSION_NUMBER < 0x30000000L
#  include <openssl/hmac.h>
#endif

#include <chrono>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <vector>

namespace brazier {

    class TicketKeyStore {
    public:
        using Clock = std::chrono::steady_clock;
        using TimePoint = Clock::time_point;

        static constexpr auto kRotationInterval = std::chrono::hours(12);
        static constexpr auto kKeyLifetime = std::chrono::hours(48);

        static constexpr std::size_t kNameSize = 16;
        static constexpr std::size_t kAesKeySize = 32;
        static constexpr std::size_t kHmacKeySize = 32;

        struct Key {
            unsigned char name[kNameSize];
            unsigned char aes_key[kAesKeySize];
            unsigned char hmac_key[kHmacKeySize];
            TimePoint created_at;
        };

        TicketKeyStore();
        explicit TicketKeyStore(std::function<TimePoint()> now_provider);

        TicketKeyStore(const TicketKeyStore&) = delete;
        TicketKeyStore& operator=(const TicketKeyStore&) = delete;

        int handle(unsigned char key_name[kNameSize],
            unsigned char iv[EVP_MAX_IV_LENGTH],
            EVP_CIPHER_CTX* cipher_ctx,
            void* mac_ctx,
            int enc);

        void ensure_initialized();
        void rotate_now();

        void attach_to(SSL_CTX* ctx);

        std::size_t key_count() const;
        std::vector<Key> snapshot() const;

    private:
        static Key generate_key(TimePoint now);
        void maybe_rotate_locked(TimePoint now);

        mutable std::mutex mtx_;
        std::deque<Key>    keys_;
        std::function<TimePoint()> now_provider_;
    };

    int ticket_store_ex_index();

} 