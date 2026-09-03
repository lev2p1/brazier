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
#pragma warning(disable : 4996)

#include <cstdio>
#include <string>
#include <chrono>
#include <mutex>
#include <filesystem>
#include <csignal>
#include <cstdlib>
#include <array>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace brazier {

    class Logger {
    private:
        static FILE* logFile;
        static std::mutex logMutex;
        static std::filesystem::path logFilePath;
        static size_t maxLines;
        static size_t currentLineCount;
        static inline bool colorsEnabled = true;

        static thread_local std::array<char, 64> timeCache;
        static thread_local std::chrono::system_clock::time_point lastTimeUpdate;

        static void signalHandler(int signal);
        static void rotateLogs();
        static const char* getCurrentTimeFast();
        static const char* getColorCodeFast(const char* level);
        static const char* resetColorFast();

    public:
        Logger() = delete;
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        static void init(const std::string& filename);
        static void log(const std::string& message, const char* level = "INFO");
        static void log(const char* message, const char* level = "INFO");
        static void shutdown();
        static void registerSignalHandlers();
    };
}