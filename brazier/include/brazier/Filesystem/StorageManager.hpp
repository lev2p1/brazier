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
#include <unordered_map>
#include <memory>
#include <mutex>
#include "BaseDriver.hpp"

namespace brazier {

    class StorageManager {
    private:
        StorageManager() = default;
        ~StorageManager() = default;

        std::unordered_map<std::string, std::shared_ptr<BaseDriver>> drivers_;
        std::string defaultDriverName_;
        mutable std::mutex mutex_;

    public:
        static StorageManager& getInstance();

        void registerDriver(const std::string& name, std::shared_ptr<BaseDriver> driver);
        void unregisterDriver(const std::string& name);
        void setDefaultDriver(const std::string& name);

        std::shared_ptr<BaseDriver> getDefaultDriver() const;
        std::shared_ptr<BaseDriver> getDriver(const std::string& name) const;

        bool hasDriver(const std::string& name) const;

        std::vector<std::string> getDriverNames() const;
    };
}