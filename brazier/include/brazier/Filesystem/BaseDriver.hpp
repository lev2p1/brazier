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
#include <memory>
#include <thread>
#include <boost/asio/awaitable.hpp>

namespace brazier {

    class BaseDriver {
    public:
        virtual ~BaseDriver() = default;
        virtual void initAsync(size_t threadCount = std::thread::hardware_concurrency()) = 0;
        virtual void setRootPath(const std::string& path) = 0;
        virtual std::string getRootPath() const = 0;
        virtual void put(const std::string& path, const std::string& content) = 0;
        virtual std::string get(const std::string& path) = 0;
        virtual bool exists(const std::string& path) const = 0;
        virtual void deleteFile(const std::string& path) = 0;
        virtual void copy(const std::string& source, const std::string& destination) = 0;
        virtual void move(const std::string& source, const std::string& destination) = 0;
        virtual boost::asio::awaitable<void> putAsync(const std::string& path, const std::string& content) = 0;
        virtual boost::asio::awaitable<std::string> getAsync(const std::string& path) = 0;
        virtual boost::asio::awaitable<bool> existsAsync(const std::string& path) const = 0;
        virtual boost::asio::awaitable<void> deleteFileAsync(const std::string& path) = 0;
        virtual boost::asio::awaitable<void> copyAsync(const std::string& source, const std::string& destination) = 0;
        virtual boost::asio::awaitable<void> moveAsync(const std::string& source, const std::string& destination) = 0;
        virtual std::string getDriverType() const = 0;
        virtual bool isAsyncSupported() const = 0;
    };

    constexpr auto S3 = "S3";
    constexpr auto File = "Filesystem";
    constexpr auto FTP = "FTP";
    constexpr auto WebDAV = "WebDAV";
}