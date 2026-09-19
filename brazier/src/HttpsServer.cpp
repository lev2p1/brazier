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

brazier::HttpsServer::HttpsServer(const std::string& host, unsigned short port)
    : acceptor_(io_)
    , port_(port), host_(host) {
    work_guard_ = std::make_unique<
        net::executor_work_guard<net::io_context::executor_type>>(io_.get_executor());
}

brazier::HttpsServer::HttpsServer(const std::string& host, unsigned short port,
    const TlsConfig& tls)
    : acceptor_(io_)
    , port_(port), host_(host), tls_(tls), tls_config_from_user_(true) {
    work_guard_ = std::make_unique<
        net::executor_work_guard<net::io_context::executor_type>>(io_.get_executor());
}

void brazier::HttpsServer::setTlsConfig(const TlsConfig& tls) {
    tls_ = tls;
    tls_config_from_user_ = true;
}


void brazier::HttpsServer::load_tls_config_from_global() {
    if (tls_config_from_user_) return;

    tls_.cert_file = global_config->get("https_server.tls.cert_file",
        std::string("server.crt"));
    tls_.key_file = global_config->get("https_server.tls.key_file",
        std::string("server.key"));
    tls_.ca_file = global_config->get("https_server.tls.ca_file",
        std::string(""));

    tls_.require_client_cert =
        global_config->get("https_server.tls.require_client_cert", false);
    tls_.verify_client_cert =
        global_config->get("https_server.tls.verify_client_cert", false);

    tls_.handshake_timeout = std::chrono::seconds(
        global_config->get("https_server.tls.handshake_timeout", 15));

    try {
        json conf = global_config->getJson("https_server.tls.conf");
        if (conf.is_array()) {
            for (const auto& item : conf) {
                if (!item.is_array() || item.empty() || item.size() > 2) {
                    throw std::runtime_error(
                        "https_server.tls.conf: each entry must be [command] "
                        "or [command, value]");
                }
                std::string cmd = item[0].get<std::string>();
                std::string val = item.size() > 1 ? item[1].get<std::string>() : "";
                tls_.conf.emplace_back(std::move(cmd), std::move(val));
            }
        }
    }
    catch (const std::exception& e) {
        Logger::log("https_server.tls.conf not loaded: " + std::string(e.what()),
            "WARNING");
    }
}

