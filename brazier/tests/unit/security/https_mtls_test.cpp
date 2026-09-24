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

#include <gtest/gtest.h>

#include <chrono>
#include <fstream>
#include <memory>
#include <string>
#include <thread>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>

#include <openssl/ssl.h>
#include <openssl/x509.h>

#include "../../../include/brazier/Core"
#include "../../../include/brazier/Http"
#include "main.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

namespace {

    constexpr unsigned short kMtlsPort = 9443;
    constexpr int kSocketTimeoutSec = 5;

    const std::string kCaCert = "certs/ca.crt";
    const std::string kServerCert = "certs/server.crt";
    const std::string kServerKey = "certs/server.key";
    const std::string kClientCert = "certs/client.crt";
    const std::string kClientKey = "certs/client.key";

    bool FileExists(const std::string& path) {
        std::ifstream f(path);
        return f.good();
    }

    bool MtlsCertsPresent() {
        return FileExists(kCaCert) && FileExists(kServerCert) &&
            FileExists(kServerKey) && FileExists(kClientCert) &&
            FileExists(kClientKey);
    }

    struct MtlsClient {
        net::io_context io;
        ssl::context    ctx;
        std::unique_ptr<ssl::stream<beast::tcp_stream>> stream;

        explicit MtlsClient(const std::string& cert = "",
            const std::string& key = "")
            : ctx(ssl::context::tls_client) {
            ctx.set_verify_mode(ssl::verify_none);
            if (!cert.empty() && !key.empty()) {
                ctx.use_certificate_chain_file(cert);
                ctx.use_private_key_file(key, ssl::context::pem);
            }
        }

        bool connect(int timeout_sec = kSocketTimeoutSec) {
            try {
                stream = std::make_unique<
                    ssl::stream<beast::tcp_stream>>(io, ctx);

                tcp::resolver resolver(io);
                auto results = resolver.resolve(
                    "127.0.0.1", std::to_string(kMtlsPort));

                beast::get_lowest_layer(*stream).expires_after(
                    std::chrono::seconds(timeout_sec));
                beast::get_lowest_layer(*stream).connect(results);
                beast::get_lowest_layer(*stream).expires_never();

                stream->handshake(ssl::stream_base::client);
                return true;
            }
            catch (const std::exception&) {
                return false;
            }
        }

        bool send_request(const std::string& target = "/test") {
            try {
                std::string req =
                    "GET " + target + " HTTP/1.1\r\n"
                    "Host: localhost\r\n"
                    "Connection: close\r\n"
                    "\r\n";
                beast::get_lowest_layer(*stream).expires_after(
                    std::chrono::seconds(kSocketTimeoutSec));
                net::write(*stream, net::buffer(req));
                return true;
            }
            catch (const std::exception&) {
                return false;
            }
        }

        std::optional<http::response<http::string_body>> read_response(
            int timeout_sec = kSocketTimeoutSec) {
            try {
                beast::flat_buffer buffer;
                http::response<http::string_body> res;
                beast::get_lowest_layer(*stream).expires_after(
                    std::chrono::seconds(timeout_sec));
                http::read(*stream, buffer, res);
                return res;
            }
            catch (const std::exception&) {
                return std::nullopt;
            }
        }
    };

} 

class HttpsMtlsTest : public ::testing::Test {
protected:
    static std::unique_ptr<brazier::HttpsServer> server_;
    static std::thread thread_;

    static void SetUpTestSuite() {
        if (!MtlsCertsPresent()) {
            return; 
        }

        brazier::HttpsServer::TlsConfig tls;
        tls.cert_file = kServerCert;
        tls.key_file = kServerKey;
        tls.ca_file = kCaCert;
        tls.require_client_cert = true;
        tls.verify_client_cert = true;
        tls.handshake_timeout = std::chrono::seconds(3);

        server_ = std::make_unique<brazier::HttpsServer>(
            "127.0.0.1", kMtlsPort, tls);

        if (!server_->initialize()) {
            server_.reset();
            return;
        }

        thread_ = std::thread([] { server_->run(); });

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    static void TearDownTestSuite() {
        if (server_) {
            server_->stop();
            if (thread_.joinable()) thread_.join();
        }
    }

    void SetUp() override {
        if (!server_) {
            GTEST_SKIP() << "mTLS test server not started "
                "(certs missing in certs/)";
        }
    }
};

std::unique_ptr<brazier::HttpsServer> HttpsMtlsTest::server_;
std::thread HttpsMtlsTest::thread_;

TEST_F(HttpsMtlsTest, RejectsClientWithoutCert) {
    MtlsClient c;

    if (c.connect()) {
        c.send_request("/test");
        auto res = c.read_response(3);
        EXPECT_FALSE(res.has_value())
            << "Server responded to request from client without certificate";
    }
}

TEST_F(HttpsMtlsTest, AcceptsClientWithValidCert) {
    MtlsClient c(kClientCert, kClientKey);

    ASSERT_TRUE(c.connect())
        << "Server must accept handshake with valid client certificate";

    X509* peer = SSL_get1_peer_certificate(c.stream->native_handle());
    ASSERT_NE(peer, nullptr)
        << "Server did not present its own certificate";

    char cn[256] = { 0 };
    X509_NAME* subj = X509_get_subject_name(peer);
    X509_NAME_get_text_by_NID(subj, NID_commonName, cn, sizeof(cn));
    EXPECT_STREQ(cn, "localhost")
        << "Unexpected server CN: " << cn;

    X509_free(peer);
}

TEST_F(HttpsMtlsTest, RequestAfterMtlsHandshake) {
    MtlsClient c(kClientCert, kClientKey);
    ASSERT_TRUE(c.connect());
    ASSERT_TRUE(c.send_request("/test"));

    auto res = c.read_response();
    ASSERT_TRUE(res.has_value()) << "No response after mTLS handshake";
    EXPECT_EQ(res->result_int(), 200);
}

TEST_F(HttpsMtlsTest, ManySequentialMtlsHandshakes) {
    for (int i = 0; i < 10; ++i) {
        MtlsClient c(kClientCert, kClientKey);
        ASSERT_TRUE(c.connect()) << "Handshake #" << i << " failed";

        ASSERT_TRUE(c.send_request("/test"));
        auto res = c.read_response();
        ASSERT_TRUE(res.has_value()) << "No response #" << i;
        EXPECT_EQ(res->result_int(), 200);
    }
}

TEST_F(HttpsMtlsTest, NoRequestWithoutClientCert) {
    MtlsClient c;

    if (c.connect()) {
        c.send_request("/test");
        auto res = c.read_response(3);
        EXPECT_FALSE(res.has_value());
    }
}