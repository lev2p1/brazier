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

TEST_F(FileDriverTest, SetRootPathCreatesDirectory) {
    auto newRoot = createTempDir();
    driver_->setRootPath(newRoot.string());
    EXPECT_TRUE(fs::exists(newRoot));
    EXPECT_EQ(driver_->getRootPath(), newRoot.string());
    fs::remove_all(newRoot);
}

TEST_F(FileDriverTest, PutAndGet) {
    const std::string path = "test.txt";
    const std::string content = "Hello, World!";
    driver_->put(path, content);
    EXPECT_TRUE(fs::exists(root_ / path));
    EXPECT_EQ(driver_->get(path), content);
}

TEST_F(FileDriverTest, PutCreatesDirectories) {
    const std::string path = "a/b/c/file.txt";
    const std::string content = "nested";
    driver_->put(path, content);
    EXPECT_TRUE(fs::exists(root_ / path));
    EXPECT_EQ(driver_->get(path), content);
}

TEST_F(FileDriverTest, Exists) {
    const std::string path = "exists.txt";
    EXPECT_FALSE(driver_->exists(path));
    driver_->put(path, "data");
    EXPECT_TRUE(driver_->exists(path));
}

TEST_F(FileDriverTest, DeleteFile) {
    const std::string path = "todelete.txt";
    driver_->put(path, "data");
    EXPECT_TRUE(driver_->exists(path));
    driver_->deleteFile(path);
    EXPECT_FALSE(driver_->exists(path));
}

TEST_F(FileDriverTest, DeleteNonExistentThrows) {
    EXPECT_THROW(driver_->deleteFile("missing.txt"), std::runtime_error);
}

TEST_F(FileDriverTest, GetNonExistentThrows) {
    EXPECT_THROW(driver_->get("missing.txt"), std::runtime_error);
}

TEST_F(FileDriverTest, Copy) {
    const std::string src = "src.txt";
    const std::string dst = "dst.txt";
    const std::string content = "copy me";
    driver_->put(src, content);
    driver_->copy(src, dst);
    EXPECT_TRUE(driver_->exists(src));
    EXPECT_TRUE(driver_->exists(dst));
    EXPECT_EQ(driver_->get(dst), content);
}

TEST_F(FileDriverTest, CopyNonExistentThrows) {
    EXPECT_THROW(driver_->copy("missing.txt", "dest.txt"), std::runtime_error);
}

TEST_F(FileDriverTest, Move) {
    const std::string src = "src.txt";
    const std::string dst = "dst.txt";
    const std::string content = "move me";
    driver_->put(src, content);
    driver_->move(src, dst);
    EXPECT_FALSE(driver_->exists(src));
    EXPECT_TRUE(driver_->exists(dst));
    EXPECT_EQ(driver_->get(dst), content);
}

TEST_F(FileDriverTest, MoveNonExistentThrows) {
    EXPECT_THROW(driver_->move("missing.txt", "dest.txt"), std::runtime_error);
}

TEST_F(FileDriverTest, PutOverwritesExistingFile) {
    const std::string path = "overwrite.txt";
    driver_->put(path, "old content");
    EXPECT_EQ(driver_->get(path), "old content");
    driver_->put(path, "new content");
    EXPECT_EQ(driver_->get(path), "new content");
}

TEST_F(FileDriverTest, GetDriverType) {
    EXPECT_FALSE(driver_->getDriverType().empty());
}