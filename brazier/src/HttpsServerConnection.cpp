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

net::awaitable<void> brazier::HttpsServer::accept_and_dispatch(
    tcp::acceptor& acceptor, int worker_begin)
{
    const int worker_count =
        static_cast<int>(io_contexts_.size()) - worker_begin;
    int next = 0;

    for (;;) {
        if (shutting_down_.load(std::memory_order_acquire)) {
            co_return;
        }

        beast::error_code ec;
        tcp::socket socket = co_await acceptor.async_accept(
            net::redirect_error(net::use_awaitable, ec));

        if (ec) {
            if (ec == net::error::operation_aborted ||
                shutting_down_.load(std::memory_order_acquire)) {
                co_return;
            }
            Logger::log("Accept error: " + ec.message(), "ERROR");
            continue;
        }

        const int prev = connection_count_.fetch_add(1,
            std::memory_order_acq_rel);
        if (prev >= max_connections_) {
            connection_count_.fetch_sub(1, std::memory_order_acq_rel);
            Logger::log("Connection limit reached (" +
                std::to_string(prev) + "/" +
                std::to_string(max_connections_) + "), rejecting",
                "WARNING");
            boost::system::error_code ignore;
            socket.close(ignore);
            continue;
        }

        const int idx = worker_begin + (next++ % worker_count);

        net::co_spawn(*io_contexts_[idx],
            handle_connection(std::move(socket)),
            net::detached);
    }
}

net::awaitable<void> brazier::HttpsServer::accept_loop(
    tcp::acceptor& acceptor)
{
    for (;;) {
        if (shutting_down_.load(std::memory_order_acquire)) {
            co_return;
        }

        beast::error_code ec;
        tcp::socket socket = co_await acceptor.async_accept(
            net::redirect_error(net::use_awaitable, ec));

        if (ec) {
            if (ec == net::error::operation_aborted ||
                shutting_down_.load(std::memory_order_acquire)) {
                co_return;
            }
            Logger::log("Accept error: " + ec.message(), "ERROR");
            continue;
        }

        const int prev = connection_count_.fetch_add(1,
            std::memory_order_acq_rel);
        if (prev >= max_connections_) {
            connection_count_.fetch_sub(1, std::memory_order_acq_rel);
            Logger::log("Connection limit reached (" +
                std::to_string(prev) + "/" +
                std::to_string(max_connections_) + "), rejecting",
                "WARNING");
            boost::system::error_code ignore;
            socket.close(ignore);
            continue;
        }

        net::co_spawn(acceptor.get_executor(),
            handle_connection(std::move(socket)),
            net::detached);
    }
}

