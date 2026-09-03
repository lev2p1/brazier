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
#include <map>
#include <memory>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <nlohmann/json.hpp>
#include "../Database.hpp"
#include "../../vendor/Debug/Logger.hpp"
#include "ModelQueryBuilder.hpp"
#include "../SQLQueryBuilder.hpp"
#include "../SQLString.hpp"
#include "../Collection.hpp"

namespace brazier {

    using json = nlohmann::json;

    template <typename Derived>
    class Model {
    protected:
        static inline std::vector<std::string> fillable;
        static inline std::vector<std::string> fields;

        std::map<std::string, std::string> attributes;
        static inline std::string primary_key = "id";

        std::shared_ptr<Database> database;

        static std::shared_ptr<Database> getDefaultDatabase() {
            static auto db = std::make_shared<Database>();
            return db;
        }

    public:
        Model(const std::shared_ptr<Database>& db = getDefaultDatabase()) : database(db) {}

        virtual ~Model() = default;

        void setDatabase(const std::shared_ptr<Database>& db) {
            this->database = db;
        }

        static ModelQueryBuilder<Derived> query() {
            return ModelQueryBuilder<Derived>(Derived::table_name);
        }

        void setAttribute(const std::string& key, const std::string& value) {
            if (isField(key)) {
                attributes[key] = value;
            }
            else {
                Logger::log("Field '" + key + "' is not fillable or not a valid field.", "ERROR");
            }
        }

        void debugPrintAttributes() const {
            std::cout << "=== MODEL ATTRIBUTES ===\n";
            for (const auto& [key, val] : attributes) {
                std::cout << key << " = '" << val << "'\n";
            }
        }

        virtual bool validate() { return true; }

        virtual bool beforeSave() { return true; }

        virtual void afterSave() {}

        std::string getAttribute(const std::string& key) const {
            auto it = attributes.find(key);
            if (it != attributes.end()) {
                return it->second;
            }
            throw std::invalid_argument("Attribute '" + key + "' not found.");
        }

        bool save() {
            if (!validate() || !beforeSave()) {
                return false;
            }

            if (!database) {
                database = getDefaultDatabase();
            }

            if (attributes.empty()) {
                Logger::log("Attributes are empty", "ERROR");
                return false;
            }

            PGconn* conn = database->getConnection();
            if (!conn) {
                Logger::log("Failed to get database connection", "ERROR");
                return false;
            }

            bool hasId = attributes.find(Derived::primary_key) != attributes.end() &&
                !attributes[Derived::primary_key].empty();

            std::map<std::string, std::string> insertValues;
            for (const auto& [key, value] : attributes) {
                if (key == Derived::primary_key) continue;
                insertValues[key] = SQLString::EscapeString(conn, value);
            }

            if (insertValues.empty()) {
                Logger::log("Insert values are empty. Aborting save operation", "ERROR");
                return false;
            }

            SQLQueryBuilder builder(Derived::table_name);

            if (hasId) {
                builder.Update(insertValues);
                builder.Where(Derived::primary_key + " = " + attributes[Derived::primary_key]);
                std::string query = builder.get();
                try {
                    database->execute(query);
                    afterSave();
                    return true;
                }
                catch (const std::exception& e) {
                    Logger::log(e.what(), "ERROR");
                    return false;
                }
            }
            else {
                builder.Insert(insertValues);
                std::string query = builder.get();
                query += " RETURNING " + Derived::primary_key;

                try {
                    auto row = database->queryMap(query, {});
                    if (!row.empty() && row.find(Derived::primary_key) != row.end()) {
                        attributes[Derived::primary_key] = row.at(Derived::primary_key);
                    }
                    afterSave();
                    return true;
                }
                catch (const std::exception& e) {
                    Logger::log(e.what(), "ERROR");
                    return false;
                }
            }
        }

        static bool isFillable(const std::string& field) {
            return std::find(Derived::fillable.begin(), Derived::fillable.end(), field) != Derived::fillable.end();
        }