void brazier::HttpsServer::apply_ssl_conf() {
    if (tls_.conf.empty()) return;

    SSL_CONF_CTX* cctx = SSL_CONF_CTX_new();
    if (!cctx) {
        throw std::runtime_error("SSL_CONF_CTX_new failed");
    }

    SSL_CONF_CTX_set_flags(cctx, SSL_CONF_FLAG_SERVER | SSL_CONF_FLAG_CERTIFICATE);
    SSL_CONF_CTX_set_ssl_ctx(cctx, ssl_ctx_.native_handle());

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

void brazier::HttpsServer::configure_tls() {
    ssl_ctx_.set_options(
        ssl::context::default_workarounds
        | ssl::context::no_sslv2
        | ssl::context::no_sslv3
        | ssl::context::single_dh_use);

    apply_ssl_conf();

    ssl_ctx_.use_certificate_chain_file(tls_.cert_file);
    ssl_ctx_.use_private_key_file(tls_.key_file, ssl::context::pem);

    if (tls_.require_client_cert || tls_.verify_client_cert) {
        if (!tls_.ca_file.empty()) {
            ssl_ctx_.load_verify_file(tls_.ca_file);
        }
        auto mode = ssl::verify_peer;
        if (tls_.require_client_cert) {
            mode |= ssl::verify_fail_if_no_peer_cert;
        }
        ssl_ctx_.set_verify_mode(mode);
    }
    else {
        ssl_ctx_.set_verify_mode(ssl::verify_none);
    }
}


bool brazier::HttpsServer::initialize() {
    try {
        Logger::init("debug.log");
        Logger::registerSignalHandlers();

        json drivers = global_config->getJson("filesystem.drivers");

        for (auto& [name, cfg] : drivers.items()) {
            if (name == "default") continue;

            auto driver = std::make_shared<brazier::FileDriver>();
            driver->setRootPath(cfg.value("root", "./"));
            driver->initAsync();

            StorageManager::getInstance().registerDriver(name, driver);
        }

        std::string def = global_config->getNested<std::string>("filesystem.default",
            "local");
        if (StorageManager::getInstance().hasDriver(def)) {
            StorageManager::getInstance().setDefaultDriver(def);
        }

        load_tls_config_from_global();
        configure_tls();

        tcp::endpoint endpoint(net::ip::make_address(host_), port_);
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(tcp::acceptor::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen();

        initializeConnections();
        RouterRegisterer::init(io_);
        Engine::init(io_);

        Logger::log("HTTPS server initialized on " + host_ + ":" +
            std::to_string(port_) + " [TLS]", "SUCCESS");
        return true;
    }
    catch (const std::exception& e) {
        Logger::log("HTTPS initialization failed: " + std::string(e.what()), "ERROR");
        return false;
    }
}

void brazier::HttpsServer::initializeConnections() {
    try {
        Queue::connect(global_config->get("nosql.host", "127.0.0.1"),
            global_config->get("nosql.port", 6379));
    }
    catch (const std::exception& e) {
        Logger::log("Connection to queue failed: " + std::string(e.what()), "ERROR");
    }

    try {
        Cache::connect(global_config->get("redis.host", "127.0.0.1"),
            global_config->get("redis.port", 6379));
    }
    catch (const std::exception& e) {
        Logger::log("Connection to NOSQL database failed: " + std::string(e.what()),
            "ERROR");
    }

    try {
        Database db;
        (new MigrationManager(db))->Initialize();
    }
    catch (const std::exception& e) {
        Logger::log("Database migration failed: " + std::string(e.what()), "ERROR");
    }
}

void brazier::HttpsServer::run() {
    try {
        net::co_spawn(io_, accept_loop(), net::detached);

        int threads_count = std::thread::hardware_concurrency();
        if (threads_count == 0) threads_count = 1;

        Logger::log("Starting " + std::to_string(threads_count) +
            " HTTPS worker threads", "INFO");

        for (int i = 0; i < threads_count; ++i) {
            threads_.emplace_back([this] { io_.run(); });
        }

        shutdown_flag_.store(false, std::memory_order_release);
        stats_thread_ = std::thread([this] {
            while (!shutdown_flag_.load(std::memory_order_acquire)) {
                std::this_thread::sleep_for(10s);
                if (shutdown_flag_.load(std::memory_order_acquire)) break;
                Logger::log(
                    "HTTPS STATS - Active connections: " +
                    std::to_string(connection_count_.load()) +
                    ", Total requests: " + std::to_string(total_requests_.load()),
                    "INFO");
            }
            });

        for (auto& t : threads_) {
            if (t.joinable()) t.join();
        }
    }
    catch (const std::exception& e) {
        Logger::log("HTTPS server run failed: " + std::string(e.what()), "ERROR");
        throw;
    }
}

void brazier::HttpsServer::stop() {
    shutdown_flag_.store(true);
    work_guard_.reset();
    io_.stop();

    if (stats_thread_.joinable()) stats_thread_.join();
    Logger::log("HTTPS server stopped", "INFO");
}

unsigned short brazier::HttpsServer::getPort() const { return port_; }
const std::string& brazier::HttpsServer::getHost() const { return host_; }

net::awaitable<void> brazier::HttpsServer::accept_loop() {
    for (;;) {
        tcp::socket socket = co_await acceptor_.async_accept(net::use_awaitable);
        net::co_spawn(io_, handle_connection(std::move(socket)), net::detached);
    }
}

net::awaitable<void> brazier::HttpsServer::handle_connection(tcp::socket socket) {
    connection_count_.fetch_add(1, std::memory_order_relaxed);

    try {
        socket.set_option(tcp::no_delay(true));
        socket.set_option(boost::asio::socket_base::keep_alive(true));

        ssl::stream<tcp::socket> stream(std::move(socket), ssl_ctx_);

        beast::error_code ec;
        co_await stream.async_handshake(
            ssl::stream_base::server,
            net::cancel_after(
                tls_.handshake_timeout,
                net::redirect_error(net::use_awaitable, ec)));

        if (ec) {
            Logger::log("TLS handshake failed: " + ec.message(), "WARNING");
            connection_count_.fetch_sub(1, std::memory_order_relaxed);
            co_return;
        }

        http::request<http::string_body>  req;
        http::response<http::string_body> res;
        beast::flat_buffer buffer;
        bool keep_alive = true;

        const auto idle_timeout = std::chrono::seconds(
            global_config->get("keep-alive-timeout", 60));

        while (keep_alive) {
            req = {};
            ec.clear();

            co_await http::async_read(
                stream, buffer, req,
                net::cancel_after(
                    idle_timeout,
                    net::redirect_error(net::use_awaitable, ec)));

            if (ec == http::error::end_of_stream) break;
            if (ec == net::error::timed_out) {
                Logger::log("HTTPS idle timeout, closing connection", "INFO");
                break;
            }
            if (ec) {
                if (ec == net::error::operation_aborted) break;
                throw boost::system::system_error(ec);
            }

            total_requests_.fetch_add(1, std::memory_order_relaxed);
            keep_alive = req.keep_alive();

            res = {};
            res.version(req.version());
            res.keep_alive(keep_alive);
            res.set(http::field::connection, keep_alive ? "keep-alive" : "close");
            res.set(http::field::server, "brazier");
            res.set(http::field::strict_transport_security, "max-age=31536000");

            co_await Router::handle_request(req, res);

            if (res.body().empty() &&
                res.count(http::field::content_length) == 0) {
                res.content_length(0);
            }
            else if (!res.body().empty() &&
                res.count(http::field::content_length) == 0) {
                res.content_length(res.body().size());
            }
            res.prepare_payload();

            ec.clear();
            co_await http::async_write(
                stream, res,
                net::cancel_after(
                    idle_timeout,
                    net::redirect_error(net::use_awaitable, ec)));
            if (ec) break;

            buffer.consume(buffer.size());
            if (!keep_alive) break;
        }

        ec.clear();
        co_await stream.async_shutdown(
            net::cancel_after(
                std::chrono::seconds(5),
                net::redirect_error(net::use_awaitable, ec)));

    }
    catch (const boost::system::system_error& e) {
        auto code = e.code();

        if (code == net::error::connection_reset ||
            code == net::error::connection_aborted ||
            code == net::error::eof ||                     
            code == net::error::operation_aborted ||      
            code == net::error::broken_pipe ||             
            code == ssl::error::stream_truncated) {        
            Logger::log("HTTPS client disconnected", "DEBUG");
        }
        else {
            Logger::log("HTTPS connection error: " + std::string(e.what()), "ERROR");
        }
    }
    catch (const std::exception& e) {
        Logger::log("HTTPS connection error: " + std::string(e.what()), "ERROR");
    }
    catch (...) {
        Logger::log("Unknown HTTPS connection error", "ERROR");
    }

    connection_count_.fetch_sub(1, std::memory_order_relaxed);
    co_return;
}