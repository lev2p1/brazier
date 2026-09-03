/*
 * Copyright (c) 2026 Kirill Sergeev, Nikolay Sugonyako, Andrey Agarkov, Gleb Safyannikov
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This file is part of brazier.
 *
 * brazier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * brazier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with brazier; if not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <memory>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/bio.h>

namespace brazier::crypto {

    class RSA {
    public:
        static constexpr size_t KEY_SIZE_2048 = 2048;
        static constexpr size_t KEY_SIZE_4096 = 4096;

        static std::pair<std::string, std::string> generateKeyPair(int bits = KEY_SIZE_2048);
        static std::string encryptWithPublic(const std::string& plaintext, const std::string& publicKey);
        static std::string decryptWithPrivate(const std::string& ciphertext, const std::string& privateKey);
        static std::string encryptWithPrivate(const std::string& plaintext, const std::string& privateKey);
        static std::string decryptWithPublic(const std::string& ciphertext, const std::string& publicKey);
        static std::string sign(const std::string& data, const std::string& privateKey);
        static bool verify(const std::string& data, const std::string& signature, const std::string& publicKey);
        static bool validatePublicKey(const std::string& publicKey);
        static bool validatePrivateKey(const std::string& privateKey);
        static std::string extractPublicKey(const std::string& privateKey);
        static std::string formatPublicKey(const std::string& pem);
        static std::string formatPrivateKey(const std::string& pem);

    private:
        static std::string bioToString(BIO* bio);
        static std::string getLastOpenSSLError();
        static EVP_PKEY* loadPublicKey(const std::string& pem);
        static EVP_PKEY* loadPrivateKey(const std::string& pem);
    };
}