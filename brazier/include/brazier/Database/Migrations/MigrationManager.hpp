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

#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include <unordered_set>
#include <stdexcept>
#include <typeinfo>
#include "../Database.hpp"
#include "../SQLSchemaBuilder.hpp"
#include "../../vendor/Debug/Logger.hpp"

namespace brazier {

    class MigrationManager {
    private:
        Database& db;
        std::unordered_set<std::string> executedMigrations;

        bool isMigrationExecuted(const std::string& name);
        void markMigrationAsExecuted(const std::string& name);
        void unmarkMigrationAsExecuted(const std::string& name);
        void executeQueries(const std::vector<std::string>& queries);

    public:
        MigrationManager(Database& db);

        template <typename Migration>
        void migrate() {
            std::string name = typeid(Migration).name();

            if (isMigrationExecuted(name)) {
                Logger::log("Migration already executed: " + name, "INFO");
                return;
            }

            try {
                db.transaction("");
                db.transaction("migration_" + name);

                auto queries = Migration::up();
                for (const auto& query : queries) {
                    db.execute(query);
                }

                markMigrationAsExecuted(name);
                db.commit();
                Logger::log("Migration completed: " + name, "INFO");
            }
            catch (const std::exception& e) {
                if (db.isInTransaction()) {
                    try {
                        db.rollback("migration_" + name);
                    }
                    catch (...) {}

                    try {
                        db.rollback("");
                    }
                    catch (...) {}
                }

                Logger::log("Migration failed: " + name + " - " + e.what(), "ERROR");
                throw;
            }
        }

        template <typename Migration>
        void rollback() {
            std::string name = typeid(Migration).name();

            if (!isMigrationExecuted(name)) {
                Logger::log("Migration not executed: " + name, "ERROR");
                return;
            }

            try {
                db.transaction("");

                std::string mainQuery = Migration::down();
                db.execute(mainQuery);

                unmarkMigrationAsExecuted(name);

                db.commit();
                Logger::log("Migration rolled back: " + name, "INFO");
            }
            catch (const std::exception& e) {
                try {
                    db.rollback("");
                }
                catch (const std::exception& se) {
                    Logger::log("Could not rollback: " + std::string(se.what()), "WARNING");
                }
                Logger::log("Rollback failed: " + name + " - " + e.what(), "ERROR");
                throw;
            }
        }

        template <typename... Migrations>
        void migrateAll() {
            try {
                (migrate<Migrations>(), ...);
            }
            catch (const std::exception& e) {
                Logger::log("Migration chain stopped due to error: " + std::string(e.what()), "ERROR");
            }
        }

        template <typename... Migrations>
        void rollbackAll() {
            (rollback<Migrations>(), ...);
        }

        void Initialize();
    };
}