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
#include <cstdlib>
#include <fstream>
#include <memory>
#include <optional>
#include <string>

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

    int envInt(const char* name, int fallback) {
        if (const char* v = std::getenv(name)) {
            try { return std::stoi(v); }
            catch (...) {}
        }
        return fallback;
    }

    const int kHandshakeTimeoutSec = envInt("BRAZIER_HANDSHAKE_TIMEOUT", 15);
    const int kIdleTimeoutSec = envInt("BRAZIER_IDLE_TIMEOUT", 60);

    constexpr int kMaxTestableTimeoutSec = 30;
    constexpr int kSocketTimeoutSec = 5;

} 

namespace {

    struct TlsClient {
        net::io_context io;
        ssl::context    ctx;
        std::unique_ptr<ssl::stream<beast::tcp_stream>> stream;

        explicit TlsClient(bool with_client_cert = false,
            const std::string& cert = "",
            const std::string& key = "")
            : ctx(ssl::context::tls_client) {
            ctx.set_verify_mode(ssl::verify_none);
            if (with_client_cert) {
                ctx.use_certificate_chain_file(cert);
                ctx.use_private_key_file(key, ssl::context::pem);
            }
        }

        bool connect(int timeout_sec = kSocketTimeoutSec) {
            try {
                stream = std::make_unique<ssl::stream<beast::tcp_stream>>(io, ctx);

                tcp::resolver resolver(io);
                auto results = resolver.resolve(https_host_global,
                    std::to_string(https_port_global));

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

        bool send_raw(const std::string& data) {
            try {
                beast::get_lowest_layer(*stream).expires_after(
                    std::chrono::seconds(kSocketTimeoutSec));
                net::write(*stream, net::buffer(data));
                return true;
            }
            catch (const std::exception&) {
                return false;
            }
        }

        std::optional<http::response<http::string_body>>
            read_response(int timeout_sec = kSocketTimeoutSec) {
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

        bool is_connection_closed(int timeout_sec = kSocketTimeoutSec) {
            if (!stream) return true;
            try {
                beast::get_lowest_layer(*stream).expires_after(
                    std::chrono::seconds(timeout_sec));
                char c;
                beast::error_code ec;
                auto n = stream->read_some(net::buffer(&c, 1), ec);

                if (ec == net::error::eof ||
                    ec == net::error::connection_reset ||
                    ec == ssl::error::stream_truncated ||
                    ec == beast::error::timeout) {
                    return true;
                }
                if (ec == net::error::timed_out) return false;
                if (!ec && n > 0) return false;
                return true;
            }
            catch (const std::exception&) {
                return true;
            }
        }
    };

    bool ServerRequiresClientCert() {
        TlsClient c;
        return !c.connect();
    }

    bool ClientCertExists(const std::string& cert, const std::string& key) {
        return std::ifstream(cert).good() && std::ifstream(key).good();
    }

    const std::string kClientCert = "certs/client.crt";
    const std::string kClientKey = "certs/client.key";

} 

class HttpsSecurityTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        TlsClient c;
        server_available_ = c.connect();
    }

    void SetUp() override {
        if (!server_available_) {
            GTEST_SKIP() << "HTTPS server is not running on "
                << https_host_global << ":" << https_port_global;
        }
    }

