/*
 * Copyright (c) 2026 Kirill Sergeev, Nikolay Sugonyako, Andrey Agarkov, Gleb Safyannikov
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This file is part of lightlib.
 *
 * lightlib is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * lightlib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lightlib; if not, see <https://www.gnu.org/licenses/>.
 */

#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <chrono>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "../../../include/lightlib/Core"
#include "../../../include/lightlib/Http"
#include "main.h"

namespace beast = boost::beast;
namespace http = beast::http;

using namespace lightlib;

class TestController : public lightlib::Controller {
public:
    using Request = http::request<http::string_body>;
    using Response = http::response<http::string_body>;

    boost::asio::awaitable<void> show(const Request& req, Response& res, const Params& params) override {
        res.result(http::status::ok);
        res.set(http::field::content_type, "text/plain");
        res.body() = "TestController show method called";
        co_return;
    }

    boost::asio::awaitable<void> json_response(const Request& req, Response& res, const Params& params) {
        res.result(http::status::ok);
        res.set(http::field::content_type, "application/json");
        res.body() = R"({"status":"success","message":"JSON response from TestController"})";
        co_return;
    }

    boost::asio::awaitable<void> echo_post(const Request& req, Response& res, const Params& params) {
        res.result(http::status::ok);
        res.set(http::field::content_type, "application/json");
        res.body() = req.body();
        co_return;
    }
};

template<typename AsyncOp>
auto RunAsync(AsyncOp&& op) {
    boost::asio::io_context io_context;
    auto future = boost::asio::co_spawn(
        io_context,
        std::forward<AsyncOp>(op),
        boost::asio::use_future
    );
    std::thread([&io_context]() { io_context.run(); }).detach();
    return future.get();
}

bool IsServerRunning() {
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::socket socket(io_context);
    boost::asio::ip::tcp::endpoint endpoint(
        boost::asio::ip::make_address(host_global),
        port_global
    );
    boost::system::error_code ec;
    socket.connect(endpoint, ec);
    return !ec;
}

class RoutingTest : public ::testing::Test {
protected:
    void SetUp() override {
        if (!IsServerRunning()) {
            GTEST_SKIP() << "Server is not running";
        }
    }
};

TEST_F(RoutingTest, AddRouteAndGet) {
    std::string host = host_global;
    std::string port = std::to_string(port_global);
    std::string base_url = "http://" + host + ":" + port;

    auto test_controller = std::make_shared<TestController>();

    R(GET, "/test", test_controller, show);
    R(GET, "/test/json", test_controller, json_response);
    R(POST, "/test/echo", test_controller, echo_post);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    lightlib::HttpClient client;
    client.set_verify_ssl(false);
    client.set_timeout(std::chrono::seconds(10));

    try {
        auto response = RunAsync([&]() -> boost::asio::awaitable<lightlib::Response> {
            co_return co_await client.get(base_url + "/test");
            });

        EXPECT_EQ(response.result_int(), 200);
        EXPECT_EQ(response.body(), "TestController show method called");
        EXPECT_EQ(response[boost::beast::http::field::content_type], "text/plain");
    }
    catch (const std::exception& e) {
        FAIL() << "Request failed: " << e.what();
    }
}

TEST_F(RoutingTest, JsonResponse) {
    std::string host = host_global;
    std::string port = std::to_string(port_global);
    std::string base_url = "http://" + host + ":" + port;

    lightlib::HttpClient client;
    client.set_verify_ssl(false);
    client.set_timeout(std::chrono::seconds(5));

    try {
        auto response = RunAsync([&]() -> boost::asio::awaitable<lightlib::Response> {
            co_return co_await client.get(base_url + "/test/json");
            });

        EXPECT_EQ(response.result_int(), 200);
        EXPECT_EQ(response[boost::beast::http::field::content_type], "application/json");

        auto json = nlohmann::json::parse(response.body());
        EXPECT_EQ(json["status"], "success");
        EXPECT_EQ(json["message"], "JSON response from TestController");
    }
    catch (const std::exception& e) {
        FAIL() << "Request failed: " << e.what();
    }
}

TEST_F(RoutingTest, PostWithBody) {
    std::string host = host_global;
    std::string port = std::to_string(port_global);
    std::string base_url = "http://" + host + ":" + port;

    lightlib::HttpClient client;
    client.set_verify_ssl(false);
    client.set_timeout(std::chrono::seconds(5));

    nlohmann::json request_body = {
        {"name", "Test User"},
        {"age", 25},
        {"email", "test@example.com"}
    };

    try {
        auto response = RunAsync([&]() -> boost::asio::awaitable<lightlib::Response> {
            co_return co_await client.post(base_url + "/test/echo", request_body);
            });

        EXPECT_EQ(response.result_int(), 200);
        EXPECT_EQ(response[boost::beast::http::field::content_type], "application/json");

        auto response_json = nlohmann::json::parse(response.body());
        EXPECT_EQ(response_json["name"], "Test User");
        EXPECT_EQ(response_json["age"], 25);
        EXPECT_EQ(response_json["email"], "test@example.com");
    }
    catch (const std::exception& e) {
        FAIL() << "Request failed: " << e.what();
    }
}

TEST_F(RoutingTest, NotFound) {
    std::string host = host_global;
    std::string port = std::to_string(port_global);
    std::string base_url = "http://" + host + ":" + port;

    lightlib::HttpClient client;
    client.set_verify_ssl(false);
    client.set_timeout(std::chrono::seconds(5));

    try {
        auto response = RunAsync([&]() -> boost::asio::awaitable<lightlib::Response> {
            co_return co_await client.get(base_url + "/nonexistent");
            });

        EXPECT_EQ(response.result_int(), 404);
    }
    catch (const std::exception& e) {
        FAIL() << "Request failed: " << e.what();
    }
}

TEST_F(RoutingTest, ResponseTime) {
    std::string host = host_global;
    std::string port = std::to_string(port_global); 
    std::string base_url = "http://" + host + ":" + port;

    lightlib::HttpClient client;
    client.set_verify_ssl(false);
    client.set_timeout(std::chrono::seconds(5));

    auto start = std::chrono::steady_clock::now();

    try {
        auto response = RunAsync([&]() -> boost::asio::awaitable<lightlib::Response> {
            co_return co_await client.get(base_url + "/test");
            });

        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        EXPECT_EQ(response.result_int(), 200);
        EXPECT_LT(duration.count(), 1000);
    }
    catch (const std::exception& e) {
        FAIL() << "Request failed: " << e.what();
    }
}

TEST_F(RoutingTest, MultipleRequests) {
    std::string host = host_global;
    std::string port = std::to_string(port_global);
    std::string base_url = "http://" + host + ":" + port;

    lightlib::HttpClient client;
    client.set_verify_ssl(false);
    client.set_timeout(std::chrono::seconds(5));

    boost::asio::io_context io_context;
    std::vector<std::future<lightlib::Response>> futures;

    for (int i = 0; i < 10; ++i) {
        auto future = boost::asio::co_spawn(
            io_context,
            [&]() -> boost::asio::awaitable<lightlib::Response> {
                co_return co_await client.get(base_url + "/test");
            },
            boost::asio::use_future
        );
        futures.push_back(std::move(future));
    }

    std::thread io_thread([&io_context]() { io_context.run(); });

    for (auto& future : futures) {
        try {
            auto response = future.get();
            EXPECT_EQ(response.result_int(), 200);
            EXPECT_EQ(response.body(), "TestController show method called");
        }
        catch (const std::exception& e) {
            FAIL() << "Request failed: " << e.what();
        }
    }

    io_context.stop();
    io_thread.join();
}