net::awaitable<void> brazier::HttpsServer::handle_connection(
    tcp::socket socket)
{
    ConnectionGuard guard(*this);

    try {
        socket.set_option(tcp::no_delay(true));
        socket.set_option(boost::asio::socket_base::keep_alive(true));

        auto ctx = get_ssl_ctx();
        ssl::stream<tcp::socket> stream(std::move(socket), *ctx);

        {
            net::steady_timer hs_timer(co_await net::this_coro::executor);
            hs_timer.expires_after(tls_.handshake_timeout);
            hs_timer.async_wait([&](beast::error_code ec) {
                if (!ec) {
                    beast::error_code ignore;
                    stream.next_layer().close(ignore);
                }
                });

            beast::error_code hs_ec;
            co_await stream.async_handshake(
                ssl::stream_base::server,
                net::redirect_error(net::use_awaitable, hs_ec));

            hs_timer.cancel();

            if (hs_ec) {
                Logger::log("TLS handshake failed: " + hs_ec.message(),
                    "WARNING");
                co_return;
            }
        }

        std::optional<http::request_parser<http::string_body>> parser;

        http::response<http::string_body> res;
        beast::flat_buffer buffer;
        bool keep_alive = true;

        net::steady_timer idle_timer(co_await net::this_coro::executor);
        const auto IDLE_TIMEOUT = keep_alive_timeout_;
        bool timed_out = false;

        auto reset_timer = [&]() {
            idle_timer.expires_after(IDLE_TIMEOUT);
            idle_timer.async_wait([&](beast::error_code ec) {
                if (!ec) {
                    timed_out = true;
                    beast::error_code ignore;
                    stream.next_layer().close(ignore);
                }
                });
            };

        reset_timer();

        while (keep_alive && !timed_out) {
            parser.emplace();
            parser->body_limit(
                static_cast<std::uint64_t>(max_body_size_));
            parser->header_limit(
                static_cast<std::uint32_t>(max_header_size_));

            beast::error_code ec;

            co_await http::async_read(
                stream, buffer, *parser,
                net::redirect_error(net::use_awaitable, ec));

            if (ec == http::error::body_limit) {
                http::response<http::string_body> err{
                    http::status::payload_too_large, 11 };
                err.set(http::field::content_type, "text/plain");
                err.set(http::field::connection, "close");
                err.body() = "Payload too large";
                err.prepare_payload();

                beast::error_code write_ec;
                co_await http::async_write(
                    stream, err,
                    net::redirect_error(net::use_awaitable, write_ec));
                break;
            }

            if (ec == http::error::header_limit) {
                http::response<http::string_body> err{
                    http::status::request_header_fields_too_large, 11 };
                err.set(http::field::content_type, "text/plain");
                err.set(http::field::connection, "close");
                err.body() = "Header too large";
                err.prepare_payload();

                beast::error_code write_ec;
                co_await http::async_write(
                    stream, err,
                    net::redirect_error(net::use_awaitable, write_ec));
                break;
            }

            if (ec == http::error::end_of_stream) break;
            if (ec) break;

            reset_timer();

            http::request<http::string_body> req = parser->release();
            total_requests_.fetch_add(1, std::memory_order_relaxed);
            keep_alive = req.keep_alive();

            res.clear();
            res.version(req.version());
            res.keep_alive(keep_alive);

            try {
                co_await Router::handle_request(req, res);
            }
            catch (const std::exception& e) {
                Logger::log("Router error: " + std::string(e.what()),
                    "ERROR");
                res.result(http::status::internal_server_error);
                res.set(http::field::content_type, "application/json");
                res.body() = R"({"error":"internal server error"})";
                keep_alive = false;
            }

            res.prepare_payload();

            std::string flat;
            flat.reserve(256 + static_headers_.size() +
                res.body().size());

            flat += "HTTP/1.1 ";
            flat += std::to_string(res.result_int());
            flat += ' ';
            flat += res.reason();
            flat += "\r\n";

            flat += static_headers_;

            flat += "Connection: ";
            flat += keep_alive ? "keep-alive\r\n" : "close\r\n";

            for (const auto& field : res.base()) {
                flat += field.name_string();
                flat += ": ";
                flat += field.value();
                flat += "\r\n";
            }
            flat += "\r\n";
            flat += res.body();

            ec.clear();
            co_await net::async_write(
                stream, net::buffer(flat),
                net::redirect_error(net::use_awaitable, ec));

            if (ec) break;

            buffer.consume(buffer.size());
            if (!keep_alive) break;
        }

        idle_timer.cancel();

        {
            net::steady_timer sd_timer(co_await net::this_coro::executor);
            sd_timer.expires_after(std::chrono::seconds(2));
            sd_timer.async_wait([&](beast::error_code ec) {
                if (!ec) {
                    beast::error_code ignore;
                    stream.next_layer().close(ignore);
                }
                });

            beast::error_code sd_ec;
            co_await stream.async_shutdown(
                net::redirect_error(net::use_awaitable, sd_ec));

            sd_timer.cancel();
        }

        {
            beast::error_code ec;
            stream.next_layer().shutdown(tcp::socket::shutdown_both, ec);
            stream.next_layer().close(ec);
        }
    }
    catch (const boost::system::system_error& e) {
        auto code = e.code();
        if (code != net::error::connection_reset &&
            code != net::error::connection_aborted &&
            code != net::error::eof &&
            code != net::error::operation_aborted &&
            code != net::error::broken_pipe &&
            code != ssl::error::stream_truncated) {
            Logger::log("HTTPS connection error: " +
                std::string(e.what()), "ERROR");
        }
    }
    catch (const std::exception& e) {
        Logger::log("HTTPS connection error: " + std::string(e.what()),
            "ERROR");
    }
    catch (...) {
        Logger::log("Unknown HTTPS connection error", "ERROR");
    }

    co_return;
}