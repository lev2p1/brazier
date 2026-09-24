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

#include "../include/brazier/HttpsServer.hpp"

#include <openssl/pem.h>

void brazier::HttpsServer::apply_ssl_conf(ssl::context& ctx) {
    if (tls_.conf.empty()) return;

    SSL_CONF_CTX* cctx = SSL_CONF_CTX_new();
    if (!cctx) {
        throw std::runtime_error("SSL_CONF_CTX_new failed");
    }

    SSL_CONF_CTX_set_flags(cctx,
        SSL_CONF_FLAG_SERVER | SSL_CONF_FLAG_CERTIFICATE);
    SSL_CONF_CTX_set_ssl_ctx(cctx, ctx.native_handle());

    for (const auto& [cmd, val] : tls_.conf) {
        int rv = val.empty()
            ? SSL_CONF_cmd(cctx, cmd.c_str(), nullptr)
            : SSL_CONF_cmd(cctx, cmd.c_str(), val.c_str());

        if (rv <= 0) {
            SSL_CONF_CTX_free(cctx);
            throw std::runtime_error(
                "SSL_CONF_cmd failed for '" + cmd +
                (val.empty() ? "'" : "=" + val + "'"));
        }
    }

    if (SSL_CONF_CTX_finish(cctx) != 1) {
        SSL_CONF_CTX_free(cctx);
        throw std::runtime_error("SSL_CONF_CTX_finish failed");
    }

    SSL_CONF_CTX_free(cctx);
}

void brazier::HttpsServer::configure_ssl_ctx(ssl::context& ctx) {
    ctx.set_options(
        ssl::context::default_workarounds
        | ssl::context::no_sslv2
        | ssl::context::no_sslv3
        | ssl::context::no_tlsv1
        | ssl::context::no_tlsv1_1
        | ssl::context::single_dh_use);

    SSL_CTX_set_options(ctx.native_handle(),
        SSL_OP_IGNORE_UNEXPECTED_EOF);

    SSL_CTX_set_session_cache_mode(ctx.native_handle(), SSL_SESS_CACHE_OFF);

    ticket_store_.ensure_initialized();
    ticket_store_.attach_to(ctx.native_handle());

    apply_ssl_conf(ctx);

    if (!tls_.cert_pem.empty() && !tls_.key_pem.empty()) {
        load_cert_from_memory(ctx, tls_.cert_pem, tls_.key_pem);
    }
    else {
        ctx.use_certificate_chain_file(tls_.cert_file);
        ctx.use_private_key_file(tls_.key_file, ssl::context::pem);

        if (SSL_CTX_check_private_key(ctx.native_handle()) != 1) {
            throw std::runtime_error(
                "Certificate/private key mismatch: " +
                tls_.cert_file + " / " + tls_.key_file);
        }
    }

    if (tls_.require_client_cert || tls_.verify_client_cert) {
        if (!tls_.ca_file.empty()) {
            ctx.load_verify_file(tls_.ca_file);
        }
        auto mode = ssl::verify_peer;
        if (tls_.require_client_cert) {
            mode |= ssl::verify_fail_if_no_peer_cert;
        }
        ctx.set_verify_mode(mode);
    }
    else {
        ctx.set_verify_mode(ssl::verify_none);
    }
}

void brazier::HttpsServer::configure_tls() {
    if (!ssl_ctx_) {
        throw std::runtime_error("configure_tls: ssl_ctx_ not initialized");
    }
    configure_ssl_ctx(*ssl_ctx_);
}