        static bool isField(const std::string& field) {
            return std::find(Derived::fields.begin(), Derived::fields.end(), field) != Derived::fields.end();
        }

        static std::shared_ptr<Derived> create(const std::map<std::string, std::string>& data, bool withFields = false, const std::shared_ptr<Database>& db = getDefaultDatabase()) {
            auto model = std::make_shared<Derived>(db);
            for (const auto& [key, value] : data) {
                if (!isField(key)) {
                    Logger::log("Field '" + key + "' is not a valid field (create)", "WARNING");
                    continue;
                }

                if (withFields || isFillable(key)) {
                    model->setAttribute(key, value);
                }
                else {
                    Logger::log("Field '" + key + "' is not fillable (create)", "WARNING");
                }
            }
            return model;
        }

        static std::shared_ptr<Derived> find(int id, const std::shared_ptr<Database>& db = getDefaultDatabase()) {
            try {
                auto model = std::make_shared<Derived>();
                auto results = query().Where(Derived::primary_key + " = " + std::to_string(id)).Limit(1).get();
                if (results.empty()) {
                    Logger::log("No data found by id", "WARNING");
                    return nullptr;
                }
                if (results.size() > 1) {
                    Logger::log("Found multiple records for single id", "ERROR");
                    return nullptr;
                }

                results.front()->setDatabase(db);
                return results.front();
            }
            catch (const std::exception& e) {
                Logger::log("Error searching database (find()): " + std::string(e.what()), "ERROR");
                return nullptr;
            }
        }

        static void update(int id, const std::map<std::string, std::string>& data, const std::shared_ptr<Database>& db = getDefaultDatabase()) {
            try {
                PGconn* conn = db->getConnection();
                if (!conn) {
                    Logger::log("Failed to get database connection", "ERROR");
                    return;
                }
                SQLQueryBuilder builder(Derived::table_name);
                std::map<std::string, std::string> updateValues;
                for (const auto& [key, value] : data) {
                    if (isField(key)) {
                        updateValues[key] = SQLString::EscapeString(conn, value);
                    }
                }
                if (updateValues.empty()) {
                    Logger::log("No valid fields provided for update", "WARNING");
                    return;
                }
                builder.Update(updateValues).Where("id = " + SQLString::EscapeString(conn, std::to_string(id)));
                db->execute(builder.get());
            }
            catch (const std::exception& e) {
                Logger::log("Update failed: " + std::string(e.what()), "ERROR");
            }
        }

        void delete_() {
            try {
                if (!database) database = getDefaultDatabase();
                PGconn* conn = database->getConnection();
                if (!conn) {
                    Logger::log("Failed to get database connection", "ERROR");
                    return;
                }
                SQLQueryBuilder builder(Derived::table_name);
                auto id = getAttribute("id");
                if (id.empty()) {
                    Logger::log("ID attribute is missing", "ERROR");
                    return;
                }
                builder.Delete().Where("id = " + SQLString::EscapeString(conn, id));
                std::string query = builder.get();
                database->execute(query);
            }
            catch (const std::exception& e) {
                Logger::log("Delete failed: " + std::string(e.what()), "ERROR");
            }
        }

        static void deleteById(int id, const std::shared_ptr<Database>& db = getDefaultDatabase()) {
            try {
                PGconn* conn = db->getConnection();
                if (!conn) {
                    Logger::log("Failed to get database connection", "ERROR");
                    return;
                }
                SQLQueryBuilder builder(Derived::table_name);
                builder.Delete().Where("id = " + SQLString::EscapeString(conn, std::to_string(id)));
                std::string query = builder.get();
                db->execute(query);
            }
            catch (const std::exception& e) {
                Logger::log("Delete by ID failed: " + std::string(e.what()), "ERROR");
            }
        }

