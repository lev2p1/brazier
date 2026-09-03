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

#include <regex>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>
#include "../vendor/WebSocket/WebSocketSession.hpp"

namespace brazier {

    using Params = std::unordered_map<std::string, std::string>;

    class WebSocketRoute {
    public:
        using ConnectHandler = std::function<void(std::shared_ptr<WebSocketSession>, const Params&)>;

        WebSocketRoute(const std::string& path, ConnectHandler handler);

        bool match(const std::string& target, Params& outParams) const;
        void on_connect(std::shared_ptr<WebSocketSession> session, const Params& params) const;

        const std::string& path() const;

        void set_message_handler(WebSocketSession::MessageHandler handler);
        void set_binary_handler(WebSocketSession::BinaryHandler handler);
        void set_close_handler(WebSocketSession::CloseHandler handler);
        void set_error_handler(WebSocketSession::ErrorHandler handler);

        WebSocketSession::MessageHandler get_message_handler() const;
        WebSocketSession::BinaryHandler get_binary_handler() const;
        WebSocketSession::CloseHandler get_close_handler() const;
        WebSocketSession::ErrorHandler get_error_handler() const;

    private:
        std::regex convertPathToRegex(const std::string& path);
        std::vector<std::string> extractParamNames(const std::string& path);

        std::string originalPath_;
        std::regex pathRegex_;
        std::vector<std::string> paramNames_;
        ConnectHandler connect_handler_;

        WebSocketSession::MessageHandler message_handler_;
        WebSocketSession::BinaryHandler binary_handler_;
        WebSocketSession::CloseHandler close_handler_;
        WebSocketSession::ErrorHandler error_handler_;
    };

}