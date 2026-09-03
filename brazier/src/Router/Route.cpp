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

#include "../../include/brazier/Router/Route.hpp"

using namespace brazier;

Route::Route(const std::string& path,
    std::function<boost::asio::awaitable<void>(const Request&, Response&, const Params&)> handler)
    : originalPath_(path), handler_(std::move(handler)) {
    pathRegex_ = convertPathToRegex(path);
    paramNames_ = extractParamNames(path);
}

bool Route::match(const std::string& target, Params& outParams) const {
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

boost::asio::awaitable<void> Route::process(const Request& req, Response& res, const Params& params) const {
    if (handler_) {
        co_await handler_(req, res, params);
    }
}

const std::string& Route::path() const {
    return originalPath_;
}

std::regex Route::convertPathToRegex(const std::string& path) {
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

std::vector<std::string> Route::extractParamNames(const std::string& path) {
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