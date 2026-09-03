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
#include <string>
#include <regex>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <functional>
#include <boost/asio/awaitable.hpp>
#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace http = beast::http;

using Request = http::request<http::string_body>;
using Response = http::response<http::string_body>;
using Params = std::unordered_map<std::string, std::string>;

namespace brazier {

    class Route {
        std::regex pathRegex_;
        std::vector<std::string> paramNames_;
        std::string originalPath_;
        std::function<boost::asio::awaitable<void>(const Request&, Response&, const Params&)> handler_;

    public:
        Route(const std::string& path,
            std::function<boost::asio::awaitable<void>(const Request&, Response&, const Params&)> handler);

        bool match(const std::string& target, Params& outParams) const;
        boost::asio::awaitable<void> process(const Request& req, Response& res, const Params& params) const;
        const std::string& path() const;

    private:
        static std::regex convertPathToRegex(const std::string& path);
        static std::vector<std::string> extractParamNames(const std::string& path);
    };
}