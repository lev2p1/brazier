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

#include "../../include/brazier/Database/Database.hpp"

using namespace brazier;

Database::Database() {
    std::string db_host     = global_config->get("database.host", "localhost");
    std::string db_port     = global_config->get("database.port", "5432");
	std::string db_user     = global_config->get("database.username", "postgres");
	std::string db_password = global_config->get("database.password", "");
    std::string db_name     = global_config->get("database.database", "postgres");

    std::string connection_string = "host=" + db_host + " user=" + db_user + " password=" + db_password + " dbname=" + db_name + " client_encoding=UTF8";
    conn_ = PQconnectdb(connection_string.c_str());
    Logger::log("Successfully connected to database " + db_name, "SUCCESS");
    if (PQstatus(conn_) != CONNECTION_OK) {
        Logger::log("Connection failed: " + std::string(PQerrorMessage(conn_)), "ERROR");
        throw std::runtime_error("Connection failed: " + std::string(PQerrorMessage(conn_)));
    }
}

Database::~Database() {
    if (conn_) {
        PQfinish(conn_);
    }
}

void brazier::Database::execute(const std::string& query) {
    Logger::log("SQL Query: " + query, "INFO");

    PGresult* res = PQexec(conn_, query.c_str());
    ExecStatusType status = PQresultStatus(res);

    if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
        std::string error = PQerrorMessage(conn_);
        PQclear(res);

        if (in_transaction_) {
            PQexec(conn_, "ROLLBACK");
            in_transaction_ = false;
        }

        throw std::runtime_error(error);
    }
    PQclear(res);
}

void brazier::Database::transaction(const std::string name)
{
    std::string sql;
    if (name.empty()) {
        sql = "BEGIN";
        in_transaction_ = true;
    }
    else {
        if (!in_transaction_) {
            throw std::runtime_error("Cannot create savepoint outside of a transaction");
        }
        sql = "SAVEPOINT \"" + name + "\"";
    }

    PGresult* res = PQexec(conn_, sql.c_str());

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        Logger::log(std::string(PQerrorMessage(conn_)), "ERROR");
        PQclear(res);
        throw std::runtime_error("Failed to start transaction: " + std::string(PQerrorMessage(conn_)));
    }

    PQclear(res);
}

void brazier::Database::rollback(const std::string name)
{
    if (!in_transaction_) {
        Logger::log("No active transaction to rollback", "WARNING");
        return;
    }
    
    std::string sql;
    if (name.empty()) {
        sql = "ROLLBACK";
        in_transaction_ = false;
    }
    else {
        sql = "ROLLBACK TO SAVEPOINT \"" + name + "\"";
    }

    PGresult* res = PQexec(conn_, sql.c_str());

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::string error = PQerrorMessage(conn_);
        PQclear(res);
        in_transaction_ = false;
        throw std::runtime_error("Failed to rollback: " + error);
    }

    PQclear(res);
    
    if (!name.empty()) {
        in_transaction_ = true;
    }
}

void brazier::Database::commit()
{
    PGresult* res = PQexec(conn_, "COMMIT");
    Logger::log("Commit transaction", "INFO");

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        Logger::log(std::string(PQerrorMessage(conn_)), "ERROR");
        PQclear(res);
        throw std::runtime_error("Failed to commit: " + std::string(PQerrorMessage(conn_)));
    }

    PQclear(res);
    in_transaction_ = false;
}

std::string Database::query(const std::string& sql) {
    PGresult* res = PQexec(conn_, sql.c_str());

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        Logger::log(std::string(PQerrorMessage(conn_)), "ERROR");
        PQclear(res);
        throw std::runtime_error(std::string(PQerrorMessage(conn_)));
    }

    std::string result;
    int rows = PQntuples(res);
    int cols = PQnfields(res);

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            result += PQgetvalue(res, i, j);
            result += " ";
        }
        result += "\n";
    }

    PQclear(res);
    Logger::log("Successfull query execution", "INFO");
    return result;
}
/*
	@brief Executes a SQL query with parameters and returns the result as a vector of maps.
	@param sql_template The SQL query template with placeholders for parameters (use '?' for placeholders).
	@param params A vector of parameters to replace the placeholders in the SQL template.
*/
std::vector<std::map<std::string, std::string>> Database::queryToVector(
    const std::string& sql_template,
    const std::vector<std::string>& params)
{
    size_t param_count = std::count(sql_template.begin(), sql_template.end(), '?');
    if (param_count != params.size()) {
        throw std::runtime_error("Parameter count mismatch. Expected " +
            std::to_string(param_count) +
            ", got " + std::to_string(params.size()));
    }

    std::string sql = sql_template;
    for (const auto& param : params) {
        size_t pos = sql.find("?");
        if (pos != std::string::npos) {
            std::string escaped_param = SQLString::EscapeString(conn_, param);
            sql.replace(pos, 1, escaped_param);
        }
    }

    PGresult* res = PQexec(conn_, sql.c_str());
    Logger::log("SQL Query: " + sql, "INFO");

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        Logger::log(std::string(PQerrorMessage(conn_)), "ERROR");
        PQclear(res);
        throw std::runtime_error(std::string(PQerrorMessage(conn_)));
    }

    std::vector<std::map<std::string, std::string>> result;
    int rows = PQntuples(res);
    int cols = PQnfields(res);

    std::vector<std::string> columnNames;
    for (int j = 0; j < cols; ++j) {
        columnNames.push_back(PQfname(res, j));
    }

    for (int i = 0; i < rows; ++i) {
        std::map<std::string, std::string> row;
        for (int j = 0; j < cols; ++j) {
            row[columnNames[j]] = PQgetvalue(res, i, j) ? PQgetvalue(res, i, j) : "";
        }
        result.push_back(row);
    }

    PQclear(res);
    return result;
}

/*
	@brief Executes a SQL query with parameters and returns the first row as a map.
	@param sql_template The SQL query template with placeholders for parameters (use '?' for placeholders).
	@param params A vector of parameters to replace the placeholders in the SQL template.
*/
std::map<std::string, std::string> Database::queryMap(
    const std::string& sql_template,
    const std::vector<std::string>& params) {

    size_t param_count = std::count(sql_template.begin(), sql_template.end(), '?');
    if (param_count != params.size()) {
        throw std::runtime_error("Parameter count mismatch. Expected " +
            std::to_string(param_count) +
            ", got " + std::to_string(params.size()));
    }

    std::string sql = sql_template;
    for (const auto& param : params) {
        size_t pos = sql.find("?");
        if (pos != std::string::npos) {
            std::string escaped_param = SQLString::EscapeString(conn_, param);
            sql.replace(pos, 1, escaped_param);
        }
    }

    PGresult* res = PQexec(conn_, sql.c_str());
    Logger::log("SQL Query: " + sql, "INFO");
    std::map<std::string, std::string> row;

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        Logger::log(std::string(PQerrorMessage(conn_)), "ERROR");
        PQclear(res);
        throw std::runtime_error(std::string(PQerrorMessage(conn_)));
    }

    if (PQntuples(res) > 0) {
        for (int i = 0; i < PQnfields(res); i++) {
            row[PQfname(res, i)] = PQgetvalue(res, 0, i) ? PQgetvalue(res, 0, i) : "";
        }
    }

    PQclear(res);
    return row;
}