        static bool deleteWhere(const std::string& condition, const std::shared_ptr<Database>& db = getDefaultDatabase()) {
            try {
                PGconn* conn = db->getConnection();
                if (!conn) {
                    Logger::log("Failed to get database connection", "ERROR");
                    return false;
                }
                SQLQueryBuilder builder(Derived::table_name);
                builder.Delete().Where(condition);
                std::string query = builder.get();
                db->execute(query);

                return true;
            }
            catch (const std::exception& e) {
                Logger::log("Delete where failed: " + std::string(e.what()), "ERROR");
                return false;
            }
        }

        static Collection<Derived> where(const std::string& condition, const std::shared_ptr<Database>& db = getDefaultDatabase()) {
            auto items = query().Where(condition).get();
            for (const auto& item : items) {
                item->setDatabase(db);
            }
            return Collection<Derived>(items);
        }


        /*
			@brief Save multiple models to the database in a single operation.
			@param models A vector of shared pointers to the models to be saved.
			@return True if all models were saved successfully, false otherwise.
			@note This method will attempt to batch insert and update models based on their primary key. If a model has a primary key, it will be updated; otherwise, it will be inserted.
        */
        static bool saveMany(const std::vector<std::shared_ptr<Derived>>& models, const std::shared_ptr<Database>& db = getDefaultDatabase()) {
            PGconn* conn = db->getConnection();

            if (!conn) {
                Logger::log("Failed to get database connection", "ERROR");
                return false;
            }

            if (models.empty()) {
                Logger::log("No models to save", "WARNING");
                return false;
            }

            const auto& first = models.front();

            std::vector<std::shared_ptr<Derived>> toInsert;
            std::vector<std::shared_ptr<Derived>> toUpdate;

            for (const auto& model : models) {
                try {
                    std::string id = model->getAttribute(Derived::primary_key);
                    if (!id.empty()) {
                        toUpdate.push_back(model);
                    }
                    else {
                        toInsert.push_back(model);
                    }
                }
                catch (const std::exception& e) {
                    toInsert.push_back(model);
                }
            }

            if (!toUpdate.empty()) {
                std::ostringstream batchQuery;

                for (const auto& model : toUpdate) {
                    try {
                        std::string id = model->getAttribute(Derived::primary_key);

                        std::map<std::string, std::string> updateData;
                        for (const auto& key : model->fields) {
                            if (key == Derived::primary_key) continue;
                            try {
                                std::string value = model->getAttribute(key);
                                if (!value.empty()) {
                                    updateData[key] = value;
                                }
                            }
                            catch (const std::exception& e) {}
                        }

                        if (updateData.empty()) continue;

                        SQLQueryBuilder builder(Derived::table_name);
                        std::map<std::string, std::string> escapedData;
                        for (const auto& [key, value] : updateData) {
                            escapedData[key] = SQLString::EscapeString(conn, value);
                        }

                        builder.Update(escapedData).Where(Derived::primary_key + " = " + SQLString::EscapeString(conn, id));
                        batchQuery << builder.get() << "; ";
                    }
                    catch (const std::exception& e) {
                        Logger::log("Failed to update model: " + std::string(e.what()), "ERROR");
                        return false;
                    }
                }

                std::string finalQuery = batchQuery.str();
                if (!finalQuery.empty()) {
                    try {
                        db->execute(finalQuery);
                    }
                    catch (const std::exception& e) {
                        Logger::log("Batch update error: " + std::string(e.what()), "ERROR");
                        return false;
                    }
                }
            }

            if (!toInsert.empty()) {
                std::vector<std::string> columns;
                for (const auto& key : first->fields) {
                    if (key == Derived::primary_key) continue;
                    try {
                        if (!first->getAttribute(key).empty()) {
                            columns.push_back(key);
                        }
                    }
                    catch (const std::exception& e) {}
                }

                if (columns.empty()) {
                    Logger::log("No columns found for insert", "WARNING");
                    return false;
                }

                std::string valuesStr;
                for (size_t i = 0; i < toInsert.size(); ++i) {
                    const auto& model = toInsert[i];
                    std::string rowStr = "(";

                    for (size_t j = 0; j < columns.size(); ++j) {
                        const std::string& col = columns[j];
                        std::string value = model->getAttribute(col);
                        std::string escaped = SQLString::EscapeString(conn, value);
                        rowStr += escaped;

                        if (j < columns.size() - 1) rowStr += ", ";
                    }
                    rowStr += ")";

                    if (i > 0) valuesStr += ", ";
                    valuesStr += rowStr;
                }

                std::string query = "INSERT INTO " + Derived::table_name + " (";
                for (size_t i = 0; i < columns.size(); ++i) {
                    query += columns[i];
                    if (i < columns.size() - 1) query += ", ";
                }
                query += ") VALUES " + valuesStr + ";";

                try {
                    Logger::log("INSERT Query: " + query, "INFO");
                    db->execute(query);
                }
                catch (const std::exception& e) {
                    Logger::log("Batch insert error: " + std::string(e.what()), "ERROR");
                    return false;
                }
            }

            return true;
        }

