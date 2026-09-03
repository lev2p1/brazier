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
#include <map>
#include <sstream>
#include <memory>

namespace brazier {

    class SQLQueryBuilder {
    private:
        std::string table;
        std::vector<std::string> selectColumns;
        std::vector<std::string> whereConditions;
        std::map<std::string, std::string> orderByColumns;
        std::vector<std::string> groupByColumns;
        std::string limit;
        std::string offset;
        std::map<std::string, std::string> joins;
        std::map<std::string, std::string> insertValues;
        std::map<std::string, std::string> updateValues;
        bool isDeleteQuery = false;
        bool isFirstConditionInGroup = true;

    public:
        explicit SQLQueryBuilder(const std::string& table);
        SQLQueryBuilder& Select(const std::vector<std::string>& columns);
        SQLQueryBuilder& Delete();
        SQLQueryBuilder& Where(const std::string& condition, const std::string& logicalOperator = "AND");
        SQLQueryBuilder& BeginGroup(const std::string& logicalOperator = "AND");
        SQLQueryBuilder& EndGroup();
        SQLQueryBuilder& OrderBy(std::map<std::string, std::string> col_dir);
        SQLQueryBuilder& GroupBy(const std::vector<std::string>& columns);
        SQLQueryBuilder& Limit(int value);
        SQLQueryBuilder& Offset(int value);
        SQLQueryBuilder& Join(const std::string& table, const std::string& onCondition, const std::string& joinType = "INNER");
        SQLQueryBuilder& Insert(const std::map<std::string, std::string>& values);
        SQLQueryBuilder& Update(const std::map<std::string, std::string>& values);
        std::string get();
    };
}