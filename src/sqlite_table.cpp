#include <stdexcept>
#include <string>
#include <unordered_map>

#include <json.hpp>
#include <sqlite3.h>

#include "sqlite_statement.h"
#include "sqlite_table.h"

namespace nerd {

////////////////////////////////////////////////////////////////////////////////
// SQLiteTable
////////////////////////////////////////////////////////////////////////////////

SQLiteTable::SQLiteTable(sqlite3* db)
: m_db(db) {}

int SQLiteTable::insert(const json& data)
{
    if (!data_is_valid(data))
        throw std::runtime_error("insert: invalid data");

    SQLiteStatement stmt = insert_statement(data);

    // Execute statement and return last-insert id.
    if (stmt.step() != SQLITE_DONE)
        throw std::runtime_error(std::string("cannot insert object: ") + sqlite3_errmsg(m_db));

    return sqlite3_last_insert_rowid(m_db);
}

json SQLiteTable::get(const std::unordered_map<std::string, std::string>& filter) const
{
    SQLiteStatement stmt = get_statement(filter);

    // Fetch results.
    json result = json::array();
    int rc;
    while ((rc = stmt.step()) == SQLITE_ROW) {
        json obj = fetch_one_shallow(stmt);
        result.push_back(obj);
    }

    // Check for errors.
    if (rc != SQLITE_DONE)
        throw std::runtime_error(std::string("fetching all objects from table failed: ")
                                 + sqlite3_errmsg(m_db));

    return result;
}

json SQLiteTable::get_one(int id) const
{
    SQLiteStatement stmt = get_one_statement(id);
    {   // Execute statement.
        int rc;
        if ((rc = stmt.step()) != SQLITE_ROW) {
            throw std::runtime_error(
                std::string("error when fetching object with id ") + std::to_string(id));
        }
    }
    json result = fetch_one_detailed(stmt);

    // Check for errors.
    if (stmt.step() != SQLITE_DONE)
        throw std::logic_error(std::string("internal error when fetching object"));

    return result;
}

void SQLiteTable::update(int id, const json& data)
{
    if (!data_is_valid(data))
        throw std::runtime_error("update: invalid_data");

    SQLiteStatement stmt = update_statement(id, data);

    // Execute statement.
    if (stmt.step() != SQLITE_DONE)
        throw std::runtime_error(std::string("cannot update object: ") + sqlite3_errmsg(m_db));
}

void SQLiteTable::remove(int id)
{
    SQLiteStatement stmt =
        delete_statement(id);
    if (stmt.step() != SQLITE_DONE)
        throw std::runtime_error(std::string("cannot delete object: ") + sqlite3_errmsg(m_db));
}

SQLiteStatement SQLiteTable::delete_statement(int id) const
{
    SQLiteStatement stmt(
        m_db,
        "DELETE FROM " + get_table_name() + " WHERE id = $1;");
    stmt.bind_int(1, id);

    return stmt;
}


////////////////////////////////////////////////////////////////////////////////
// CardSQLiteTable
////////////////////////////////////////////////////////////////////////////////

CardSQLiteTable::CardSQLiteTable(sqlite3* db)
: SQLiteTable(db) {}


std::string CardSQLiteTable::get_table_name() const
{
    return "card";
}

bool CardSQLiteTable::data_is_valid(const json& data) const
{
    // todo Check for correct type.
    return data.find("title") != data.end() && data["title"] != ""
        && data.find("question") != data.end() && data["question"] != "";
}

SQLiteStatement CardSQLiteTable::insert_statement(const json& data) const
{
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

    return stmt;
}

SQLiteStatement CardSQLiteTable::get_statement(
    const std::unordered_map<std::string, std::string>& filter) const
{
    std::string str("SELECT id, title from card");
    if (filter.find("topic") != filter.end())
        str += " WHERE topic = " + filter.at("topic");  // TODO error checking
    str += ";";

    SQLiteStatement stmt(
        m_db,
        str);

    return stmt;
}

SQLiteStatement CardSQLiteTable::get_one_statement(int id) const
{
    SQLiteStatement stmt(
        m_db,
        R"RAW(SELECT id, title, question, answer, topic
                from card
                WHERE id = $1;)RAW");
    stmt.bind_int(1, id);
    return stmt;
}

SQLiteStatement CardSQLiteTable::update_statement(int id, const json& data) const
{
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

    return stmt;
}


json CardSQLiteTable::fetch_one_shallow(SQLiteStatement& stmt) const
{
    json c;
    c["id"] = stmt.column_int(0);
    c["title"] = stmt.column_text(1);
    return c;
}

json CardSQLiteTable::fetch_one_detailed(SQLiteStatement& stmt) const
{
    json card;

    card["id"] = stmt.column_int(0);
    card["title"] = stmt.column_text(1);
    card["question"] = stmt.column_text(2);
    try {
        card["answer"] = stmt.column_text(3);
    } catch (const SQLiteColumnNull&) {
        // answer is NULL
    }
    card["topic"] = stmt.column_int(4);

    return card;
}


////////////////////////////////////////////////////////////////////////////////
// TopicSQLiteTable
////////////////////////////////////////////////////////////////////////////////

TopicSQLiteTable::TopicSQLiteTable(sqlite3* db)
: SQLiteTable(db) {}

std::string TopicSQLiteTable::get_table_name() const
{
    return "topic";
}

bool TopicSQLiteTable::data_is_valid(const json& data) const
{
    // todo Check for correct type.
    return data.find("name") != data.end() && data["name"] != "";
}

SQLiteStatement TopicSQLiteTable::insert_statement(const json& data) const
{
    SQLiteStatement stmt(
        m_db,
        R"RAW(INSERT INTO topic (name) VALUES ($1);)RAW");
    stmt.bind_text(1, data["name"]);
    return stmt;
}

SQLiteStatement TopicSQLiteTable::get_statement(
    const std::unordered_map<std::string, std::string>&) const
{
    SQLiteStatement stmt(
        m_db,
        R"RAW(SELECT id, name from topic;)RAW");
    return stmt;
}

SQLiteStatement TopicSQLiteTable::get_one_statement(int id) const
{
    SQLiteStatement stmt(
        m_db,
        R"RAW(SELECT id, name
                from topic
                WHERE id = $1;)RAW");
    stmt.bind_int(1, id);
    return stmt;
}

SQLiteStatement TopicSQLiteTable::update_statement(int id, const json& data) const
{
    SQLiteStatement stmt(
        m_db,
        R"RAW(UPDATE topic SET name = $1 WHERE id = $2;)RAW");

    stmt.bind_text(1, data["name"]);
    stmt.bind_int(2, id);

    return stmt;
}


json TopicSQLiteTable::fetch_one_shallow(SQLiteStatement& stmt) const
{
    json c;
    c["id"] = stmt.column_int(0);
    c["name"] = stmt.column_text(1);
    return c;
}

json TopicSQLiteTable::fetch_one_detailed(SQLiteStatement& stmt) const
{
    json topic;

    topic["id"] = stmt.column_int(0);
    topic["name"] = stmt.column_text(1);

    return topic;
}

}   // nerd
