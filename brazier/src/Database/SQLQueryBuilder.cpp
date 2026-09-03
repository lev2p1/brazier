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

#include "../../include/brazier/Database/SQLQueryBuilder.hpp"

using namespace brazier;

SQLQueryBuilder::SQLQueryBuilder(const std::string& table) : table(table) {}

SQLQueryBuilder& SQLQueryBuilder::Select(const std::vector<std::string>& columns) {
    selectColumns = columns;
    return *this;
}

SQLQueryBuilder& SQLQueryBuilder::Delete() {
    isDeleteQuery = true;
    return *this;
}

SQLQueryBuilder& SQLQueryBuilder::Where(const std::string& condition, const std::string& logicalOperator) {
    if (!whereConditions.empty() && !isFirstConditionInGroup) {
        whereConditions.push_back(logicalOperator + " " + condition);
    }
    else {
        whereConditions.push_back(condition);
        isFirstConditionInGroup = false;
    }
    return *this;
}

SQLQueryBuilder& SQLQueryBuilder::BeginGroup(const std::string& logicalOperator) {
    if (!whereConditions.empty()) {
        whereConditions.push_back(logicalOperator + " (");
    }
    else {
        whereConditions.push_back("(");
    }
    isFirstConditionInGroup = true;
    return *this;
}

SQLQueryBuilder& SQLQueryBuilder::EndGroup() {
    whereConditions.push_back(")");
    isFirstConditionInGroup = false;
    return *this;
}

SQLQueryBuilder& brazier::SQLQueryBuilder::OrderBy(std::map<std::string, std::string> col_dir)
{
    orderByColumns = col_dir;
    return *this;
}

SQLQueryBuilder& SQLQueryBuilder::GroupBy(const std::vector<std::string>& columns) {
    groupByColumns = columns;
    return *this;
}

SQLQueryBuilder& SQLQueryBuilder::Limit(int value) {
    limit = " LIMIT " + std::to_string(value);
    return *this;
}

SQLQueryBuilder& SQLQueryBuilder::Offset(int value) {
    offset = " OFFSET " + std::to_string(value);
    return *this;
}

SQLQueryBuilder& SQLQueryBuilder::Join(const std::string& table, const std::string& onCondition, const std::string& joinType) {
    joins[table] = joinType + " JOIN " + table + " ON " + onCondition;
    return *this;
}

SQLQueryBuilder& SQLQueryBuilder::Insert(const std::map<std::string, std::string>& values) {
    insertValues = values;
    return *this;
}

SQLQueryBuilder& SQLQueryBuilder::Update(const std::map<std::string, std::string>& values) {
    updateValues = values;
    return *this;
}

std::string SQLQueryBuilder::get() {
    std::ostringstream query;

    if (!insertValues.empty()) {
        query << "INSERT INTO " << table << " (";
        for (auto it = insertValues.begin(); it != insertValues.end(); ++it) {
            query << it->first;
            if (std::next(it) != insertValues.end()) {
                query << ", ";
            }
        }
        query << ") VALUES (";
        for (auto it = insertValues.begin(); it != insertValues.end(); ++it) {
            query << it->second;
            if (std::next(it) != insertValues.end()) {
                query << ", ";
            }
        }
        query << ")";
    }
    else if (!updateValues.empty()) {
        query << "UPDATE " << table << " SET ";
        for (auto it = updateValues.begin(); it != updateValues.end(); ++it) {
            query << it->first << " = " << it->second;
            if (std::next(it) != updateValues.end()) {
                query << ", ";
            }
        }

        if (!whereConditions.empty()) {
            query << " WHERE ";
            for (size_t i = 0; i < whereConditions.size(); ++i) {
                query << whereConditions[i];
                if (i < whereConditions.size() - 1) {
                    query << " ";
                }
            }
        }
    }
    else if (isDeleteQuery) {
        query << "DELETE FROM " << table;

        if (!whereConditions.empty()) {
            query << " WHERE ";
            for (size_t i = 0; i < whereConditions.size(); ++i) {
                query << whereConditions[i];
                if (i < whereConditions.size() - 1) {
                    query << " ";
                }
            }
        }

        if (!orderByColumns.empty()) {
            query << " ORDER BY ";

            int i = 0;
            size_t size = orderByColumns.size();
            for (const auto& [k, v] : orderByColumns) {
                query << k << " " << v;
                if (++i < size) {
                    query << ", ";
                }
            }
        }

        if (!limit.empty()) {
            query << limit;
        }
    }
    else {
        query << "SELECT ";
        if (selectColumns.empty()) {
            query << "*";
        }
        else {
            for (size_t i = 0; i < selectColumns.size(); ++i) {
                query << selectColumns[i];
                if (i < selectColumns.size() - 1) {
                    query << ", ";
                }
            }
        }
        query << " FROM " << table;

        for (const auto& [table, joinClause] : joins) {
            query << " " << joinClause;
        }

        if (!whereConditions.empty()) {
            query << " WHERE ";
            for (size_t i = 0; i < whereConditions.size(); ++i) {
                query << whereConditions[i];
                if (i < whereConditions.size() - 1) {
                    query << " ";
                }
            }
        }

        if (!groupByColumns.empty()) {
            query << " GROUP BY ";
            for (size_t i = 0; i < groupByColumns.size(); ++i) {
                query << groupByColumns[i];
                if (i < groupByColumns.size() - 1) {
                    query << ", ";
                }
            }
        }

        if (!orderByColumns.empty()) {
            query << " ORDER BY ";

            int i = 0;
            size_t size = orderByColumns.size();
            for (const auto& [k, v] : orderByColumns) {
                query << k << " " << v;
                if (++i < size) {
                    query << ", ";
                }
            }
        }

        if (!limit.empty()) {
            query << limit;
        }

        if (!offset.empty()) {
            query << offset;
        }
    }

    return query.str();
}