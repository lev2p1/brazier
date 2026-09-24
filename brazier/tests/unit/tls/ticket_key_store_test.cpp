#include <gtest/gtest.h>

#include "../../../include/brazier/TLS/TicketKeyStore.hpp"

#include <openssl/evp.h>
#include <openssl/ssl.h>

#if OPENSSL_VERSION_NUMBER < 0x30000000L
#  include <openssl/hmac.h>
#endif

#include <cstring>
#include <stdexcept>

using brazier::TicketKeyStore;
using Clock = TicketKeyStore::Clock;

namespace {

    struct FakeTime {
        Clock::time_point t = Clock::time_point{};
        Clock::time_point operator()() const { return t; }
        void advance(std::chrono::seconds s) { t += s; }
        void advance(std::chrono::hours h) { t += h; }
    };

    struct CipherPair {
        EVP_CIPHER_CTX* cipher = EVP_CIPHER_CTX_new();

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
        EVP_MAC* mac_alg = EVP_MAC_fetch(nullptr, "HMAC", nullptr);
        EVP_MAC_CTX* mac = mac_alg ? EVP_MAC_CTX_new(mac_alg) : nullptr;

        ~CipherPair() {
            EVP_CIPHER_CTX_free(cipher);
            EVP_MAC_CTX_free(mac);
            EVP_MAC_free(mac_alg);
        }
#else
        HMAC_CTX* mac = HMAC_CTX_new();

        ~CipherPair() {
            EVP_CIPHER_CTX_free(cipher);
            HMAC_CTX_free(mac);
        }
#endif

        CipherPair() = default;
        CipherPair(const CipherPair&) = delete;
        CipherPair& operator=(const CipherPair&) = delete;
    };

} 

TEST(TicketKeyStore, InitiallyEmpty) {
    TicketKeyStore store;
    EXPECT_EQ(store.key_count(), 0u);
    EXPECT_TRUE(store.snapshot().empty());
}

TEST(TicketKeyStore, EnsureInitializedCreatesOneKey) {
    TicketKeyStore store;
    store.ensure_initialized();
    ASSERT_EQ(store.key_count(), 1u);
    store.ensure_initialized();
    EXPECT_EQ(store.key_count(), 1u);
}

TEST(TicketKeyStore, EnsureInitializedSetsTimestamp) {
    FakeTime ft;
    TicketKeyStore store([&] { return ft(); });

    ft.advance(std::chrono::hours(100));
    store.ensure_initialized();

    const auto snap = store.snapshot();
    ASSERT_EQ(snap.size(), 1u);
    EXPECT_EQ(snap[0].created_at, ft.t);
}

TEST(TicketKeyStore, EncryptReturnsOne) {
    TicketKeyStore store;
    unsigned char name[16] = {};
    unsigned char iv[EVP_MAX_IV_LENGTH] = {};
    CipherPair cp;

    EXPECT_EQ(store.handle(name, iv, cp.cipher, cp.mac, 1), 1);
}

TEST(TicketKeyStore, EncryptFillsNameAndIv) {
    TicketKeyStore store;
    unsigned char name[16] = {};
    unsigned char iv[EVP_MAX_IV_LENGTH] = {};
    CipherPair cp;

    ASSERT_EQ(store.handle(name, iv, cp.cipher, cp.mac, 1), 1);

    const auto snap = store.snapshot();
    ASSERT_EQ(snap.size(), 1u);
    EXPECT_EQ(std::memcmp(name, snap[0].name, 16), 0);

    bool all_zero = true;
    for (auto b : iv) if (b != 0) { all_zero = false; break; }
    EXPECT_FALSE(all_zero);
}

TEST(TicketKeyStore, DecryptWithCurrentKeyReturnsOne) {
    TicketKeyStore store;
    unsigned char name[16] = {};
    unsigned char iv[EVP_MAX_IV_LENGTH] = {};
    CipherPair cp1;

    ASSERT_EQ(store.handle(name, iv, cp1.cipher, cp1.mac, 1), 1);

    CipherPair cp2;
    EXPECT_EQ(store.handle(name, iv, cp2.cipher, cp2.mac, 0), 1);
}

