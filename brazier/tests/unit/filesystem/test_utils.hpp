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

#include <gtest/gtest.h>
#include <filesystem>
#include <random>
#include <thread>
#include <optional>
#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/this_coro.hpp>

namespace fs = std::filesystem;

class TestBase : public ::testing::Test {
protected:
    fs::path createTempDir() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dist(100000, 999999);
        auto tempRoot = fs::temp_directory_path() / ("brazier_test_" + std::to_string(dist(gen)));
        fs::create_directories(tempRoot);
        return tempRoot;
    }

    template <typename Awaitable>
    auto runAsync(Awaitable&& awaitable) -> typename std::decay_t<Awaitable>::value_type {
        boost::asio::io_context ctx;
        auto work = boost::asio::make_work_guard(ctx);
        std::thread t([&ctx]() { ctx.run(); });

        using ResultType = typename std::decay_t<Awaitable>::value_type;
        std::optional<ResultType> result;
        std::exception_ptr ex;

        boost::asio::co_spawn(ctx, std::forward<Awaitable>(awaitable),
            [&](std::exception_ptr e, ResultType res) {
                if (e) ex = e;
                else result = std::move(res);
                work.reset();
            });

        t.join();
        if (ex) std::rethrow_exception(ex);
        return std::move(*result);
    }

    void runAsyncVoid(auto&& awaitable) {
        boost::asio::io_context ctx;
        auto work = boost::asio::make_work_guard(ctx);
        std::thread t([&ctx]() { ctx.run(); });

        std::exception_ptr ex;
        boost::asio::co_spawn(ctx, std::forward<decltype(awaitable)>(awaitable),
            [&](std::exception_ptr e) {
                ex = e;
                work.reset();
            });

        t.join();
        if (ex) std::rethrow_exception(ex);
    }
};