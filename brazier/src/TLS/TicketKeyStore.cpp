#include "../../include/brazier/TLS/TicketKeyStore.hpp"

#include <openssl/rand.h>

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#  include <openssl/core_names.h>
#  include <openssl/params.h>
#endif

#include <cstring>
#include <mutex>
#include <stdexcept>

namespace {

    std::once_flag g_ex_index_flag;
    int            g_ex_index = -1;

#if OPENSSL_VERSION_NUMBER >= 0x30000000L

    bool init_hmac(EVP_MAC_CTX* hctx, const unsigned char* key, std::size_t keylen) {
        char digest_name[] = "SHA256";
        OSSL_PARAM params[] = {
            OSSL_PARAM_construct_utf8_string(OSSL_MAC_PARAM_DIGEST,
                                             digest_name, 0),
            OSSL_PARAM_construct_end()
        };
        return EVP_MAC_init(hctx, key, keylen, params) == 1;
    }

    int ticket_key_cb(SSL* ssl,
        unsigned char key_name[16],
        unsigned char iv[EVP_MAX_IV_LENGTH],
        EVP_CIPHER_CTX* cipher_ctx,
        EVP_MAC_CTX* mac_ctx,
        int enc) {
        SSL_CTX* ssl_ctx = SSL_get_SSL_CTX(ssl);
        if (!ssl_ctx) return 0;

        auto* store = static_cast<brazier::TicketKeyStore*>(
            SSL_CTX_get_ex_data(ssl_ctx, brazier::ticket_store_ex_index()));
        if (!store) return 0;

        return store->handle(key_name, iv, cipher_ctx, mac_ctx, enc);
    }

#else

    bool init_hmac(HMAC_CTX* hctx, const unsigned char* key, int keylen) {
        return HMAC_Init_ex(hctx, key, keylen, EVP_sha256(), nullptr) == 1;
    }

    int ticket_key_cb(SSL* ssl,
        unsigned char key_name[16],
        unsigned char iv[EVP_MAX_IV_LENGTH],
        EVP_CIPHER_CTX* cipher_ctx,
        HMAC_CTX* mac_ctx,
        int enc) {
        SSL_CTX* ssl_ctx = SSL_get_SSL_CTX(ssl);
        if (!ssl_ctx) return 0;

        auto* store = static_cast<brazier::TicketKeyStore*>(
            SSL_CTX_get_ex_data(ssl_ctx, brazier::ticket_store_ex_index()));
        if (!store) return 0;

        return store->handle(key_name, iv, cipher_ctx, mac_ctx, enc);
    }

#endif

} 

namespace brazier {

    TicketKeyStore::TicketKeyStore()
        : now_provider_([] { return Clock::now(); }) {}

    TicketKeyStore::TicketKeyStore(std::function<TimePoint()> now_provider)
        : now_provider_(std::move(now_provider)) {
        if (!now_provider_) {
            throw std::invalid_argument("TicketKeyStore: now_provider is null");
        }
    }

    TicketKeyStore::Key TicketKeyStore::generate_key(TimePoint now) {
        Key k{};
        if (RAND_bytes(k.name, kNameSize) != 1 ||
            RAND_bytes(k.aes_key, kAesKeySize) != 1 ||
            RAND_bytes(k.hmac_key, kHmacKeySize) != 1) {
            throw std::runtime_error("TicketKeyStore: RAND_bytes failed");
        }
        k.created_at = now;
        return k;
    }

    void TicketKeyStore::ensure_initialized() {
        std::lock_guard<std::mutex> lk(mtx_);
        if (keys_.empty()) {
            keys_.push_back(generate_key(now_provider_()));
        }
    }

    void TicketKeyStore::rotate_now() {
        std::lock_guard<std::mutex> lk(mtx_);
        const auto now = now_provider_();
        keys_.push_front(generate_key(now));
        while (!keys_.empty() &&
            now - keys_.back().created_at > kKeyLifetime) {
            keys_.pop_back();
        }
    }

    void TicketKeyStore::maybe_rotate_locked(TimePoint now) {
        if (keys_.empty()) {
            keys_.push_back(generate_key(now));
            return;
        }
        if (now - keys_.front().created_at < kRotationInterval) {
            return;
        }
        keys_.push_front(generate_key(now));
        while (!keys_.empty() &&
            now - keys_.back().created_at > kKeyLifetime) {
            keys_.pop_back();
        }
    }

    int TicketKeyStore::handle(unsigned char key_name[kNameSize],
        unsigned char iv[EVP_MAX_IV_LENGTH],
        EVP_CIPHER_CTX* cipher_ctx,
        void* mac_ctx,
        int enc) {
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
        auto* hctx = static_cast<EVP_MAC_CTX*>(mac_ctx);
#else
        auto* hctx = static_cast<HMAC_CTX*>(mac_ctx);
#endif

        std::lock_guard<std::mutex> lk(mtx_);

        if (keys_.empty()) {
            keys_.push_back(generate_key(now_provider_()));
        }

        if (enc == 1) {
            maybe_rotate_locked(now_provider_());

            const auto& cur = keys_.front();
            std::memcpy(key_name, cur.name, kNameSize);
            RAND_bytes(iv, EVP_MAX_IV_LENGTH);

            if (EVP_EncryptInit_ex(cipher_ctx, EVP_aes_256_cbc(), nullptr,
                cur.aes_key, iv) != 1) {
                return -1;
            }
            if (!init_hmac(hctx, cur.hmac_key, kHmacKeySize)) {
                return -1;
            }
            return 1;
        }

        for (std::size_t i = 0; i < keys_.size(); ++i) {
            if (std::memcmp(key_name, keys_[i].name, kNameSize) != 0) {
                continue;
            }
            const auto& k = keys_[i];
            if (EVP_DecryptInit_ex(cipher_ctx, EVP_aes_256_cbc(), nullptr,
                k.aes_key, iv) != 1) {
                return -1;
            }
            if (!init_hmac(hctx, k.hmac_key, kHmacKeySize)) {
                return -1;
            }
            return (i == 0) ? 1 : 2;
        }
        return 0;
    }

    void TicketKeyStore::attach_to(SSL_CTX* ctx) {
        if (!ctx) {
            throw std::invalid_argument("TicketKeyStore::attach_to: ctx is null");
        }

        const int idx = ticket_store_ex_index();
        if (idx < 0) {
            throw std::runtime_error(
                "TicketKeyStore::attach_to: ex_data index not available");
        }

        void* existing = SSL_CTX_get_ex_data(ctx, idx);
        if (existing != nullptr && existing != this) {
            throw std::runtime_error(
                "TicketKeyStore::attach_to: SSL_CTX already has a different store");
        }

        SSL_CTX_set_ex_data(ctx, idx, this);

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
        SSL_CTX_set_tlsext_ticket_key_evp_cb(ctx, ticket_key_cb);
#else
        SSL_CTX_set_tlsext_ticket_key_cb(ctx, ticket_key_cb);
#endif
    }

    std::size_t TicketKeyStore::key_count() const {
        std::lock_guard<std::mutex> lk(mtx_);
        return keys_.size();
    }

    std::vector<TicketKeyStore::Key> TicketKeyStore::snapshot() const {
        std::lock_guard<std::mutex> lk(mtx_);
        return { keys_.begin(), keys_.end() };
    }

    int ticket_store_ex_index() {
        std::call_once(g_ex_index_flag, [] {
            g_ex_index = SSL_CTX_get_ex_new_index(
                0, nullptr, nullptr, nullptr, nullptr);
            });
        return g_ex_index;
    }

} 