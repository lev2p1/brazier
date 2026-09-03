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

#include "../../include/brazier/Database/Cache.hpp"

using namespace brazier;

redisContext* Cache::context_ = nullptr;

void Cache::connect(const std::string& host, int port) {
    if (context_) {
        throw std::runtime_error("Already connected to Redis.");
    }

    context_ = redisConnect(host.c_str(), port);
    if (context_ == nullptr || context_->err) {
        if (context_) {
            std::string err = "Connection error: " + std::string(context_->errstr);
            redisFree(context_);
            context_ = nullptr;
            throw std::runtime_error(err);
        }
        else {
            Logger::log("Failed to allocate Redis context.", "ERROR");
            throw std::runtime_error("Failed to allocate Redis context.");
        }
    }

    Logger::log("Successfully connected to Redis " + host + ':' + std::to_string(port), "SUCCESS");
}

void Cache::disconnect() {
    if (context_) {
        redisFree(context_);
        context_ = nullptr;
        Logger::log("Disconnected from Redis.", "INFO");
    }
}

void Cache::set(const std::string& key, const std::string& value, int expire_seconds) {
    if (!context_) {
        Logger::log("Not connected to Redis.", "ERROR");
        throw std::runtime_error("Not connected to Redis.");
    }

    redisReply* reply;
    if (expire_seconds > 0) {
        reply = (redisReply*)redisCommand(context_, "SET %s %s EX %d", key.c_str(), value.c_str(), expire_seconds);
    }
    else {
        reply = (redisReply*)redisCommand(context_, "SET %s %s", key.c_str(), value.c_str());
    }

    if (reply == nullptr) {
		Logger::log("Failed to execute SET command.", "ERROR");
        throw std::runtime_error("Failed to execute SET command.");
    }

    freeReplyObject(reply);
    std::cout << "Set key '" << key << "' with value '" << value << "'." << std::endl;
}

std::string Cache::get(const std::string& key) {
    if (!context_) {
        Logger::log("Not connected to Redis.", "ERROR");
        throw std::runtime_error("Not connected to Redis.");
    }

    redisReply* reply = (redisReply*)redisCommand(context_, "GET %s", key.c_str());
    if (reply == nullptr) {
        Logger::log("Failed to execute GET command.", "ERROR");
        throw std::runtime_error("Failed to execute GET command.");
    }

    std::string result;
    if (reply->type == REDIS_REPLY_STRING) {
        result = std::string(reply->str, reply->len);
    }
    else if (reply->type == REDIS_REPLY_NIL) {
        result = "";
    }
    else {
        freeReplyObject(reply);
        Logger::log("Unexpected reply type from Redis.", "ERROR");
        throw std::runtime_error("Unexpected reply type from Redis.");
    }

    freeReplyObject(reply);
    std::cout << "Got value '" << result << "' for key '" << key << "'." << std::endl;
    return result;
}

void Cache::del(const std::string& key) {
    if (!context_) {
        Logger::log("Not connected to Redis.", "ERROR");
        throw std::runtime_error("Not connected to Redis.");
    }

    redisReply* reply = (redisReply*)redisCommand(context_, "DEL %s", key.c_str());
    if (reply == nullptr) {
        Logger::log("Failed to execute DEL command.", "ERROR");
        throw std::runtime_error("Failed to execute DEL command.");
    }

    freeReplyObject(reply);
    std::cout << "Deleted key '" << key << "'." << std::endl;
}

void Cache::expire(const std::string& key, int expire_seconds) {
    if (!context_) {
        Logger::log("Not connected to Redis.", "ERROR");
        throw std::runtime_error("Not connected to Redis.");
    }

    redisReply* reply = (redisReply*)redisCommand(context_, "EXPIRE %s %d", key.c_str(), expire_seconds);
    if (reply == nullptr) {
        Logger::log("Failed to execute EXPIRE command.", "ERROR");
        throw std::runtime_error("Failed to execute EXPIRE command.");
    }

    freeReplyObject(reply);
    std::cout << "Set expiration for key '" << key << "' to " << expire_seconds << " seconds." << std::endl;
}

boost::asio::awaitable<std::string> Cache::get_async(const std::string& key) {
    co_return Cache::get(key);
}

boost::asio::awaitable<void> Cache::set_async(const std::string& key, const std::string& value, int ttl) {
    Cache::set(key, value, ttl);
    co_return;
}