#ifndef NERD_SQLITE_TABLE_H
#define NERD_SQLITE_TABLE_H

#include <string>
#include <unordered_map>

#include <json.hpp>
#include <sqlite3.h>

#include "names.h"
#include "sqlite_statement.h"

namespace nerd {

class SQLiteTable {
protected:
    SQLiteTable(sqlite3* db);

public:
    // Returns id of newly inserted object.
    // Throws if object can't be inserted.
    int insert(const json& data);

    // Return all objects from database.
    // todo Add filter.
    json get(const std::unordered_map<std::string, std::string>& filter=std::unordered_map<std::string, std::string>()) const;
    
    // Return all details from object with given id.
    json get_one(int id) const;

    // Update object with given id.
    void update(int id, const json& data);

    // Delete object with given id.
    void remove(int id);

private:    // or protected??
    virtual std::string get_table_name() const = 0;
    virtual bool data_is_valid(const json& data) const = 0;

    virtual SQLiteStatement insert_statement(const json& data) const = 0;
    // TODO: Use map of arbitrary value types.
    virtual SQLiteStatement get_statement(
        const std::unordered_map<std::string, std::string>& filter) const = 0;
    virtual SQLiteStatement get_one_statement(int id) const = 0;
    virtual SQLiteStatement update_statement(int id, const json& data) const = 0;
    SQLiteStatement delete_statement(int id) const;

    virtual json fetch_one_shallow(SQLiteStatement& stmt) const = 0;
    virtual json fetch_one_detailed(SQLiteStatement& stmt) const = 0;

protected:
    sqlite3* m_db;
};

class CardSQLiteTable : public SQLiteTable {
public:
    CardSQLiteTable(sqlite3* db);

private:
    std::string get_table_name() const override;

    bool data_is_valid(const json& data) const override;

    SQLiteStatement insert_statement(const json& data) const override;
    SQLiteStatement get_statement(
        const std::unordered_map<std::string, std::string>& filter) const override;
    SQLiteStatement get_one_statement(int id) const override;
    SQLiteStatement update_statement(int id, const json& data) const override;

    json fetch_one_shallow(SQLiteStatement& stmt) const override;
    json fetch_one_detailed(SQLiteStatement& stmt) const override;
};
    
class TopicSQLiteTable : public SQLiteTable {
public:
    TopicSQLiteTable(sqlite3* db);

private:
    std::string get_table_name() const override;

    bool data_is_valid(const json& data) const override;

    SQLiteStatement insert_statement(const json& data) const override;
    SQLiteStatement get_statement(
        const std::unordered_map<std::string, std::string>& filter) const override;
    SQLiteStatement get_one_statement(int id) const override;
    SQLiteStatement update_statement(int id, const json& data) const override;

    json fetch_one_shallow(SQLiteStatement& stmt) const override;
    json fetch_one_detailed(SQLiteStatement& stmt) const override;
};
    
}   // nerd

#endif  // NERD_SQLITE_TABLE_H
