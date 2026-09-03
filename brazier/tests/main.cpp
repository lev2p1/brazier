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

#include "main.h"

std::shared_ptr<brazier::Server> g_test_server;
std::atomic<bool> g_server_ready{ false };
std::thread g_server_thread;
int port_global;
std::string host_global;

bool WaitForServer(int port, int max_attempts = 30) {
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::socket socket(io_context);
    boost::asio::ip::tcp::endpoint endpoint(
        boost::asio::ip::make_address(host_global),
        port
    );

    for (int i = 0; i < max_attempts; ++i) {
        boost::system::error_code ec;
        socket.connect(endpoint, ec);
        if (!ec) {
            socket.close();
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    return false;
}

int main(int argc, char** argv) {
    try {
        testing::InitGoogleTest(&argc, argv);
        brazier::ConfigManager::initGlobal("config_test.json");
        brazier::global_config->setAutoSave(false);

        std::string server_host = brazier::global_config->get("server.host", "127.0.0.1");
        int server_port = brazier::global_config->get<int>("server.port", 3502);

        if (server_host == "0.0.0.0") {
			server_host = "127.0.0.1";
        }

		host_global = server_host;
		port_global = server_port;

        g_test_server = std::make_shared<brazier::Server>(server_host, server_port);

        g_server_thread = std::thread([]() {
            try {
                if (!g_test_server->initialize()) {
                    brazier::Logger::log("Failed to initialize test server", "ERROR");
                    g_server_ready = false;
                    return;
                }
                g_server_ready = true;
                brazier::Logger::log("Test server initialized successfully", "INFO");
                g_test_server->run();
            }
            catch (const std::exception& e) {
                brazier::Logger::log("Server error: " + std::string(e.what()), "ERROR");
                g_server_ready = false;
            }
            });

        if (!WaitForServer(server_port)) {
            brazier::Logger::log("Server failed to start within timeout", "ERROR");
            return -1;
        }

        int result = RUN_ALL_TESTS();
        
        if (g_test_server) {
            g_test_server->stop();
        }
        if (g_server_thread.joinable()) {
            g_server_thread.join();
        }

        return result;

    }
    catch (const std::exception& e) {
        brazier::Logger::log("Exception: " + std::string(e.what()), "ERROR");
        return -1;
    }
}