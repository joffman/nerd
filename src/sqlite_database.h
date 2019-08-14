#ifndef NERD_SQLITE_DATABASE_H
#define NERD_SQLITE_DATABASE_H

#include <string>

#include <json.hpp>
#include <sqlite3.h>

#include "names.h"

namespace nerd {

class SQLiteStatement {
public:
    SQLiteStatement(sqlite3* db, const std::string& stmt_str);

    ~SQLiteStatement();

    int step();


    //////////////////////////////
    // bind functions
    //////////////////////////////

    void bind_null(int pos);

    void bind_text(int pos, const std::string& text);
    

    //////////////////////////////
    // column functions
    //////////////////////////////

    int column_int(int pos);

    std::string column_text(int pos);

private:
    sqlite3* m_db;
    sqlite3_stmt* m_stmt;
};

class SQLiteDatabase {
public:
    explicit SQLiteDatabase(const char* filename);

    SQLiteDatabase(SQLiteDatabase&& other);

    ~SQLiteDatabase();

    // Return true on success.
    bool init();

    // Returns id of newly created card.
    // Throws if object can't be created.
    int create_card(const json& data);

    // Get all cards from database.
    // TODO Allow filters.
    json get_cards() const;
    
private:
    sqlite3* m_db;
};

}   // nerd

#endif  // NERD_SQLITE_DATABASE_H
