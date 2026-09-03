#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>

namespace brazier::crypto {

    class AES256 {
    public:
        static constexpr size_t KEY_SIZE = 32;
        static constexpr size_t BLOCK_SIZE = 16;
        static constexpr size_t SALT_SIZE = 16;
        static constexpr size_t HMAC_SIZE = 32;

        static std::string generateKey();
        static std::string generateIV();
        static std::string generateSalt();
        static std::string deriveKeyFromPassword(const std::string& password, const std::string& salt, int iterations = 100000);
        static std::string encrypt(const std::string& plaintext, const std::string& key, const std::string& iv);
        static std::string decrypt(const std::string& ciphertext, const std::string& key, const std::string& iv);
        static std::string encryptWithHmac(const std::string& plaintext, const std::string& key, const std::string& iv);
        static std::string decryptWithHmac(const std::string& data, const std::string& key, const std::string& iv);
        static std::string encryptMessage(const std::string& message, const std::string& key);
        static std::string decryptMessage(const std::string& encryptedData, const std::string& key);
        static std::string toHex(const std::string& binary);
        static std::string fromHex(const std::string& hex);
        static bool validateKey(const std::string& key);
        static bool validateIV(const std::string& iv);

    private:
        static std::string generateRandomBytes(int size);
        static std::string computeHmac(const std::string& data, const std::string& key);
    };
}