        static Collection<Derived> all(const std::shared_ptr<Database>& db = getDefaultDatabase()) {
            return Derived::where("1 = 1", db);
        }

        static std::shared_ptr<Derived> first(const std::shared_ptr<Database>& db = getDefaultDatabase()) {
            auto results = query().Limit(1).get();
            if (results.empty()) return nullptr;
            results.front->setDatabase(db);
            return results.front();
        }

        static std::shared_ptr<Derived> findOrFail(int id, const std::shared_ptr<Database>& db = getDefaultDatabase()) {
            auto model = find(id, db);
            if (!model) {
                throw std::runtime_error("Model with id " + std::to_string(id) + " not found");
            }
            return model;
        }

        /*
			@brief Finds the first model matching the given attributes or creates a new one if none exists.
			@param attributes A map of attribute names and values to search for.
			@param values A map of attribute names and values to set on the new model if it is created (default is an empty map).
			@return A shared pointer to the found or newly created model.
			@note This method first constructs a SQL condition based on the provided attributes and attempts to find an existing model that matches. If a matching model is found, it is returned. If no matching model exists, a new model is created with the provided attributes and values, and then returned.
        */
        static std::shared_ptr<Derived> firstOrCreate(
            const std::map<std::string, std::string>& attributes,
            const std::map<std::string, std::string>& values = {},
            const std::shared_ptr<Database>& db = getDefaultDatabase()
        ) {
            std::string condition;
            for (const auto& [key, value] : attributes) {
                if (!condition.empty()) condition += " AND ";
                condition += key + " = '" + value + "'";
            }

            auto existing = where(condition, db);
            if (!existing.empty()) return existing.front(db);

            auto allValues = attributes;
            allValues.insert(values.begin(), values.end());
            return create(allValues, true, db);
        }

        void fill(const std::map<std::string, std::string>& data) {
            for (const auto& [key, value] : data) {
                if (isFillable(key)) {
                    setAttribute(key, value);
                }
            }
        }

        std::map<std::string, std::string> getAttributes() const {
            return attributes;
        }

        bool hasAttribute(const std::string& key) const {
            return attributes.find(key) != attributes.end();
        }

        static int count(const std::string& condition = "1=1", const std::shared_ptr<Database>& db = getDefaultDatabase()) {
            auto results = query()
                .Select("COUNT(*) as total")
                .Where(condition)
                .get();
            // remark : Technically, models from query() doesn't need assigned `db` here unless the data is used and not just read. I assigned it for safety.

            for (const auto& row : results) row->setDatabase(db);
            return results.empty() ? 0 :
                std::stoi(results[0]->getAttribute("total"));
        }

        static int max(const std::string& column, const std::string& condition = "1=1", const std::shared_ptr<Database>& db = getDefaultDatabase()) {
            auto results = query()
                .Select("MAX(" + column + ") as max")
                .Where(condition)
                .get();

            for (const auto& row : results) row->setDatabase(db);
            return results.empty() ? 0 :
                std::stoi(results[0]->getAttribute("max"));
        }

