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

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <memory>
#include "Database.hpp"

namespace brazier {

    class SQLSchemaBuilder {
    private:
        std::string table;
        std::vector<std::string> columns;
        std::vector<std::string> primaryKeys;
        std::vector<std::string> foreignKeys;
        std::vector<std::string> indexes;
        std::vector<std::string> uniqueConstraints;
        std::vector<std::string> dropColumns;
        std::vector<std::string> alterColumns;

    public:
        explicit SQLSchemaBuilder(const std::string& table);
        SQLSchemaBuilder& AddColumn(const std::string& columnDefinition);
        SQLSchemaBuilder& AddPrimaryKey(const std::string& column);
        SQLSchemaBuilder& AddForeignKey(const std::string& column, const std::string& referenceTable, const std::string& referenceColumn);
        std::string AddIndex(const std::string& indexName, const std::vector<std::string>& columns);
        std::string AddUniqueConstraint(const std::string& constraintName, const std::vector<std::string>& columns);
        SQLSchemaBuilder& DropColumn(const std::string& column);
        SQLSchemaBuilder& AlterColumn(const std::string& columnDefinition);
        std::string CreateTable();
        std::string DropTable();
    };
}