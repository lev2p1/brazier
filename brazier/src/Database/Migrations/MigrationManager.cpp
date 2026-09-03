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

#include "../../../include/brazier/Database/Migrations/MigrationManager.hpp"

using namespace brazier;

MigrationManager::MigrationManager(Database& db) : db(db) {
    try {
        auto result = db.queryToVector("SELECT name FROM migrations;");

        for (const auto& row : result) {
            auto it = row.find("name");
            if (it != row.end()) {
                executedMigrations.insert(it->second);
            }
            else {
                Logger::log("Column 'name' not found in row", "Warning");
            }
        }
    }
    catch (const std::exception& e) {
        Logger::log("Error loading migrations", "ERROR");
        throw e;
    }
}

bool MigrationManager::isMigrationExecuted(const std::string& name) {
    return executedMigrations.find(name) != executedMigrations.end();
}

void MigrationManager::markMigrationAsExecuted(const std::string& name) {
    db.execute("INSERT INTO migrations (name) VALUES ('" + name + "');");
    executedMigrations.insert(name);
}

void MigrationManager::unmarkMigrationAsExecuted(const std::string& name) {
    db.execute("DELETE FROM migrations WHERE name = '" + name + "';");
    executedMigrations.erase(name);
}

void MigrationManager::executeQueries(const std::vector<std::string>& queries) {
    for (const auto& query : queries) {
        db.execute(query);
    }
}

void MigrationManager::Initialize() {
    this->migrateAll<

    >();
}