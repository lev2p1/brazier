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
#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <thread>
#include <future>
#include <boost/asio/thread_pool.hpp>

namespace brazier {

    namespace fs = std::filesystem;

    class Storage {
    private:
        Storage() = default;
        Storage(const Storage&) = delete;
        Storage& operator=(const Storage&) = delete;

        std::string rootPath_;
        std::shared_ptr<boost::asio::thread_pool> ioPool_;

    public:
        static Storage& getInstance();
        void initAsync(size_t threadCount = std::thread::hardware_concurrency());
        void setRootPath(const std::string& path);
        std::string getRootPath() const;

        void put(const std::string& path, const std::string& content);
        std::string get(const std::string& path);
        bool exists(const std::string& path) const;
        void deleteFile(const std::string& path);
        void copy(const std::string& source, const std::string& destination);
        void move(const std::string& source, const std::string& destination);
        boost::asio::awaitable<void> putAsync(const std::string& path, const std::string& content);
        boost::asio::awaitable<std::string> getAsync(const std::string& path);
        boost::asio::awaitable<bool> existsAsync(const std::string& path) const;
        boost::asio::awaitable<void> deleteFileAsync(const std::string& path);
        boost::asio::awaitable<void> copyAsync(const std::string& source, const std::string& destination);
        boost::asio::awaitable<void> moveAsync(const std::string& source, const std::string& destination);
    };

}