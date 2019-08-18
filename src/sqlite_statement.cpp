#include <limits>
#include <stdexcept>
#include <string>

#include "sqlite_statement.h"

namespace nerd {

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

}   // nerd