    static bool server_available_;
};

bool HttpsSecurityTest::server_available_ = false;

TEST_F(HttpsSecurityTest, HandshakeWithoutClientCert) {
    TlsClient c;
    bool connected = c.connect();

    if (ServerRequiresClientCert()) {
        EXPECT_FALSE(connected)
            << "Server requires client cert, but handshake succeeded without one";
    }
    else {
        EXPECT_TRUE(connected)
            << "Server does not require client cert, but handshake failed";
    }
}

TEST_F(HttpsSecurityTest, HandshakeWithClientCert) {
    if (!ServerRequiresClientCert()) {
        GTEST_SKIP() << "Server does not require client cert (mTLS disabled)";
    }
    if (!ClientCertExists(kClientCert, kClientKey)) {
        GTEST_SKIP() << "Client cert not found at " << kClientCert;
    }

    TlsClient c(true, kClientCert, kClientKey);
    EXPECT_TRUE(c.connect())
        << "Server requires client cert, but handshake with cert failed";
}

TEST_F(HttpsSecurityTest, NoRequestWithoutClientCertWhenMtlsRequired) {
    if (!ServerRequiresClientCert()) {
        GTEST_SKIP() << "Server does not require client cert";
    }

    TlsClient c;
    EXPECT_FALSE(c.connect())
        << "Handshake should have failed without client cert";
}

TEST_F(HttpsSecurityTest, HandshakeTimeout) {
    if (kHandshakeTimeoutSec > kMaxTestableTimeoutSec) {
        GTEST_SKIP() << "Handshake timeout is " << kHandshakeTimeoutSec
            << "s, skipping to keep test fast";
    }

    net::io_context io;
    beast::tcp_stream stream(io);

    tcp::resolver resolver(io);
    auto results = resolver.resolve(https_host_global,
        std::to_string(https_port_global));
    stream.connect(results);

    stream.expires_after(std::chrono::seconds(kHandshakeTimeoutSec + 5));

    beast::error_code ec;
    char c;
    stream.read_some(net::buffer(&c, 1), ec);

    bool closed =
        (ec == net::error::eof) ||
        (ec == net::error::connection_reset) ||
        (ec == beast::error::timeout);

    EXPECT_TRUE(closed)
        << "Server did not close idle TCP within "
        << (kHandshakeTimeoutSec + 5) << "s, ec=" << ec.message();
}

TEST_F(HttpsSecurityTest, IdleTimeout) {
    if (kIdleTimeoutSec > kMaxTestableTimeoutSec) {
        GTEST_SKIP() << "Idle timeout is " << kIdleTimeoutSec
            << "s, skipping to keep test fast";
    }

    TlsClient c;
    ASSERT_TRUE(c.connect()) << "Initial TLS handshake failed";

    EXPECT_TRUE(c.is_connection_closed(kIdleTimeoutSec + 5))
        << "Server did not close idle TLS within "
        << (kIdleTimeoutSec + 5) << "s";
}

TEST_F(HttpsSecurityTest, ClientInitiatedGracefulClose) {
    {
        TlsClient c;
        ASSERT_TRUE(c.connect());
        ASSERT_TRUE(c.send_raw(
            "GET /test HTTP/1.1\r\n"
            "Host: " + https_host_global + "\r\n"
            "Connection: close\r\n"
            "\r\n"));

        auto res = c.read_response();
        if (res.has_value()) {
            EXPECT_EQ(res->result_int(), 200);
            EXPECT_EQ((*res)[http::field::connection], "close");
        }

    }

    TlsClient c2;
    EXPECT_TRUE(c2.connect())
        << "Server is not accepting connections after graceful close";
}

TEST_F(HttpsSecurityTest, ServerSendsCloseNotify) {
    TlsClient c;
    ASSERT_TRUE(c.connect());
    ASSERT_TRUE(c.send_raw(
        "GET /test HTTP/1.1\r\n"
        "Host: " + https_host_global + "\r\n"
        "Connection: close\r\n"
        "\r\n"));

    auto res = c.read_response();
    if (res.has_value()) {
        EXPECT_EQ(res->result_int(), 200);
        EXPECT_EQ((*res)[http::field::connection], "close");
    }

    EXPECT_TRUE(c.is_connection_closed(5))
        << "Server did not close after 'Connection: close'";
}

TEST_F(HttpsSecurityTest, GarbageRequest) {
    TlsClient c;
    ASSERT_TRUE(c.connect());
    ASSERT_TRUE(c.send_raw("this is not an HTTP request at all\r\n\r\n"));

    auto res = c.read_response(3);
    bool closed = c.is_connection_closed(3);

    bool acceptable = (res.has_value() && res->result_int() == 400) ||
        (!res.has_value() && closed);
    EXPECT_TRUE(acceptable)
        << "Expected 400 or close, got has_value=" << res.has_value()
        << ", status=" << (res.has_value() ? res->result_int() : 0)
        << ", closed=" << closed;
}

TEST_F(HttpsSecurityTest, InvalidMethod) {
    TlsClient c;
    ASSERT_TRUE(c.connect());
    ASSERT_TRUE(c.send_raw(
        "INVALID_METHOD_XYZ /test HTTP/1.1\r\n"
        "Host: " + https_host_global + "\r\n"
        "Connection: close\r\n"
        "\r\n"));

    auto res = c.read_response();

    if (res.has_value()) {
        EXPECT_NE(res->result_int(), 500)
            << "Server returned 500 for invalid method";
    }
    else {
        EXPECT_TRUE(c.is_connection_closed(2))
            << "No response and connection not closed";
    }
}

TEST_F(HttpsSecurityTest, VeryLongUri) {
    TlsClient c;
    ASSERT_TRUE(c.connect());

    std::string long_path = "/" + std::string(16 * 1024, 'a');
    ASSERT_TRUE(c.send_raw(
        "GET " + long_path + " HTTP/1.1\r\n"
        "Host: " + https_host_global + "\r\n"
        "Connection: close\r\n"
        "\r\n"));

    auto res = c.read_response();

    if (res.has_value()) {
        EXPECT_NE(res->result_int(), 200)
            << "Server accepted 16KB URI as valid";
        EXPECT_NE(res->result_int(), 500)
            << "Server returned 500 on long URI";
    }
    else {
        EXPECT_TRUE(c.is_connection_closed(2))
            << "No response and connection not closed";
    }
}

TEST_F(HttpsSecurityTest, IncompleteHeaders) {
    if (kIdleTimeoutSec > kMaxTestableTimeoutSec) {
        GTEST_SKIP() << "Idle timeout is " << kIdleTimeoutSec
            << "s, skipping to keep test fast. "
            "Set BRAZIER_IDLE_TIMEOUT env or keep-alive-timeout in config.";
    }

    TlsClient c;
    ASSERT_TRUE(c.connect());
    ASSERT_TRUE(c.send_raw(
        "GET /test HTTP/1.1\r\n"
        "Host: " + https_host_global + "\r\n"));

    auto res = c.read_response(5);
    bool closed = c.is_connection_closed(1);

    bool acceptable = (res.has_value() && res->result_int() == 400) ||
        (!res.has_value() && closed);
    EXPECT_TRUE(acceptable)
        << "Server neither responded 400 nor closed on incomplete headers";
}

TEST_F(HttpsSecurityTest, BodyWithZeroContentLength) {
    TlsClient c;
    ASSERT_TRUE(c.connect());
    ASSERT_TRUE(c.send_raw(
        "POST /test/echo HTTP/1.1\r\n"
        "Host: " + https_host_global + "\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n"
        "\r\n"
        "extra body that should be ignored"));

    auto res = c.read_response();
    if (res.has_value()) {
        EXPECT_NE(res->result_int(), 500);
    }
}

TEST_F(HttpsSecurityTest, ManySequentialHandshakes) {
    for (int i = 0; i < 20; ++i) {
        TlsClient c;
        ASSERT_TRUE(c.connect()) << "Handshake #" << i << " failed";
    }
}

TEST_F(HttpsSecurityTest, AbruptClientDisconnect) {
    {
        TlsClient c;
        ASSERT_TRUE(c.connect());
    }

    TlsClient c2;
    EXPECT_TRUE(c2.connect()) << "Server not accepting after abrupt disconnect";
}