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

#include "../include/brazier/vendor/Storage/Storage.hpp"
#include <boost/asio/post.hpp>

namespace brazier {

    Storage& Storage::getInstance() {
        static Storage instance;
        return instance;
    }

    void Storage::initAsync(size_t threadCount) {
        if (!ioPool_) {
            ioPool_ = std::make_shared<boost::asio::thread_pool>(threadCount);
        }
    }

    void Storage::setRootPath(const std::string& path) {
        rootPath_ = path;
        if (!fs::exists(rootPath_)) {
            fs::create_directories(rootPath_);
        }
    }

    std::string Storage::getRootPath() const {
        return rootPath_;
    }

    void Storage::put(const std::string& path, const std::string& content) {
        std::string fullPath = rootPath_ + "/" + path;
        auto parentPath = std::filesystem::path(fullPath).parent_path();
        std::error_code ec;
        std::filesystem::create_directories(parentPath, ec);

        if (ec) {
            throw std::runtime_error("Cannot create directories for: " + path);
        }

        std::ofstream file(fullPath, std::ios::out);

        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for writing: " + path);
        }

        file << content;
    }

    std::string Storage::get(const std::string& path) {
        std::string fullPath = rootPath_ + "/" + path;
        std::ifstream file(fullPath, std::ios::in);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for reading: " + path);
        }
        return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }

    bool Storage::exists(const std::string& path) const {
        return fs::exists(rootPath_ + "/" + path);
    }

    void Storage::deleteFile(const std::string& path) {
        if (!exists(path)) {
            throw std::runtime_error("File does not exist: " + path);
        }
        fs::remove(rootPath_ + "/" + path);
    }

    void Storage::copy(const std::string& source, const std::string& destination) {
        if (!exists(source)) {
            throw std::runtime_error("Source file does not exist: " + source);
        }
        fs::copy(rootPath_ + "/" + source, rootPath_ + "/" + destination);
    }

    void Storage::move(const std::string& source, const std::string& destination) {
        if (!exists(source)) {
            throw std::runtime_error("Source file does not exist: " + source);
        }
        fs::rename(rootPath_ + "/" + source, rootPath_ + "/" + destination);
    }

    boost::asio::awaitable<void> Storage::putAsync(const std::string& path, const std::string& content) {
        if (!ioPool_) {
            throw std::runtime_error("Storage not initialized for async operations. Call initAsync() first.");
        }

        auto executor = co_await boost::asio::this_coro::executor;

        co_await boost::asio::post(executor, boost::asio::use_awaitable);

        co_await boost::asio::post(*ioPool_, boost::asio::use_awaitable);

        try {
            put(path, content);
        }
        catch (const std::exception& e) {
            throw e;
        }

        co_return;
    }

    boost::asio::awaitable<std::string> Storage::getAsync(const std::string& path) {
        if (!ioPool_) {
            throw std::runtime_error("Storage not initialized for async operations. Call initAsync() first.");
        }

        co_await boost::asio::post(*ioPool_, boost::asio::use_awaitable);

        std::string result;
        try {
            result = get(path);
        }
        catch (const std::exception& e) {
            throw e;
        }

        co_return result;
    }

    boost::asio::awaitable<bool> Storage::existsAsync(const std::string& path) const {
        if (!ioPool_) {
            throw std::runtime_error("Storage not initialized for async operations. Call initAsync() first.");
        }

        co_await boost::asio::post(*ioPool_, boost::asio::use_awaitable);
        co_return exists(path);
    }

    boost::asio::awaitable<void> Storage::deleteFileAsync(const std::string& path) {
        if (!ioPool_) {
            throw std::runtime_error("Storage not initialized for async operations. Call initAsync() first.");
        }

        co_await boost::asio::post(*ioPool_, boost::asio::use_awaitable);
        deleteFile(path);
        co_return;
    }

    boost::asio::awaitable<void> Storage::copyAsync(const std::string& source, const std::string& destination) {
        if (!ioPool_) {
            throw std::runtime_error("Storage not initialized for async operations. Call initAsync() first.");
        }

        co_await boost::asio::post(*ioPool_, boost::asio::use_awaitable);
        copy(source, destination);
        co_return;
    }

    boost::asio::awaitable<void> Storage::moveAsync(const std::string& source, const std::string& destination) {
        if (!ioPool_) {
            throw std::runtime_error("Storage not initialized for async operations. Call initAsync() first.");
        }

        co_await boost::asio::post(*ioPool_, boost::asio::use_awaitable);
        move(source, destination);
        co_return;
    }
}