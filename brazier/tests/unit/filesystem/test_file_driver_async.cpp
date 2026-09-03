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

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "../../../include/brazier/Filesystem/FileDriver.hpp"
#include "test_utils.hpp"

namespace fs = std::filesystem;

class FileDriverTest : public TestBase {
protected:
    void SetUp() override {
        root_ = createTempDir();
        driver_ = std::make_shared<brazier::FileDriver>();
        driver_->setRootPath(root_.string());
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(root_, ec);
        driver_.reset();
    }

    fs::path root_;
    std::shared_ptr<brazier::FileDriver> driver_;
};

class FileDriverAsyncTest : public FileDriverTest {
protected:
    void SetUp() override {
        FileDriverTest::SetUp();
        driver_->initAsync(2);
    }
};

TEST_F(FileDriverAsyncTest, PutAsyncAndGetAsync) {
    const std::string path = "async.txt";
    const std::string content = "async data";
    runAsyncVoid(driver_->putAsync(path, content));
    auto result = runAsync(driver_->getAsync(path));
    EXPECT_EQ(result, content);
    auto exists = runAsync(driver_->existsAsync(path));
    EXPECT_TRUE(exists);
}

TEST_F(FileDriverAsyncTest, DeleteAsync) {
    const std::string path = "async_del.txt";
    driver_->put(path, "data");
    EXPECT_TRUE(driver_->exists(path));
    runAsyncVoid(driver_->deleteFileAsync(path));
    EXPECT_FALSE(driver_->exists(path));
}

TEST_F(FileDriverAsyncTest, CopyAsync) {
    const std::string src = "async_src.txt";
    const std::string dst = "async_dst.txt";
    const std::string content = "copy async";
    driver_->put(src, content);
    runAsyncVoid(driver_->copyAsync(src, dst));
    EXPECT_TRUE(driver_->exists(dst));
    EXPECT_EQ(driver_->get(dst), content);
}

TEST_F(FileDriverAsyncTest, MoveAsync) {
    const std::string src = "async_src.txt";
    const std::string dst = "async_dst.txt";
    const std::string content = "move async";
    driver_->put(src, content);
    runAsyncVoid(driver_->moveAsync(src, dst));
    EXPECT_FALSE(driver_->exists(src));
    EXPECT_TRUE(driver_->exists(dst));
    EXPECT_EQ(driver_->get(dst), content);
}

TEST_F(FileDriverAsyncTest, AsyncWithoutInitThrows) {
    auto freshDriver = std::make_shared<brazier::FileDriver>();
    freshDriver->setRootPath(root_.string());
    EXPECT_THROW(runAsyncVoid(freshDriver->putAsync("any", "data")), std::runtime_error);
    EXPECT_THROW(runAsync(freshDriver->getAsync("any")), std::runtime_error);
}

TEST_F(FileDriverAsyncTest, AsyncDeleteNonExistentThrows) {
    EXPECT_THROW(runAsyncVoid(driver_->deleteFileAsync("missing.txt")), std::runtime_error);
}

TEST_F(FileDriverAsyncTest, AsyncCopyNonExistentThrows) {
    EXPECT_THROW(runAsyncVoid(driver_->copyAsync("missing_src.txt", "dst.txt")), std::runtime_error);
}

TEST_F(FileDriverAsyncTest, AsyncMoveNonExistentThrows) {
    EXPECT_THROW(runAsyncVoid(driver_->moveAsync("missing_src.txt", "dst.txt")), std::runtime_error);
}

TEST_F(FileDriverAsyncTest, InitAsyncMultipleTimes) {
    EXPECT_NO_THROW(driver_->initAsync(2));
    EXPECT_NO_THROW(driver_->initAsync(4));
    const std::string path = "after_reinit.txt";
    const std::string content = "content";
    runAsyncVoid(driver_->putAsync(path, content));
    auto result = runAsync(driver_->getAsync(path));
    EXPECT_EQ(result, content);
}