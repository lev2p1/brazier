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

#include "BaseDriver.hpp"
#include <filesystem>
#include <boost/asio/thread_pool.hpp>

namespace brazier {

    namespace fs = std::filesystem;

    class S3Driver : public BaseDriver {
    private:
        std::string rootPath_;
        std::shared_ptr<boost::asio::thread_pool> ioPool_;
        bool asyncInitialized_ = false;

    public:
        S3Driver() = default;
        ~S3Driver() override = default;

        void initAsync(size_t threadCount = std::thread::hardware_concurrency()) override;
        void setRootPath(const std::string& path) override;
        std::string getRootPath() const override;

        void put(const std::string& path, const std::string& content) override;
        std::string get(const std::string& path) override;
        bool exists(const std::string& path) const override;
        void deleteFile(const std::string& path) override;
        void copy(const std::string& source, const std::string& destination) override;
        void move(const std::string& source, const std::string& destination) override;

        boost::asio::awaitable<void> putAsync(const std::string& path, const std::string& content) override;
        boost::asio::awaitable<std::string> getAsync(const std::string& path) override;
        boost::asio::awaitable<bool> existsAsync(const std::string& path) const override;
        boost::asio::awaitable<void> deleteFileAsync(const std::string& path) override;
        boost::asio::awaitable<void> copyAsync(const std::string& source, const std::string& destination) override;
        boost::asio::awaitable<void> moveAsync(const std::string& source, const std::string& destination) override;

        std::string getDriverType() const override { return S3; }
        bool isAsyncSupported() const override { return true; }
    };
}