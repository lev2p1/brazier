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

#include "../../../include/brazier/Database/Migrations/migration_migrations_table.hpp"

using namespace brazier;

std::vector<std::string> MigrationMigrationsCreate::up() {
    SQLSchemaBuilder builder("migrations");
    std::vector<std::string> queries;

    queries.push_back(builder
        .AddColumn("id SERIAL PRIMARY KEY")
        .AddColumn("name VARCHAR(255) NOT NULL")
        .CreateTable());

    queries.push_back(builder.AddIndex("idx_name", { "name" }));

    return queries;
}

std::string MigrationMigrationsCreate::down() {
    SQLSchemaBuilder builder("migrations");
    return builder.DropTable();
}