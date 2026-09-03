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

#include "../../include/brazier/Filesystem/StorageManager.hpp"

using namespace brazier;

StorageManager& StorageManager::getInstance() {
	static StorageManager instance;
	return instance;
}

void StorageManager::registerDriver(const std::string& name, std::shared_ptr<BaseDriver> driver) {
	drivers_[name] = driver;
}

void StorageManager::unregisterDriver(const std::string& name) {
	drivers_.erase(name);
}

void StorageManager::setDefaultDriver(const std::string& name) {
	if (drivers_.find(name) == drivers_.end()) {
		throw std::runtime_error("Driver not found: " + name);
	}
	defaultDriverName_ = name;
}

std::shared_ptr<BaseDriver> StorageManager::getDefaultDriver() const {
	return getDriver(defaultDriverName_);
}

std::shared_ptr<BaseDriver> StorageManager::getDriver(const std::string& name) const {
	if (drivers_.find(name) == drivers_.end()) {
		throw std::runtime_error("Driver not found: " + name);
	}

	return drivers_.at(name);
}

bool StorageManager::hasDriver(const std::string& name) const {
	return drivers_.find(name) != drivers_.end();
}

std::vector<std::string> StorageManager::getDriverNames() const {
	std::vector<std::string> names;
	for (const auto& pair : drivers_) {
		names.push_back(pair.first);
	}
	return names;
}