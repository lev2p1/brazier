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

#include "../../include/brazier/Router/WSRoute.hpp"
#include <sstream>

namespace brazier {

    WebSocketRoute::WebSocketRoute(const std::string& path, ConnectHandler handler)
        : originalPath_(path)
        , connect_handler_(std::move(handler)) {
        pathRegex_ = convertPathToRegex(path);
        paramNames_ = extractParamNames(path);
    }

    std::regex WebSocketRoute::convertPathToRegex(const std::string& path) {
        std::string regexStr = "^";
        std::istringstream stream(path);
        std::string token;

        while (std::getline(stream, token, '/')) {
            if (token.empty()) continue;

            if (token[0] == ':') {
                regexStr += "/([^/]+)";
            }
            else {
                regexStr += "/" + token;
            }
        }

        if (regexStr == "^") {
            regexStr += "/?$";
        }
        else {
            regexStr += "$";
        }

        return std::regex(regexStr);
    }

    std::vector<std::string> WebSocketRoute::extractParamNames(const std::string& path) {
        std::vector<std::string> names;
        std::istringstream stream(path);
        std::string token;

        while (std::getline(stream, token, '/')) {
            if (!token.empty() && token[0] == ':') {
                names.push_back(token.substr(1));
            }
        }
        return names;
    }

    bool WebSocketRoute::match(const std::string& target, Params& outParams) const {
        std::string pathOnly = target;
        size_t queryPos = target.find('?');

        if (queryPos != std::string::npos) {
            pathOnly = target.substr(0, queryPos);
        }

        std::smatch match;
        if (std::regex_match(pathOnly, match, pathRegex_)) {
            for (size_t i = 0; i < paramNames_.size(); ++i) {
                outParams[paramNames_[i]] = match[i + 1];
            }
            return true;
        }
        return false;
    }

    void WebSocketRoute::on_connect(std::shared_ptr<WebSocketSession> session, const Params& params) const {
        if (connect_handler_) {
            connect_handler_(session, params);
        }
    }

    const std::string& WebSocketRoute::path() const {
        return originalPath_;
    }

    void WebSocketRoute::set_message_handler(WebSocketSession::MessageHandler handler) {
        message_handler_ = std::move(handler);
    }

    void WebSocketRoute::set_binary_handler(WebSocketSession::BinaryHandler handler) {
        binary_handler_ = std::move(handler);
    }

    void WebSocketRoute::set_close_handler(WebSocketSession::CloseHandler handler) {
        close_handler_ = std::move(handler);
    }

    void WebSocketRoute::set_error_handler(WebSocketSession::ErrorHandler handler) {
        error_handler_ = std::move(handler);
    }

    WebSocketSession::MessageHandler WebSocketRoute::get_message_handler() const {
        return message_handler_;
    }

    WebSocketSession::BinaryHandler WebSocketRoute::get_binary_handler() const {
        return binary_handler_;
    }

    WebSocketSession::CloseHandler WebSocketRoute::get_close_handler() const {
        return close_handler_;
    }

    WebSocketSession::ErrorHandler WebSocketRoute::get_error_handler() const {
        return error_handler_;
    }

}