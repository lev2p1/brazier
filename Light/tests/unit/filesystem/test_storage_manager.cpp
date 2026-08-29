/*
 * Copyright (c) 2026 Kirill Sergeev, Nikolay Sugonyako, Andrey Agarkov, Gleb Safyannikov
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This file is part of lightlib.
 *
 * lightlib is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * lightlib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lightlib; if not, see <https://www.gnu.org/licenses/>.
 */

#include <gtest/gtest.h>
#include <algorithm>
#include "../../../include/lightlib/Filesystem/StorageManager.hpp"
#include "../../../include/lightlib/Filesystem/FileDriver.hpp"

class StorageManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto& mgr = lightlib::StorageManager::getInstance();
        for (const auto& name : mgr.getDriverNames()) {
            mgr.unregisterDriver(name);
        }
    }

    void TearDown() override {
        auto& mgr = lightlib::StorageManager::getInstance();
        for (const auto& name : mgr.getDriverNames()) {
            mgr.unregisterDriver(name);
        }
    }
};

TEST_F(StorageManagerTest, RegisterAndGetDriver) {
    auto& mgr = lightlib::StorageManager::getInstance();
    auto driver = std::make_shared<lightlib::FileDriver>();
    const std::string name = "test_driver";
    mgr.registerDriver(name, driver);
    auto retrieved = mgr.getDriver(name);
    EXPECT_EQ(retrieved, driver);
    EXPECT_TRUE(mgr.hasDriver(name));
}

TEST_F(StorageManagerTest, GetNonExistentThrows) {
    auto& mgr = lightlib::StorageManager::getInstance();
    EXPECT_THROW(mgr.getDriver("non_existent"), std::runtime_error);
}

TEST_F(StorageManagerTest, UnregisterDriver) {
    auto& mgr = lightlib::StorageManager::getInstance();
    auto driver = std::make_shared<lightlib::FileDriver>();
    const std::string name = "to_unregister";
    mgr.registerDriver(name, driver);
    EXPECT_TRUE(mgr.hasDriver(name));
    mgr.unregisterDriver(name);
    EXPECT_FALSE(mgr.hasDriver(name));
    EXPECT_THROW(mgr.getDriver(name), std::runtime_error);
}

TEST_F(StorageManagerTest, SetDefaultDriver) {
    auto& mgr = lightlib::StorageManager::getInstance();
    auto driver = std::make_shared<lightlib::FileDriver>();
    const std::string name = "default_driver";
    mgr.registerDriver(name, driver);
    mgr.setDefaultDriver(name);
    auto defaultDriver = mgr.getDefaultDriver();
    EXPECT_EQ(defaultDriver, driver);
}

TEST_F(StorageManagerTest, SetDefaultNonExistentThrows) {
    auto& mgr = lightlib::StorageManager::getInstance();
    EXPECT_THROW(mgr.setDefaultDriver("missing"), std::runtime_error);
}

TEST_F(StorageManagerTest, GetDefaultWhenNotSetThrows) {
    auto& mgr = lightlib::StorageManager::getInstance();
    for (const auto& name : mgr.getDriverNames()) {
        mgr.unregisterDriver(name);
    }
    EXPECT_THROW(mgr.getDefaultDriver(), std::runtime_error);
}

TEST_F(StorageManagerTest, GetDriverNames) {
    auto& mgr = lightlib::StorageManager::getInstance();
    auto d1 = std::make_shared<lightlib::FileDriver>();
    auto d2 = std::make_shared<lightlib::FileDriver>();
    mgr.registerDriver("one", d1);
    mgr.registerDriver("two", d2);
    auto names = mgr.getDriverNames();
    EXPECT_EQ(names.size(), 2);
    EXPECT_TRUE(std::find(names.begin(), names.end(), "one") != names.end());
    EXPECT_TRUE(std::find(names.begin(), names.end(), "two") != names.end());
}

TEST_F(StorageManagerTest, RegisterOverwritesExisting) {
    auto& mgr = lightlib::StorageManager::getInstance();
    auto d1 = std::make_shared<lightlib::FileDriver>();
    auto d2 = std::make_shared<lightlib::FileDriver>();
    mgr.registerDriver("same", d1);
    mgr.registerDriver("same", d2);
    auto retrieved = mgr.getDriver("same");
    EXPECT_EQ(retrieved, d2); 
}

TEST_F(StorageManagerTest, UnregisterNonExistentDoesNothing) {
    auto& mgr = lightlib::StorageManager::getInstance();
    EXPECT_FALSE(mgr.hasDriver("missing"));
    EXPECT_NO_THROW(mgr.unregisterDriver("missing"));
    EXPECT_FALSE(mgr.hasDriver("missing"));
}