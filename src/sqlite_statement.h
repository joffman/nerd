#ifndef NERD_SQLITE_STATEMENT_H
#define NERD_SQLITE_STATEMENT_H

#include <exception>
#include <string>

#include <sqlite3.h>

namespace nerd {

class SQLiteColumnNull : public std::exception {
};

class SQLiteStatement {
public:
    SQLiteStatement(sqlite3* db, const std::string& stmt_str);

    ~SQLiteStatement();

    int step();


    //////////////////////////////
    // bind functions
    //////////////////////////////

    void bind_null(int pos);
    void bind_int(int pos, int val);
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

}   // nerd

#endif  // NERD_SQLITE_STATEMENT_H
