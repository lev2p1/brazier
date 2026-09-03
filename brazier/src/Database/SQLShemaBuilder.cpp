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

#include "../../include/brazier/Database/SQLSchemaBuilder.hpp"

using namespace brazier;

SQLSchemaBuilder::SQLSchemaBuilder(const std::string& table) : table(table) {}

SQLSchemaBuilder& SQLSchemaBuilder::AddColumn(const std::string& columnDefinition) {
    columns.push_back(columnDefinition);
    return *this;
}

SQLSchemaBuilder& SQLSchemaBuilder::AddPrimaryKey(const std::string& column) {
    primaryKeys.push_back(column);
    return *this;
}

SQLSchemaBuilder& SQLSchemaBuilder::AddForeignKey(const std::string& column, const std::string& referenceTable, const std::string& referenceColumn) {
    foreignKeys.push_back("(" + column + ")" + " REFERENCES " + referenceTable + "(" + referenceColumn + ")");
    return *this;
}

std::string SQLSchemaBuilder::AddIndex(const std::string& indexName, const std::vector<std::string>& columns) {
    std::ostringstream indexColumns;
    for (size_t i = 0; i < columns.size(); ++i) {
        indexColumns << columns[i];
        if (i < columns.size() - 1) {
            indexColumns << ", ";
        }
    }
    return "CREATE INDEX " + indexName + " ON " + table + " (" + indexColumns.str() + ");";
}

std::string SQLSchemaBuilder::AddUniqueConstraint(const std::string& constraintName, const std::vector<std::string>& columns) {
    std::ostringstream uniqueColumns;
    for (size_t i = 0; i < columns.size(); ++i) {
        uniqueColumns << columns[i];
        if (i < columns.size() - 1) {
            uniqueColumns << ", ";
        }
    }
    return "ALTER TABLE " + table + " ADD CONSTRAINT " + constraintName + " UNIQUE (" + uniqueColumns.str() + ");";
}

SQLSchemaBuilder& SQLSchemaBuilder::DropColumn(const std::string& column) {
    dropColumns.push_back(column);
    return *this;
}

SQLSchemaBuilder& SQLSchemaBuilder::AlterColumn(const std::string& columnDefinition) {
    alterColumns.push_back(columnDefinition);
    return *this;
}

std::string SQLSchemaBuilder::CreateTable() {
    std::ostringstream query;

    query << "CREATE TABLE " << table << " (";

    for (size_t i = 0; i < columns.size(); ++i) {
        query << columns[i];
        if (i < columns.size() - 1 || !primaryKeys.empty() || !foreignKeys.empty()) {
            query << ", ";
        }
    }

    if (!primaryKeys.empty()) {
        query << "PRIMARY KEY (";
        for (size_t i = 0; i < primaryKeys.size(); ++i) {
            query << primaryKeys[i];
            if (i < primaryKeys.size() - 1) {
                query << ", ";
            }
        }
        query << ")";
        if (!foreignKeys.empty()) {
            query << ", ";
        }
    }

    for (size_t i = 0; i < foreignKeys.size(); ++i) {
        query << "FOREIGN KEY " << foreignKeys[i];
        if (i < foreignKeys.size() - 1) {
            query << ", ";
        }
    }

    query << ");";

    return query.str();
}

std::string SQLSchemaBuilder::DropTable() {
    return "DROP TABLE " + table + " CASCADE;";
}