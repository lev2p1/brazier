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

#include "../include/brazier/App/Http/Helpers/Code.hpp"

using namespace brazier;

std::string Code::generateRandomCode(size_t length) {
    const std::string characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::string code;
    code.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        code += characters[rand() % characters.size()];
    }
    return code;
}

std::string Code::generateNumericCode(size_t length) {
    const std::string characters = "0123456789";
    std::string code;
    code.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        code += characters[rand() % characters.size()];
    }
    return code;
}

std::vector<std::string> Code::generateMultipleCodes(size_t count, size_t length, bool numeric) {
    std::vector<std::string> codes;
    codes.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        if (numeric) {
            codes.push_back(generateNumericCode(length));
        }
        else {
            codes.push_back(generateRandomCode(length));
        }
    }
    return codes;
}

std::string Code::base64_encode(const std::string& bytes) {
    std::string result;
    result.resize(boost::beast::detail::base64::encoded_size(bytes.size()));

    const auto encoded_size = boost::beast::detail::base64::encode(
        result.data(), bytes.data(), bytes.size()
    );

    result.resize(encoded_size);
    return result;
}