void brazier::HttpsServer::load_cert_from_memory(
    ssl::context& ctx,
    const std::string& cert_pem, const std::string& key_pem)
{
    if (cert_pem.empty() || key_pem.empty()) {
        throw std::runtime_error("load_cert_from_memory: empty PEM");
    }

    BIO* cert_bio = BIO_new_mem_buf(cert_pem.data(),
        static_cast<int>(cert_pem.size()));
    if (!cert_bio) {
        throw std::runtime_error("BIO_new_mem_buf (cert) failed");
    }

    X509* leaf = PEM_read_bio_X509(cert_bio, nullptr, nullptr, nullptr);
    if (!leaf) {
        BIO_free(cert_bio);
        throw std::runtime_error("PEM_read_bio_X509 failed: " +
            std::string(ERR_error_string(ERR_get_error(), nullptr)));
    }

    if (SSL_CTX_use_certificate(ctx.native_handle(), leaf) != 1) {
        X509_free(leaf);
        BIO_free(cert_bio);
        throw std::runtime_error("SSL_CTX_use_certificate failed");
    }
    X509_free(leaf);

    X509* chain_cert = nullptr;
    while ((chain_cert = PEM_read_bio_X509(cert_bio, nullptr,
        nullptr, nullptr)) != nullptr) {
        if (SSL_CTX_add_extra_chain_cert(ctx.native_handle(),
            chain_cert) != 1) {
            X509_free(chain_cert);
            BIO_free(cert_bio);
            throw std::runtime_error(
                "SSL_CTX_add_extra_chain_cert failed");
        }
    }
    BIO_free(cert_bio);

    BIO* key_bio = BIO_new_mem_buf(key_pem.data(),
        static_cast<int>(key_pem.size()));
    if (!key_bio) {
        throw std::runtime_error("BIO_new_mem_buf (key) failed");
    }

    EVP_PKEY* pkey = PEM_read_bio_PrivateKey(key_bio, nullptr,
        nullptr, nullptr);
    BIO_free(key_bio);

    if (!pkey) {
        throw std::runtime_error("PEM_read_bio_PrivateKey failed: " +
            std::string(ERR_error_string(ERR_get_error(), nullptr)));
    }

    if (SSL_CTX_use_PrivateKey(ctx.native_handle(), pkey) != 1) {
        EVP_PKEY_free(pkey);
        throw std::runtime_error("SSL_CTX_use_PrivateKey failed");
    }
    EVP_PKEY_free(pkey);

    if (SSL_CTX_check_private_key(ctx.native_handle()) != 1) {
        throw std::runtime_error(
            "Certificate and private key do not match");
    }

    Logger::log("TLS certificate loaded from PEM (in-memory, with chain)",
        "INFO");
}

std::shared_ptr<ssl::context> brazier::HttpsServer::get_ssl_ctx() {
    std::shared_lock lock(ssl_ctx_mutex_);
    return ssl_ctx_;
}

bool brazier::HttpsServer::reloadTls() {
    if (!tls_.cert_file.empty() && !tls_.key_file.empty()) {
        std::ifstream cf(tls_.cert_file, std::ios::binary);
        std::ifstream kf(tls_.key_file, std::ios::binary);

        if (!cf || !kf) {
            Logger::log("reloadTls: cannot open files: " +
                tls_.cert_file + " / " + tls_.key_file, "ERROR");
            return false;
        }

        TlsConfig new_tls = tls_;
        new_tls.cert_pem = std::string(
            std::istreambuf_iterator<char>(cf), {});
        new_tls.key_pem = std::string(
            std::istreambuf_iterator<char>(kf), {});

        return reloadTls(new_tls);
    }

    Logger::log("reloadTls: no file paths configured, "
        "use reloadTls(new_tls) with fresh PEM", "WARNING");
    return false;
}

bool brazier::HttpsServer::reloadTls(const TlsConfig& new_tls) {
    try {
        auto new_ctx = std::make_shared<ssl::context>(
            ssl::context::tls_server);

        TlsConfig saved = tls_;
        tls_ = new_tls;

        try {
            configure_ssl_ctx(*new_ctx);
        }
        catch (...) {
            tls_ = saved;
            throw;
        }

        {
            std::unique_lock lock(ssl_ctx_mutex_);
            ssl_ctx_ = new_ctx;
        }

        Logger::log("TLS reloaded successfully (new SSL_CTX active)",
            "SUCCESS");
        return true;
    }
    catch (const std::exception& e) {
        Logger::log("TLS reload failed: " + std::string(e.what()) +
            " (keeping old SSL_CTX)", "ERROR");
        return false;
    }
}