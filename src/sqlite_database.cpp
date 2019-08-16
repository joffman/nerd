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
            std::string("preparing SQL statement failed: ") + sqlite3_errmsg(m_db));
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

void SQLiteStatement::bind_int(int pos, int val)
{
    int rc = sqlite3_bind_int(m_stmt, pos, val);
    if (rc != SQLITE_OK)
        throw std::runtime_error(std::string("bind_int failed: ") + sqlite3_errmsg(m_db));
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
    // TODO Don't use exception. Use std::optional instead as return value.
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
    const unsigned char* ptr = sqlite3_column_text(m_stmt, pos);
    if (ptr)
        return std::string(reinterpret_cast<const char*>(ptr));
    else
        throw SQLiteColumnNull();
}


////////////////////////////////////////////////////////////////////////////////
// SQLiteDatabase
////////////////////////////////////////////////////////////////////////////////

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

int SQLiteDatabase::create_card(const json& data)
{
    // Check pre-condition. TODO Check for correct type.
    if (data.find("title") == data.end() || data.find("question") == data.end())
        throw std::runtime_error("create_card: title or question missing");

    // Create SQL-statement.
    SQLiteStatement stmt(
        m_db,
        R"RAW(INSERT INTO card
                (title, question, answer, topic)
                VALUES ($1, $2, $3, $4);
            )RAW");

    stmt.bind_text(1, data["title"]);
    stmt.bind_text(2, data["question"]);
    if (data.find("answer") != data.end())
        stmt.bind_text(3, data["answer"]);
    else
        stmt.bind_null(3);
    if (data.find("topic") != data.end())
        stmt.bind_int(4, data["topic"]);
    else
        stmt.bind_int(4, 0);    // set default topic

    // Execute statement and return last-insert id.
    if (stmt.step() != SQLITE_DONE)
        throw std::runtime_error(std::string("cannot create card: ") + sqlite3_errmsg(m_db));

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
        throw std::runtime_error(std::string("fetching cards from database failed: ")
                                 + sqlite3_errmsg(m_db));

    return result;
}

json SQLiteDatabase::get_card(int id) const
{
    // Create SQL-statement.
    SQLiteStatement stmt(
        m_db,
        R"RAW(SELECT id, title, question, answer, topic
                from card
                WHERE id = $1;)RAW");
    stmt.bind_int(1, id);

    // Fetch result.
    {
        int rc;
        if ((rc = stmt.step()) != SQLITE_ROW) {
            throw std::runtime_error(
                std::string("error when fetching card with id ") + std::to_string(id));
        }
    }

    // Create json.
    json result;
    result["id"] = stmt.column_int(0);
    result["title"] = stmt.column_text(1);
    result["question"] = stmt.column_text(2);
    try {
        result["answer"] = stmt.column_text(3);
    } catch (const SQLiteColumnNull&) {
        // answer is NULL
    }
    result["topic"] = stmt.column_int(4);

    // Check for errors.
    int rc = stmt.step();
    if (rc != SQLITE_DONE)
        throw std::logic_error(std::string("internal error when fetching card"));

    return result;
}

void SQLiteDatabase::update_card(int id, const json& data)
{
    // Check pre-condition. TODO Check for correct type.
    if (data.find("title") == data.end()
        || data.find("question") == data.end())
        throw std::runtime_error("create_card: title or question missing");

    // Create SQL-statement.
    SQLiteStatement stmt(
        m_db,
        R"RAW(UPDATE card
                SET title = $1, question = $2, answer = $3, topic = $4
                WHERE id = $5;
            )RAW");

    stmt.bind_text(1, data["title"]);
    stmt.bind_text(2, data["question"]);
    if (data.find("answer") != data.end())
        stmt.bind_text(3, data["answer"]);
    else
        stmt.bind_null(3);
    if (data.find("topic") != data.end())
        stmt.bind_int(4, data["topic"]);
    else
        stmt.bind_int(4, 0);    // set default topic
    stmt.bind_int(5, id);

    // Execute statement.
    if (stmt.step() != SQLITE_DONE)
        throw std::runtime_error(std::string("cannot update card: ") + sqlite3_errmsg(m_db));
}

void SQLiteDatabase::delete_card(int id)
{
    // Create SQL-statement.
    SQLiteStatement stmt(
        m_db,
        R"RAW(DELETE FROM card WHERE id = $1;)RAW");

    stmt.bind_int(1, id);

    // Execute statement.
    if (stmt.step() != SQLITE_DONE)
        throw std::runtime_error(std::string("cannot delete card: ") + sqlite3_errmsg(m_db));
}

}   // nerd
