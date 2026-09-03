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

#include "../include/brazier/vendor/Facades/Hash.hpp"

using namespace brazier;

std::vector<unsigned char> Hash::generateSalt(int length) {
    std::vector<unsigned char> salt(length);
    if (RAND_bytes(salt.data(), length) != 1) {
        Logger::log("Failed to generate salt", "ERROR");
        return {};
    }
    return salt;
}

std::pair<std::string, std::vector<unsigned char>> Hash::hash(const std::string& password, std::vector<unsigned char> salt) {
    std::vector<unsigned char> output(output_length);
    uint32_t actual_iterations = iterations;

    EVP_KDF* kdf = EVP_KDF_fetch(nullptr, "ARGON2ID", nullptr);
    if (!kdf) {
        Logger::log("Error: Argon2 not supported in OpenSSL", "ERROR");
        return { "", std::vector<unsigned char> {} };
    }

    EVP_KDF_CTX* ctx = EVP_KDF_CTX_new(kdf);
    if (!ctx) {
        Logger::log("Error: Failed to create KDF context", "ERROR");
        EVP_KDF_free(kdf);
        return { "", std::vector<unsigned char> {} };
    }

    salt = salt.size() > 0 ? salt : generateSalt(64);

    OSSL_PARAM params[] = {
        OSSL_PARAM_construct_octet_string("pass", const_cast<char*>(password.data()), password.size()),
        OSSL_PARAM_construct_octet_string("salt", salt.data(), salt.size()),
        OSSL_PARAM_construct_uint("iter", &actual_iterations),
        OSSL_PARAM_construct_uint("memcost", &memory_cost),
        OSSL_PARAM_construct_uint("parallelism", &parallelism),
        OSSL_PARAM_construct_end()
    };

    if (EVP_KDF_derive(ctx, output.data(), output.size(), params) <= 0) {
        Logger::log("Error: Argon2 hashing failed", "ERROR");
        EVP_KDF_CTX_free(ctx);
        EVP_KDF_free(kdf);
        return { "", std::vector<unsigned char> {} };
    }

    std::ostringstream oss;
    for (unsigned char byte : output) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    }

    EVP_KDF_CTX_free(ctx);
    EVP_KDF_free(kdf);

    return { oss.str(),  salt };
}

std::vector<uint8_t> Hash::hexStringToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(strtol(byteString.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

std::string Hash::bytesToHexString(const std::vector<uint8_t>& bytes) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t byte : bytes) {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

bool Hash::verify(const std::string& password, const std::string& stored_hash, const std::vector<unsigned char>& salt) {
    auto [new_hash, _] = Hash::hash(password, salt);

    auto stored_bytes = Hash::hexStringToBytes(stored_hash);
    auto new_bytes = Hash::hexStringToBytes(new_hash);
    bool match = stored_bytes == new_bytes;

    return match;
}

boost::asio::awaitable<std::pair<std::string, std::vector<unsigned char>>> Hash::awaitableHash(
    const std::string& password,
    std::vector<unsigned char> salt) {
    co_return Hash::hash(password, salt);
}

boost::asio::awaitable<bool> Hash::awaitableVerify(
    const std::string& password, const std::string& stored_hash, const std::vector<unsigned char>& salt) {
    co_return Hash::verify(password, stored_hash, salt);
}