        static int sum(const std::string& column, const std::string& condition = "1=1", const std::shared_ptr<Database>& db = getDefaultDatabase()) {
            auto results = query()
                .Select("SUM(" + column + ") as sum")
                .Where(condition)
                .get();

            for (const auto& row : results) row->setDatabase(db);
            return results.empty() ? 0 :
                std::stoi(results[0]->getAttribute("sum"));
        }

        std::string operator[] (std::string key) {
            return this->getAttribute(key);
        }

        json toJson() const {
            json j;

            for (const auto& [key, value] : attributes) {
                j[key] = value;
            }

            return j;
        }

        /*
			@brief Defines a belongs-to relationship between the current model and a related model.
			@param foreignKey The foreign key in the current model that references the related model.
			@param ownerKey The primary key in the related model that is referenced by the foreign key (default is the primary key of the related model).
			@return A shared pointer to the related model that is associated with the current model, or nullptr if no related model is found.
			@note This method retrieves the first instance of the related model where the owner key matches the value of the foreign key in the current model. If the foreign key value is empty, it returns nullptr.
        */
        template<typename RelatedModel>
        std::shared_ptr<RelatedModel> belongsTo(
            const std::string& foreignKey,
            const std::string& ownerKey = Derived::primary_key
        ) {
            auto foreignValue = getAttribute(foreignKey);
            if (foreignValue.empty()) return nullptr;
            return RelatedModel::where(ownerKey + " = " + foreignValue, database).front();
        }

        /*
			@brief Defines a one-to-many relationship between the current model and a related model.
			@param foreignKey The foreign key in the related model that references the current model.
			@param localKey The local key in the current model that is referenced by the foreign key (default is the primary key of the current model).
			@return A vector of shared pointers to the related models that are associated with the current model.
			@note This method retrieves all instances of the related model where the foreign key matches the value of the local key in the current model. If the local key value is empty, it returns an empty vector.
        */
        template<typename RelatedModel>
        std::vector<std::shared_ptr<RelatedModel>> hasMany(
            const std::string& foreignKey,
            const std::string& localKey = Derived::primary_key
        ) {
            auto localValue = getAttribute(localKey);
            if (localValue.empty()) return {};
            return RelatedModel::where(foreignKey + " = " + localValue, database);
        }

        /*
			@brief Paginate the results of a query.
			@param page The page number to retrieve (1-based index).
			@param perPage The number of items per page.
			@param condition Optional SQL condition to filter the results (default is "1=1" which means no filtering).
			@return A pair containing a Collection of the paginated results and the total count of items matching the condition.
			@note This method calculates the offset based on the page number and items per page, retrieves the corresponding items, and also counts the total number of items that match the given condition.
        */
        static std::pair<Collection<Derived>, int> paginate(
            int page,
            int perPage,
            const std::string& condition = "1=1",
            const std::shared_ptr<Database>& db = getDefaultDatabase()
        ) {
            int offset = (page - 1) * perPage;
            auto items = query()
                .Where(condition)
                .Limit(perPage)
                .Offset(offset)
                .get();
            for (const auto& row : items) row->setDatabase(db);

            auto countQuery = query()
                .Select("COUNT(*) as total")
                .Where(condition)
                .get();
            for (const auto& row : countQuery) row->setDatabase(db);

            int total = countQuery.empty() ? 0 :
                std::stoi(countQuery[0]->getAttribute("total"));

            return { Collection<Derived>(items), total };
        }

        void debugAttributes() {
            for (const auto& field : fields) {
                Logger::log("Field: " + field + " = " + this->getAttribute(field), "DEBUG");
            }
        }

        static Collection<Derived> collect(const std::vector<std::shared_ptr<Derived>>& items) {
            return Collection<Derived>(items);
        }
    };
}