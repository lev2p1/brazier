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
#include "../include/brazier/App/Http/Helpers/Validator.hpp"
using namespace brazier;

bool Validator::password(const std::string& password) {
    return passwordError(password).empty();
}

std::string Validator::passwordError(const std::string& password) {
    if (password.size() < 8) return "Password is too short (minimum 8 characters).";
    if (password.size() > 20) return "Password is too long (maximum 20 characters).";
    bool hasDigit = false;
    bool hasUpper = false;
    bool hasSpecial = false;
    for (char ch : password) {
        if (std::isdigit(ch)) hasDigit = true;
        else if (std::isupper(ch)) hasUpper = true;
        else if (!std::isalnum(ch)) hasSpecial = true;
    }
    if (!hasDigit) return "Password must contain at least one digit.";
    if (!hasUpper) return "Password must contain at least one uppercase letter.";
    if (!hasSpecial) return "Password must contain at least one special character.";
    return "";
}

bool Validator::email(const std::string& email) {
    const std::regex pattern(R"(^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$)");
    return std::regex_match(email, pattern);
}
