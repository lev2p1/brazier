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

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/cancel_after.hpp>        
#include <boost/config.hpp>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <utility>                            
#include <vector>

#include "vendor/Handlers/ENV.hpp"
#include "Database/Queue.hpp"
#include "Database/Cache.hpp"
#include "Database/Migrations/MigrationManager.hpp"
#include "Router/RouterRegisterer.hpp"
#include "Router/Router.hpp"
#include "Engine.hpp"
#include "Filesystem/Filesystem.hpp"
#include "vendor/ConfigManager.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;
using namespace std::chrono_literals;

namespace brazier {

    class HttpsServer {
    public:
        struct TlsConfig {
            std::string cert_file;    
            std::string key_file;     
            std::string ca_file;      

            std::vector<std::pair<std::string, std::string>> conf;

            bool require_client_cert = false;
            bool verify_client_cert = false;

            std::chrono::seconds handshake_timeout{ 15 };
        };

    private:
        net::io_context io_;
        ssl::context    ssl_ctx_{ ssl::context::tls_server };
        tcp::acceptor   acceptor_;

        unsigned short port_;
        std::string    host_;

        std::vector<std::thread> threads_;
        std::unique_ptr<net::executor_work_guard<net::io_context::executor_type>> work_guard_;

        std::atomic<int> connection_count_{ 0 };
        std::atomic<int> total_requests_{ 0 };

        std::thread        stats_thread_;
        std::atomic<bool>  shutdown_flag_{ false };

        TlsConfig tls_;
        bool      tls_config_from_user_ = false;

    public:
        HttpsServer(const std::string& host, unsigned short port);
        HttpsServer(const std::string& host, unsigned short port, const TlsConfig& tls);

        void setTlsConfig(const TlsConfig& tls);

        bool initialize();

        void run();
        void stop();

        unsigned short     getPort() const;
        const std::string& getHost() const;

    private:
        void initializeConnections();

        void load_tls_config_from_global();
        void configure_tls();
        void apply_ssl_conf();

        net::awaitable<void> handle_connection(tcp::socket socket);
        net::awaitable<void> accept_loop();
    };

}