TEST(TicketKeyStore, DecryptWithUnknownKeyReturnsZero) {
    TicketKeyStore store;
    store.ensure_initialized();

    unsigned char name[16];
    std::memset(name, 0xFF, sizeof(name));
    unsigned char iv[EVP_MAX_IV_LENGTH] = {};
    CipherPair cp;

    EXPECT_EQ(store.handle(name, iv, cp.cipher, cp.mac, 0), 0);
}

TEST(TicketKeyStore, NoRotationBeforeInterval) {
    FakeTime ft;
    TicketKeyStore store([&] { return ft(); });
    store.ensure_initialized();

    unsigned char name[16] = {};
    unsigned char iv[EVP_MAX_IV_LENGTH] = {};

    ft.advance(std::chrono::hours(11));
    CipherPair cp;
    ASSERT_EQ(store.handle(name, iv, cp.cipher, cp.mac, 1), 1);
    EXPECT_EQ(store.key_count(), 1u);
}

TEST(TicketKeyStore, RotationAfterInterval) {
    FakeTime ft;
    TicketKeyStore store([&] { return ft(); });
    store.ensure_initialized();

    unsigned char first_raw[16];
    std::memcpy(first_raw, store.snapshot()[0].name, 16);

    ft.advance(std::chrono::hours(13));

    unsigned char name[16] = {};
    unsigned char iv[EVP_MAX_IV_LENGTH] = {};
    CipherPair cp;
    ASSERT_EQ(store.handle(name, iv, cp.cipher, cp.mac, 1), 1);

    ASSERT_EQ(store.key_count(), 2u);
    const auto snap = store.snapshot();
    EXPECT_EQ(std::memcmp(name, snap[0].name, 16), 0);
    EXPECT_EQ(std::memcmp(first_raw, snap[1].name, 16), 0);
}

TEST(TicketKeyStore, RotateNowForcesRotation) {
    TicketKeyStore store;
    store.ensure_initialized();
    ASSERT_EQ(store.key_count(), 1u);

    unsigned char old_name[16];
    std::memcpy(old_name, store.snapshot()[0].name, 16);

    store.rotate_now();

    ASSERT_EQ(store.key_count(), 2u);
    const auto snap = store.snapshot();
    EXPECT_NE(std::memcmp(old_name, snap[0].name, 16), 0);
    EXPECT_EQ(std::memcmp(old_name, snap[1].name, 16), 0);
}

TEST(TicketKeyStore, OldKeyReturnsTwo) {
    FakeTime ft;
    TicketKeyStore store([&] { return ft(); });
    store.ensure_initialized();

    unsigned char old_name[16];
    std::memcpy(old_name, store.snapshot()[0].name, 16);

    ft.advance(std::chrono::hours(13));
    store.rotate_now();
    ASSERT_EQ(store.key_count(), 2u);

    unsigned char iv[EVP_MAX_IV_LENGTH] = {};
    CipherPair cp;
    EXPECT_EQ(store.handle(old_name, iv, cp.cipher, cp.mac, 0), 2);
}

TEST(TicketKeyStore, CurrentKeyStillReturnsOneAfterRotation) {
    FakeTime ft;
    TicketKeyStore store([&] { return ft(); });
    store.ensure_initialized();

    ft.advance(std::chrono::hours(13));
    store.rotate_now();

    unsigned char current_name[16];
    std::memcpy(current_name, store.snapshot()[0].name, 16);

    unsigned char iv[EVP_MAX_IV_LENGTH] = {};
    CipherPair cp;
    EXPECT_EQ(store.handle(current_name, iv, cp.cipher, cp.mac, 0), 1);
}

TEST(TicketKeyStore, KeysOlderThanLifetimeAreRemoved) {
    FakeTime ft;
    TicketKeyStore store([&] { return ft(); });
    store.ensure_initialized();

    for (int i = 0; i < 5; ++i) {
        ft.advance(std::chrono::hours(13));
        store.rotate_now();
    }

    const auto snap = store.snapshot();
    EXPECT_LE(snap.size(), 4u);
    EXPECT_GE(snap.size(), 2u);

    for (const auto& k : snap) {
        EXPECT_LT(ft.t - k.created_at, TicketKeyStore::kKeyLifetime);
    }
}

