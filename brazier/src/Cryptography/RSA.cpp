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

#include "../../include/brazier/vendor/Cryptography/RSA.hpp"
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <sstream>
#include <iomanip>

namespace brazier::crypto {

    std::pair<std::string, std::string> RSA::generateKeyPair(int bits) {
        EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
        if (!ctx) {
            throw std::runtime_error("Failed to create RSA context: " + getLastOpenSSLError());
        }

        if (EVP_PKEY_keygen_init(ctx) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            throw std::runtime_error("Failed to initialize key generation: " + getLastOpenSSLError());
        }

        if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, bits) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            throw std::runtime_error("Failed to set RSA key size: " + getLastOpenSSLError());
        }

        EVP_PKEY* pkey = nullptr;
        if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            throw std::runtime_error("Failed to generate RSA key pair: " + getLastOpenSSLError());
        }
        EVP_PKEY_CTX_free(ctx);

        BIO* bioPub = BIO_new(BIO_s_mem());
        if (!bioPub) {
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to create BIO for public key");
        }

        if (PEM_write_bio_PUBKEY(bioPub, pkey) <= 0) {
            BIO_free(bioPub);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to write public key: " + getLastOpenSSLError());
        }
        std::string publicKey = bioToString(bioPub);
        BIO_free(bioPub);

        BIO* bioPriv = BIO_new(BIO_s_mem());
        if (!bioPriv) {
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to create BIO for private key");
        }

        if (PEM_write_bio_PrivateKey(bioPriv, pkey, nullptr, nullptr, 0, nullptr, nullptr) <= 0) {
            BIO_free(bioPriv);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to write private key: " + getLastOpenSSLError());
        }
        std::string privateKey = bioToString(bioPriv);
        BIO_free(bioPriv);

        EVP_PKEY_free(pkey);

        return { publicKey, privateKey };
    }

    std::string RSA::encryptWithPublic(const std::string& plaintext, const std::string& publicKey) {
        EVP_PKEY* pkey = loadPublicKey(publicKey);
        if (!pkey) {
            throw std::runtime_error("Failed to load public key: " + getLastOpenSSLError());
        }

        EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
        if (!ctx) {
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to create encryption context: " + getLastOpenSSLError());
        }

        if (EVP_PKEY_encrypt_init(ctx) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to initialize encryption: " + getLastOpenSSLError());
        }

        if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to set RSA padding: " + getLastOpenSSLError());
        }

        size_t outLen = 0;
        if (EVP_PKEY_encrypt(ctx, nullptr, &outLen,
            reinterpret_cast<const unsigned char*>(plaintext.data()),
            plaintext.size()) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to get encrypted length: " + getLastOpenSSLError());
        }

        std::string ciphertext(outLen, 0);
        if (EVP_PKEY_encrypt(ctx,
            reinterpret_cast<unsigned char*>(&ciphertext[0]), &outLen,
            reinterpret_cast<const unsigned char*>(plaintext.data()),
            plaintext.size()) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to encrypt: " + getLastOpenSSLError());
        }

        ciphertext.resize(outLen);
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);

        return ciphertext;
    }

    std::string RSA::decryptWithPrivate(const std::string& ciphertext, const std::string& privateKey) {
        EVP_PKEY* pkey = loadPrivateKey(privateKey);
        if (!pkey) {
            throw std::runtime_error("Failed to load private key: " + getLastOpenSSLError());
        }

        EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
        if (!ctx) {
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to create decryption context: " + getLastOpenSSLError());
        }

        if (EVP_PKEY_decrypt_init(ctx) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to initialize decryption: " + getLastOpenSSLError());
        }

        if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to set RSA padding: " + getLastOpenSSLError());
        }

        size_t outLen = 0;
        if (EVP_PKEY_decrypt(ctx, nullptr, &outLen,
            reinterpret_cast<const unsigned char*>(ciphertext.data()),
            ciphertext.size()) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to get decrypted length: " + getLastOpenSSLError());
        }

        std::string plaintext(outLen, 0);
        if (EVP_PKEY_decrypt(ctx,
            reinterpret_cast<unsigned char*>(&plaintext[0]), &outLen,
            reinterpret_cast<const unsigned char*>(ciphertext.data()),
            ciphertext.size()) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to decrypt: " + getLastOpenSSLError());
        }

        plaintext.resize(outLen);
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);

        return plaintext;
    }

    std::string RSA::encryptWithPrivate(const std::string& plaintext, const std::string& privateKey) {
        return sign(plaintext, privateKey);
    }

    std::string RSA::decryptWithPublic(const std::string& ciphertext, const std::string& publicKey) {
        EVP_PKEY* pkey = loadPublicKey(publicKey);
        if (!pkey) {
            throw std::runtime_error("Failed to load public key: " + getLastOpenSSLError());
        }

        EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
        if (!ctx) {
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to create decryption context: " + getLastOpenSSLError());
        }

        if (EVP_PKEY_decrypt_init(ctx) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to initialize decryption: " + getLastOpenSSLError());
        }

        if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to set RSA padding: " + getLastOpenSSLError());
        }

        size_t outLen = 0;
        if (EVP_PKEY_decrypt(ctx, nullptr, &outLen,
            reinterpret_cast<const unsigned char*>(ciphertext.data()),
            ciphertext.size()) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to get decrypted length: " + getLastOpenSSLError());
        }

        std::string plaintext(outLen, 0);
        if (EVP_PKEY_decrypt(ctx,
            reinterpret_cast<unsigned char*>(&plaintext[0]), &outLen,
            reinterpret_cast<const unsigned char*>(ciphertext.data()),
            ciphertext.size()) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to decrypt with public key: " + getLastOpenSSLError());
        }

        plaintext.resize(outLen);
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);

        return plaintext;
    }

    std::string RSA::sign(const std::string& data, const std::string& privateKey) {
        EVP_PKEY* pkey = loadPrivateKey(privateKey);
        if (!pkey) {
            throw std::runtime_error("Failed to load private key for signing: " + getLastOpenSSLError());
        }

        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        if (!ctx) {
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to create signature context");
        }

        if (EVP_DigestSignInit(ctx, nullptr, EVP_sha256(), nullptr, pkey) <= 0) {
            EVP_MD_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to initialize signing: " + getLastOpenSSLError());
        }

        if (EVP_DigestSignUpdate(ctx, data.data(), data.size()) <= 0) {
            EVP_MD_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to update signature: " + getLastOpenSSLError());
        }

        size_t sigLen = 0;
        if (EVP_DigestSignFinal(ctx, nullptr, &sigLen) <= 0) {
            EVP_MD_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to get signature length: " + getLastOpenSSLError());
        }

        std::string signature(sigLen, 0);
        if (EVP_DigestSignFinal(ctx,
            reinterpret_cast<unsigned char*>(&signature[0]), &sigLen) <= 0) {
            EVP_MD_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to create signature: " + getLastOpenSSLError());
        }

        signature.resize(sigLen);
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);

        return signature;
    }

    bool RSA::verify(const std::string& data, const std::string& signature, const std::string& publicKey) {
        EVP_PKEY* pkey = loadPublicKey(publicKey);
        if (!pkey) {
            throw std::runtime_error("Failed to load public key for verification: " + getLastOpenSSLError());
        }

        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        if (!ctx) {
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to create verification context");
        }

        if (EVP_DigestVerifyInit(ctx, nullptr, EVP_sha256(), nullptr, pkey) <= 0) {
            EVP_MD_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to initialize verification: " + getLastOpenSSLError());
        }

        if (EVP_DigestVerifyUpdate(ctx, data.data(), data.size()) <= 0) {
            EVP_MD_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to update verification: " + getLastOpenSSLError());
        }

        int result = EVP_DigestVerifyFinal(ctx,
            reinterpret_cast<const unsigned char*>(signature.data()),
            signature.size());

        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);

        return result == 1;
    }

    bool RSA::validatePublicKey(const std::string& publicKey) {
        EVP_PKEY* pkey = loadPublicKey(publicKey);
        if (!pkey) return false;
        EVP_PKEY_free(pkey);
        return true;
    }

    bool RSA::validatePrivateKey(const std::string& privateKey) {
        EVP_PKEY* pkey = loadPrivateKey(privateKey);
        if (!pkey) return false;
        EVP_PKEY_free(pkey);
        return true;
    }

    std::string RSA::extractPublicKey(const std::string& privateKey) {
        EVP_PKEY* pkey = loadPrivateKey(privateKey);
        if (!pkey) {
            throw std::runtime_error("Failed to load private key: " + getLastOpenSSLError());
        }

        BIO* bio = BIO_new(BIO_s_mem());
        if (!bio) {
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to create BIO");
        }

        if (PEM_write_bio_PUBKEY(bio, pkey) <= 0) {
            BIO_free(bio);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to extract public key: " + getLastOpenSSLError());
        }

        std::string publicKey = bioToString(bio);
        BIO_free(bio);
        EVP_PKEY_free(pkey);

        return publicKey;
    }

    std::string RSA::formatPublicKey(const std::string& pem) {
        std::string result;
        for (char c : pem) {
            if (c != '\r' && c != '\n' && c != ' ') {
                result += c;
            }
        }
        return result;
    }

    std::string RSA::formatPrivateKey(const std::string& pem) {
        return formatPublicKey(pem);
    }

    std::string RSA::bioToString(BIO* bio) {
        char* data = nullptr;
        long len = BIO_get_mem_data(bio, &data);
        if (len <= 0 || !data) {
            return "";
        }
        return std::string(data, len);
    }

    std::string RSA::getLastOpenSSLError() {
        char buf[256];
        ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
        return std::string(buf);
    }

    EVP_PKEY* RSA::loadPublicKey(const std::string& pem) {
        BIO* bio = BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size()));
        if (!bio) {
            return nullptr;
        }

        EVP_PKEY* pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
        BIO_free(bio);

        return pkey;
    }

    EVP_PKEY* RSA::loadPrivateKey(const std::string& pem) {
        BIO* bio = BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size()));
        if (!bio) {
            return nullptr;
        }

        EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
        BIO_free(bio);

        return pkey;
    }

}