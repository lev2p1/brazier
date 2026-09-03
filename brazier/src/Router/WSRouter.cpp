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

#include "../../include/brazier/Router/WSRouter.hpp"

namespace brazier {

    WebSocketRouter& WebSocketRouter::instance() {
        static WebSocketRouter router;
        return router;
    }

    void WebSocketRouter::add_route(const std::string& path, WebSocketRoute::ConnectHandler handler) {
        routes_.emplace_back(path, std::move(handler));
    }

    WebSocketRoute* WebSocketRouter::find_route(const std::string& target, Params& outParams) {
        for (auto& route : routes_) {
            if (route.match(target, outParams)) {
                return &route;
            }
        }
        return nullptr;
    }

    void WebSocketRouter::set_global_message_handler(WebSocketSession::MessageHandler handler) {
        global_message_handler_ = std::move(handler);
    }

    void WebSocketRouter::set_global_binary_handler(WebSocketSession::BinaryHandler handler) {
        global_binary_handler_ = std::move(handler);
    }

    void WebSocketRouter::set_global_close_handler(WebSocketSession::CloseHandler handler) {
        global_close_handler_ = std::move(handler);
    }

    void WebSocketRouter::set_global_error_handler(WebSocketSession::ErrorHandler handler) {
        global_error_handler_ = std::move(handler);
    }

    WebSocketSession::MessageHandler WebSocketRouter::get_global_message_handler() const {
        return global_message_handler_;
    }

    WebSocketSession::BinaryHandler WebSocketRouter::get_global_binary_handler() const {
        return global_binary_handler_;
    }

    WebSocketSession::CloseHandler WebSocketRouter::get_global_close_handler() const {
        return global_close_handler_;
    }

    WebSocketSession::ErrorHandler WebSocketRouter::get_global_error_handler() const {
        return global_error_handler_;
    }

    void WebSocketRouter::clear() {
        routes_.clear();
    }

}