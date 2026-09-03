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

#include "WSRoute.hpp"
#include <vector>
#include <memory>

namespace brazier {

    class WebSocketRouter {
    public:
        static WebSocketRouter& instance();

        void add_route(const std::string& path, WebSocketRoute::ConnectHandler handler);

        WebSocketRoute* find_route(const std::string& target, Params& outParams);

        void set_global_message_handler(WebSocketSession::MessageHandler handler);
        void set_global_binary_handler(WebSocketSession::BinaryHandler handler);
        void set_global_close_handler(WebSocketSession::CloseHandler handler);
        void set_global_error_handler(WebSocketSession::ErrorHandler handler);

        WebSocketSession::MessageHandler get_global_message_handler() const;
        WebSocketSession::BinaryHandler get_global_binary_handler() const;
        WebSocketSession::CloseHandler get_global_close_handler() const;
        WebSocketSession::ErrorHandler get_global_error_handler() const;

        void clear();

    private:
        WebSocketRouter() = default;

        std::vector<WebSocketRoute> routes_;

        WebSocketSession::MessageHandler global_message_handler_;
        WebSocketSession::BinaryHandler global_binary_handler_;
        WebSocketSession::CloseHandler global_close_handler_;
        WebSocketSession::ErrorHandler global_error_handler_;
    };

}