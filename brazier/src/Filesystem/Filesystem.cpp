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

#include "../../include/brazier/Filesystem/Filesystem.hpp"

brazier::Filesystem::Filesystem()
	: driver_(StorageManager::getInstance().getDefaultDriver())
	, driverName_("default")
{
	if (!driver_) {
		throw std::runtime_error("No default driver registered");
	}
}

brazier::Filesystem::Filesystem(const std::string& name)
    : driver_(StorageManager::getInstance().getDriver(name))
    , driverName_(name)
{
    if (!driver_) {
        throw std::runtime_error("Driver not found: " + name);
    }
}

brazier::Filesystem::Filesystem(std::shared_ptr<BaseDriver> driver)
    : driver_(driver)
    , driverName_("custom")
{
    if (!driver_) {
        throw std::runtime_error("Driver is null");
    }
}

brazier::Filesystem brazier::Filesystem::driver(const std::string& name) {
	return brazier::Filesystem(name);
}

bool brazier::Filesystem::hasDriver(const std::string& name) {
    return brazier::StorageManager::getInstance().hasDriver(name);
}

std::string brazier::Filesystem::getDriverName() const {
    return driverName_;
}

std::string brazier::Filesystem::getDriverType() const {
    return driver_->getDriverType();
}

void brazier::Filesystem::put(const std::string& path, const std::string& content) {
    driver_->put(path, content);
}

std::string brazier::Filesystem::get(const std::string& path) {
    return driver_->get(path);
}

bool brazier::Filesystem::exists(const std::string& path) const {
    return driver_->exists(path);
}

void brazier::Filesystem::deleteFile(const std::string& path) {
    driver_->deleteFile(path);
}

void brazier::Filesystem::copy(const std::string& source, const std::string& destination) {
    driver_->copy(source, destination);
}

void brazier::Filesystem::move(const std::string& source, const std::string& destination) {
    driver_->move(source, destination);
}

boost::asio::awaitable<void> brazier::Filesystem::putAsync(const std::string& path, const std::string& content) {
    co_await driver_->putAsync(path, content);
}

boost::asio::awaitable<std::string> brazier::Filesystem::getAsync(const std::string& path) {
    co_return co_await driver_->getAsync(path);
}

boost::asio::awaitable<bool> brazier::Filesystem::existsAsync(const std::string& path) const {
    co_return co_await driver_->existsAsync(path);
}

boost::asio::awaitable<void> brazier::Filesystem::deleteFileAsync(const std::string& path) {
    co_await driver_->deleteFileAsync(path);
}

boost::asio::awaitable<void> brazier::Filesystem::copyAsync(const std::string& source, const std::string& destination) {
    co_await driver_->copyAsync(source, destination);
}

boost::asio::awaitable<void> brazier::Filesystem::moveAsync(const std::string& source, const std::string& destination) {
    co_await driver_->moveAsync(source, destination);
}

std::shared_ptr<brazier::BaseDriver> brazier::Filesystem::getDriver() const {
	return driver_;
}