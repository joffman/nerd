#include <exception>
#include <string>

#include <json.hpp>
#include <sqlite3.h>

#include "names.h"
#include "sqlite_database.h"

namespace nerd {

////////////////////////////////////////////////////////////////////////////////
// SQLiteStatement
////////////////////////////////////////////////////////////////////////////////

SQLiteStatement::SQLiteStatement(sqlite3* db, const std::string& stmt_str)
    : m_db{db}
{
    int rc = sqlite3_prepare_v2(
        m_db,
        stmt_str.c_str(),
        -1,     // read until null-terminator
        &m_stmt,
        nullptr);
    if (rc != SQLITE_OK)
        throw std::runtime_error(
            std::string("preparing SQL statment failed: ") + sqlite3_errmsg(m_db));
}

SQLiteStatement::~SQLiteStatement()
{
    sqlite3_finalize(m_stmt);
}

int SQLiteStatement::step()
{
    return sqlite3_step(m_stmt);
}


//////////////////////////////
// bind functions
//////////////////////////////

void SQLiteStatement::bind_null(int pos)
{
    if (sqlite3_bind_null(m_stmt, pos) != SQLITE_OK)
        throw std::runtime_error(std::string("bind_null failed: ") + sqlite3_errmsg(m_db));
}

void SQLiteStatement::bind_text(int pos, const std::string& text)
{
    if (text.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument("String too long."); 
    }

    int rc = sqlite3_bind_text(
        m_stmt, pos, text.c_str(),
        -1 /* null-terminated */, SQLITE_TRANSIENT);

    if (rc != SQLITE_OK)
        throw std::runtime_error(std::string("bind_text failed: ") + sqlite3_errmsg(m_db));
}

//////////////////////////////
// column functions
//////////////////////////////

int SQLiteStatement::column_int(int pos)
{
    return sqlite3_column_int(m_stmt, pos);
}

std::string SQLiteStatement::column_text(int pos)
{
    return std::string(reinterpret_cast<const char*>(sqlite3_column_text(m_stmt, pos)));
}


////////////////////////////////////////////////////////////////////////////////
// SQLiteDatabase
////////////////////////////////////////////////////////////////////////////////

SQLiteDatabase::SQLiteDatabase(const char* filename)
{
    int rc = sqlite3_open(filename, &m_db);
    if (rc != SQLITE_OK)
        throw std::runtime_error(
            std::string("can't open database: ") + sqlite3_errmsg(m_db));
}

SQLiteDatabase::SQLiteDatabase(SQLiteDatabase&& other)
    : m_db{other.m_db}
{
    other.m_db = nullptr;
}

SQLiteDatabase::~SQLiteDatabase()
{
    if (sqlite3_close(m_db))
        throw std::runtime_error(std::string("can't close database: ")
                                 + sqlite3_errmsg(m_db));
}

bool SQLiteDatabase::init()
{
    SQLiteStatement stmt(
        m_db,
        R"RAW(CREATE TABLE IF NOT EXISTS card (
                id          INTEGER UNIQUE PRIMARY KEY  NOT NULL,
                title       TEXT                        NOT NULL,
                question    TEXT                        NOT NULL,
                answer      TEXT
            ))RAW");
        return stmt.step() == SQLITE_DONE;
}

int SQLiteDatabase::create_card(const json& data)
{
    // Check pre-condition. TODO Check for correct type.
    if (data.find("title") == data.end() || data.find("question") == data.end())
        throw std::runtime_error("create_card: title or question missing");

    // Create SQL-statement.
    SQLiteStatement stmt(
        m_db,
        R"RAW(INSERT INTO card
                (title, question, answer)
                VALUES ($1, $2, $3);
            )RAW");

        stmt.bind_text(1, data["title"]);
    stmt.bind_text(2, data["question"]);
    if (data.find("answer") != data.end())
        stmt.bind_text(3, data["answer"]);
    else
        stmt.bind_null(3);

    // Execute statement and return last-insert id.
    if (stmt.step() != SQLITE_DONE)
        throw std::runtime_error(std::string("can't create user: ") + sqlite3_errmsg(m_db));

    return sqlite3_last_insert_rowid(m_db);
}

json SQLiteDatabase::get_cards() const
{
    // Create SQL-statement.
    SQLiteStatement stmt(
        m_db,
        R"RAW(SELECT id, title from card;)RAW");

    // Fetch results.
    json result = json::array();
    int rc;
    while ((rc = stmt.step()) == SQLITE_ROW) {
        json c;
        c["id"] = stmt.column_int(0);
        c["title"] = stmt.column_text(1);
        result.push_back(c);
    }

    // Check for errors.
    if (rc != SQLITE_DONE)
        throw std::runtime_error(std::string("can't create user: ") + sqlite3_errmsg(m_db));

    return result;
}

}   // nerd
