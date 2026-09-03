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

#include "../include/brazier/vendor/Debug/Logger.hpp"

using namespace brazier;

FILE* Logger::logFile = nullptr;
std::mutex Logger::logMutex;
std::filesystem::path Logger::logFilePath;
size_t Logger::maxLines = 10000;
size_t Logger::currentLineCount = 0;

thread_local std::array<char, 64> Logger::timeCache;
thread_local std::chrono::system_clock::time_point Logger::lastTimeUpdate;

void Logger::signalHandler(int signal) {
    std::fprintf(stderr, "\033[1;31m [ERROR] Program terminated with signal: %d\033[0m\n", signal);
    std::exit(signal);
}

void Logger::rotateLogs() {
    if (logFile) {
        std::fclose(logFile);
        logFile = nullptr;
    }

    std::filesystem::path oldLogPath = logFilePath;
    std::string result = oldLogPath.filename().string();

    auto ext_pos = result.rfind('.');
    if (ext_pos != std::string::npos) {
        result.replace(ext_pos, std::string::npos, "_old" + oldLogPath.extension().string());
    }
    else {
        result += "_old";
    }

    oldLogPath.replace_filename(result);

    if (std::filesystem::exists(oldLogPath)) {
        std::filesystem::remove(oldLogPath);
    }

    std::filesystem::rename(logFilePath, oldLogPath);

    logFile = std::fopen(logFilePath.string().c_str(), "a");
    currentLineCount = 0;
}

const char* Logger::getCurrentTimeFast() {
    auto now = std::chrono::system_clock::now();

    if (now - lastTimeUpdate < std::chrono::seconds(1)) {
        return timeCache.data();
    }

    lastTimeUpdate = now;
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t_now);

    std::strftime(timeCache.data(), timeCache.size(), "%Y-%m-%d %H:%M:%S", &tm);
    return timeCache.data();
}

const char* Logger::getColorCodeFast(const char* level) {
    if (!Logger::colorsEnabled) return "";

    switch (level[0]) {
    case 'E': return "\033[1;31m";
    case 'W': return "\033[1;33m";
    case 'I': return "\033[1;36m";
    case 'S': return "\033[1;32m";
    case 'H': return "\033[1;34m";
    case 'C': return "\033[1;35m";
    default:  return "\033[0m";
    }
}

const char* Logger::resetColorFast() {
    return Logger::colorsEnabled ? "\033[0m" : "";
}

void Logger::init(const std::string& filename) {
    std::lock_guard<std::mutex> lock(logMutex);

    std::filesystem::path logDir = std::filesystem::current_path() / "Storage" / "Logs";
    std::filesystem::create_directories(logDir);

    logFilePath = logDir / filename;
    logFile = std::fopen(logFilePath.string().c_str(), "a");

    if (!logFile) {
        throw std::runtime_error("Failed to open log file: " + logFilePath.string());
    }

    std::ifstream inFile(logFilePath);
    currentLineCount = std::count(std::istreambuf_iterator<char>(inFile),
        std::istreambuf_iterator<char>(), '\n');
}

void Logger::log(const std::string& message, const char* level) {
    std::lock_guard<std::mutex> lock(logMutex);

    const char* timestamp = getCurrentTimeFast();
    const char* colorCode = getColorCodeFast(level);
    const char* resetCode = resetColorFast();

    if (logFile) {
        std::fprintf(logFile, "%s [%s] %s\n", timestamp, level, message.c_str());
        std::fflush(logFile);

        if (++currentLineCount >= maxLines) {
            rotateLogs();
        }
    }

    std::fprintf(stdout, "%s%s [%s] %s%s\n", timestamp, colorCode, level, resetCode, message.c_str());
}

void Logger::log(const char* message, const char* level) {
    std::lock_guard<std::mutex> lock(logMutex);

    const char* timestamp = getCurrentTimeFast();
    const char* colorCode = getColorCodeFast(level);
    const char* resetCode = resetColorFast();

    if (logFile) {
        std::fprintf(logFile, "%s [%s] %s\n", timestamp, level, message);
        std::fflush(logFile);

        if (++currentLineCount >= maxLines) {
            rotateLogs();
        }
    }

    std::fprintf(stdout, "%s%s [%s] %s%s\n", timestamp, colorCode, level, resetCode, message);
}

void Logger::shutdown() {
    if (logFile) {
        std::fclose(logFile);
        logFile = nullptr;
    }
}

void Logger::registerSignalHandlers() {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    std::atexit(shutdown);
}