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

#include "../include/brazier/App/Http/Helpers/Cookie.hpp"

using namespace brazier;

std::map<std::string, std::string> Cookie::parseCookies(const std::string& cookieHeader) {
    std::map<std::string, std::string> cookies;
    std::istringstream stream(cookieHeader);
    std::string pair;

    while (std::getline(stream, pair, ';')) {
        auto pos = pair.find('=');
        if (pos != std::string::npos) {
            std::string key = pair.substr(0, pos);
            std::string value = pair.substr(pos + 1);
            key.erase(0, key.find_first_not_of(" \t"));
            value.erase(0, value.find_first_not_of(" \t"));
            cookies[key] = value;
        }
    }

    return cookies;
}

void Cookie::set(Response& res, std::map<std::string, std::string> cookies) {
    for (const auto& [name, value] : cookies) {
        res.insert(http::field::set_cookie, name + "=" + value + "; Path=/; HttpOnly; Secure; SameSite=None");
    }
}