TEST(TicketKeyStore, OldestKeyTimestampOrder) {
    FakeTime ft;
    TicketKeyStore store([&] { return ft(); });
    store.ensure_initialized();

    ft.advance(std::chrono::hours(13));
    store.rotate_now();
    ft.advance(std::chrono::hours(13));
    store.rotate_now();

    const auto snap = store.snapshot();
    ASSERT_GE(snap.size(), 3u);

    for (std::size_t i = 1; i < snap.size(); ++i) {
        EXPECT_GE(snap[i - 1].created_at, snap[i].created_at);
    }
}

TEST(TicketKeyStore, TwoStoresHaveDifferentKeys) {
    TicketKeyStore a;
    TicketKeyStore b;
    a.ensure_initialized();
    b.ensure_initialized();

    const auto sa = a.snapshot();
    const auto sb = b.snapshot();
    ASSERT_EQ(sa.size(), 1u);
    ASSERT_EQ(sb.size(), 1u);

    EXPECT_NE(std::memcmp(sa[0].name, sb[0].name, 16), 0);
}

TEST(TicketKeyStore, KeyFromOneStoreNotAcceptedByOther) {
    TicketKeyStore a;
    TicketKeyStore b;
    a.ensure_initialized();
    b.ensure_initialized();

    unsigned char name[16] = {};
    unsigned char iv[EVP_MAX_IV_LENGTH] = {};
    CipherPair cp1;
    ASSERT_EQ(a.handle(name, iv, cp1.cipher, cp1.mac, 1), 1);

    CipherPair cp2;
    EXPECT_EQ(b.handle(name, iv, cp2.cipher, cp2.mac, 0), 0);
}

TEST(TicketKeyStore, AttachRegistersStoreInExData) {
    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    ASSERT_NE(ctx, nullptr);

    TicketKeyStore store;
    store.ensure_initialized();

    store.attach_to(ctx);

    const int idx = brazier::ticket_store_ex_index();
    ASSERT_GE(idx, 0);

    void* raw = SSL_CTX_get_ex_data(ctx, idx);
    EXPECT_EQ(static_cast<TicketKeyStore*>(raw), &store);

    SSL_CTX_free(ctx);
}

TEST(TicketKeyStore, AttachIsIdempotentForSameStore) {
    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    ASSERT_NE(ctx, nullptr);

    TicketKeyStore store;
    store.ensure_initialized();

    store.attach_to(ctx);
    EXPECT_NO_THROW(store.attach_to(ctx));

    SSL_CTX_free(ctx);
}

TEST(TicketKeyStore, AttachThrowsIfDifferentStoreAlreadyAttached) {
    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    ASSERT_NE(ctx, nullptr);

    TicketKeyStore a;
    TicketKeyStore b;
    a.ensure_initialized();
    b.ensure_initialized();

    a.attach_to(ctx);
    EXPECT_THROW(b.attach_to(ctx), std::runtime_error);

    SSL_CTX_free(ctx);
}

TEST(TicketKeyStore, AttachThrowsOnNullCtx) {
    TicketKeyStore store;
    EXPECT_THROW(store.attach_to(nullptr), std::invalid_argument);
}

TEST(TicketKeyStore, TwoContextsUseSeparateStores) {
    SSL_CTX* ctx_a = SSL_CTX_new(TLS_server_method());
    SSL_CTX* ctx_b = SSL_CTX_new(TLS_server_method());
    ASSERT_NE(ctx_a, nullptr);
    ASSERT_NE(ctx_b, nullptr);

    TicketKeyStore a;
    TicketKeyStore b;
    a.ensure_initialized();
    b.ensure_initialized();

    a.attach_to(ctx_a);
    b.attach_to(ctx_b);

    const int idx = brazier::ticket_store_ex_index();
    EXPECT_EQ(SSL_CTX_get_ex_data(ctx_a, idx), &a);
    EXPECT_EQ(SSL_CTX_get_ex_data(ctx_b, idx), &b);

    SSL_CTX_free(ctx_a);
    SSL_CTX_free(ctx_b);
}

TEST(TicketKeyStore, NullNowProviderThrows) {
    EXPECT_THROW(
        TicketKeyStore(std::function<Clock::time_point()>{}),
        std::invalid_argument);
}