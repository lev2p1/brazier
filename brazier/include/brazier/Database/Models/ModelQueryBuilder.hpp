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
#include "../SQLQueryBuilder.hpp"
#include "../Database.hpp"
#include <vector>
#include <memory>

namespace brazier {

    template<typename Derived>
    class ModelQueryBuilder : public SQLQueryBuilder {
    public:
        using SQLQueryBuilder::SQLQueryBuilder;

        ModelQueryBuilder& Select(const std::vector<std::string>& columns) {
            SQLQueryBuilder::Select(columns);
            return *this;
        }
        ModelQueryBuilder& Select(const std::string column) {
            SQLQueryBuilder::Select({ column });
            return *this;
        }
        ModelQueryBuilder& Select(const char* column) {
            SQLQueryBuilder::Select({ std::string(column) });
            return *this;
        }
        ModelQueryBuilder& Where(const std::string& condition, const std::string& logicalOperator = "AND") {
            SQLQueryBuilder::Where(condition, logicalOperator);
            return *this;
        }
        ModelQueryBuilder& OrderBy(const std::map<std::string, std::string> col_dir) {
            SQLQueryBuilder::OrderBy(col_dir);
            return *this;
        }
        ModelQueryBuilder& Limit(int value) {
            SQLQueryBuilder::Limit(value);
            return *this;
        }
        ModelQueryBuilder& Offset(int value) {
            SQLQueryBuilder::Offset(value);
            return *this;
        }

        std::vector<std::shared_ptr<Derived>> get() {
            auto database = std::make_shared<Database>();
            auto rows = database->queryToVector(this->getQuery());
            std::vector<std::shared_ptr<Derived>> result;
            for (const auto& row : rows) {
                auto model = Derived::create(row, true);
                if (model) result.push_back(model);
            }
            return result;
        }
    private:
        std::string getQuery() {
            return SQLQueryBuilder::get();
        }
    };
}