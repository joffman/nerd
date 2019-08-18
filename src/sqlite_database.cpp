#include <stdexcept>
#include <string>
#include <vector>

#include <sqlite3.h>

#include "sqlite_database.h"
#include "sqlite_statement.h"

namespace nerd {

SQLiteDatabase::SQLiteDatabase(const char* filename)
{
    int rc = sqlite3_open(filename, &m_db);
    if (rc != SQLITE_OK)
        throw std::runtime_error(
            std::string("cannot open database: ") + sqlite3_errmsg(m_db));

    SQLiteStatement stmt(m_db, R"RAW(PRAGMA foreign_keys = ON;)RAW");
    if (stmt.step() != SQLITE_DONE)
        throw std::runtime_error(
            std::string("cannot set foreign_keys pragma: ") + sqlite3_errmsg(m_db));
}

SQLiteDatabase::SQLiteDatabase(SQLiteDatabase&& other)
    : m_db{other.m_db}
{
    other.m_db = nullptr;
}

SQLiteDatabase::~SQLiteDatabase()
{
    if (sqlite3_close(m_db))
        throw std::runtime_error(std::string("cannot close database: ")
                                 + sqlite3_errmsg(m_db));
}

sqlite3* SQLiteDatabase::data() const
{
    return m_db;
}

void SQLiteDatabase::init()
{
    const std::vector<std::string> raw_stmts = {
        R"RAW(
           CREATE TABLE IF NOT EXISTS topic (
                id          INTEGER UNIQUE PRIMARY KEY  NOT NULL,
                name        TEXT                        NOT NULL
            );
        )RAW",
        R"RAW(
            INSERT INTO topic (id, name) VALUES (0, "Default");
        )RAW",
        R"RAW(
            CREATE TABLE IF NOT EXISTS card (
                id          INTEGER UNIQUE PRIMARY KEY  NOT NULL,
                title       TEXT                        NOT NULL,
                question    TEXT                        NOT NULL,
                answer      TEXT                                ,
                topic       INTEGER REFERENCES topic ON UPDATE CASCADE ON DELETE SET DEFAULT DEFAULT 0
            );
        )RAW",
        R"RAW(
            CREATE INDEX topicindex ON card(topic);
                -- see: https://www.sqlite.org/foreignkeys.html
        )RAW"};

    for (const auto& raw : raw_stmts) {
        SQLiteStatement stmt(m_db, raw);
        if (stmt.step() != SQLITE_DONE)
            throw std::runtime_error(std::string("error during initialization: ")
                                     + sqlite3_errmsg(m_db));
    }
}

}   // nerd
