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

#include "../include/brazier/Core"
#include "../include/brazier/DB"
#include "../include/brazier/Http"
#include "../include/brazier/Engine.hpp"

int main() {
    try {
        brazier::ConfigManager::initGlobal("config_test.json");
        brazier::global_config->setAutoSave(false);

        std::string https_server_host =
            brazier::global_config->get("https_server.host", "0.0.0.0");
        int https_server_port =
            brazier::global_config->get("https_server.port", 8443);

        brazier::Logger::log("HTTPS server: " + https_server_host + ":" +
            std::to_string(https_server_port), "INFO");

        brazier::HttpsServer https_server(https_server_host, https_server_port);

        if (!https_server.initialize()) return 1;

        https_server.run();  

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        brazier::Logger::log(std::string(e.what()), "ERROR");
        return 1;
    }
}