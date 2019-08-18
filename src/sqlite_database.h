#ifndef NERD_SQLITE_DATABASE_H
#define NERD_SQLITE_DATABASE_H

#include <sqlite3.h>

namespace nerd {

class SQLiteDatabase {
public:
    explicit SQLiteDatabase(const char* filename);

    SQLiteDatabase(SQLiteDatabase&& other);

    ~SQLiteDatabase();

    // Create database. Throw on error.
    void init();

    sqlite3* data() const;

private:
    sqlite3* m_db;
};

}   // nerd

#endif  // NERD_SQLITE_DATABASE_H
