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

#include "../../include/brazier/Database/SQLString.hpp"

using namespace brazier;

std::string SQLString::EscapeString(PGconn* conn, const std::string& input) {
    char* escaped = PQescapeLiteral(conn, input.c_str(), input.size());
    if (!escaped) {
        return "'ERROR: FAILED TO ESCAPE STRING'";
    }
    std::string result(escaped);
    PQfreemem(escaped);
    return result;
}

std::string SQLString::EscapeIdentifier(PGconn* conn, const std::string& input) {
    char* escaped = PQescapeIdentifier(conn, input.c_str(), input.size());
    if (!escaped) {
        return "\"ERROR: FAILED TO ESCAPE IDENTIFIER\"";
    }
    std::string result(escaped);
    PQfreemem(escaped);
    return result;
}