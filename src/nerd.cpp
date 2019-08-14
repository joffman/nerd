//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: HTTP server, synchronous
//
//------------------------------------------------------------------------------

// TODO:
// - multi-threading synchronization?

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/config.hpp>

#include <cstdlib>      // EXIT_FAILURE
#include <exception>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <utility>      // move

#include <sqlite3.h>

#include "json.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
using json = nlohmann::json;

//------------------------------------------------------------------------------

class SQLiteStatement {
public:
    SQLiteStatement(sqlite3* db, const std::string& stmt_str)
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

    ~SQLiteStatement()
    {
        sqlite3_finalize(m_stmt);
    }

    void bind_null(int pos)
    {
        if (sqlite3_bind_null(m_stmt, pos) != SQLITE_OK)
            throw std::runtime_error(std::string("bind_null failed: ") + sqlite3_errmsg(m_db));
    }

    void bind_text(int pos, const std::string& text)
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

    int step()
    {
        return sqlite3_step(m_stmt);
    }

private:
    sqlite3* m_db;
    sqlite3_stmt* m_stmt;
};

class SQLiteDatabase {
public:
    explicit SQLiteDatabase(const char* filename)
    {
        int rc = sqlite3_open(filename, &m_db);
        if (rc != SQLITE_OK)
            throw std::runtime_error(
                std::string("can't open database: ") + sqlite3_errmsg(m_db));
    }

    SQLiteDatabase(SQLiteDatabase&& other)
    : m_db{other.m_db}
    {
        other.m_db = nullptr;
    }

    ~SQLiteDatabase()
    {
        if (sqlite3_close(m_db))
            throw std::runtime_error(std::string("can't close database: ") + sqlite3_errmsg(m_db));
    }

    // Return true on success.
    bool init()
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

    // Returns id of newly created card.
    // Throws if object can't be created.
    int create_card(const json& data)
    {
        // Check pre-condition.
        if (data.find("title") == data.end() || data.find("question") == data.end())
            throw std::runtime_error("create_card: title or question missing");

        // Create SQL-statement.
        SQLiteStatement stmt(
            m_db,
            R"RAW(INSERT INTO card
                (title, question, answer)
                VALUES ($1, $2, $3)
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

    json get_cards() const
    {
//        enum query_columns { USERNAME = 0, FULL_NAME = 1, IS_ADMIN = 2 };
//
//        SQLiteStatement st = GET_STATEMENT(m_db, sql::list_users);
//
//        if (filter.find("username") != filter.end())
//            st->bind_string(1, "%" + filter.at("username") + "%");
//        else
//            st->bind_string(1, "%");
//
//        if (filter.find("full_name") != filter.end())
//            st->bind_string(2, "%" + filter.at("full_name") + "%");
//        else
//            st->bind_string(2, "%");
//
//        if (filter.find("is_admin") != filter.end())
//            st->bind_string(3, filter.at("is_admin"));
//        else
//            st->bind_string(3, "%");
//
//        st->execute();
//        while (st->fetch_row()) {
//            UserInfo user;
//            user.username = st->column_string(USERNAME);
//            user.full_name = st->column_string(FULL_NAME);
//            user.is_admin = st->column_integer(IS_ADMIN);
//            users.push_back(std::move(user));
//        }


        json cards = json::array();
//        for (const UserInfo& user : result.cards) {
//            json user_object = json::object();
//            user_object["username"] = user.username;
//            user_object["full_name"] = user.full_name;
//            user_object["is_admin"] = user.is_admin;
//            cards.push_back(std::move(user_object));
//        }
        return cards;
    }
private:
    sqlite3* m_db;
};

class HttpServer {
public:
    HttpServer(SQLiteDatabase db,
               net::ip::address addr, unsigned short port,
               std::string doc_root)
    : m_db{std::move(db)}
    , m_ioctx{1}
    , m_acceptor{m_ioctx, {addr, port}}
    , m_doc_root{std::move(doc_root)}
    {
    }

    void run()
    {
        for(;;) {
            // This will receive the new connection
            tcp::socket socket{m_ioctx};

            // Block until we get a connection
            m_acceptor.accept(socket);

            // Launch the session, transferring ownership of the socket
            std::thread{std::bind(
                    &HttpServer::do_session,
                    this,           // shared_ptr?
                    std::move(socket))}.detach();
        }
    }

private:

    //////////////////////////////
    // Static functions.
    //////////////////////////////

    // Return a reasonable mime type based on the extension of a file.
    static beast::string_view mime_type(beast::string_view path)
    {
        using beast::iequals;
        auto const ext = [&path]
        {
            auto const pos = path.rfind(".");
            if (pos == beast::string_view::npos)
                return beast::string_view{};
            return path.substr(pos);
        }();
        if (iequals(ext, ".htm"))  return "text/html";
        if (iequals(ext, ".html")) return "text/html";
        if (iequals(ext, ".php"))  return "text/html";
        if (iequals(ext, ".css"))  return "text/css";
        if (iequals(ext, ".txt"))  return "text/plain";
        if (iequals(ext, ".js"))   return "application/javascript";
        if (iequals(ext, ".json")) return "application/json";
        if (iequals(ext, ".xml"))  return "application/xml";
        if (iequals(ext, ".swf"))  return "application/x-shockwave-flash";
        if (iequals(ext, ".flv"))  return "video/x-flv";
        if (iequals(ext, ".png"))  return "image/png";
        if (iequals(ext, ".jpe"))  return "image/jpeg";
        if (iequals(ext, ".jpeg")) return "image/jpeg";
        if (iequals(ext, ".jpg"))  return "image/jpeg";
        if (iequals(ext, ".gif"))  return "image/gif";
        if (iequals(ext, ".bmp"))  return "image/bmp";
        if (iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
        if (iequals(ext, ".tiff")) return "image/tiff";
        if (iequals(ext, ".tif"))  return "image/tiff";
        if (iequals(ext, ".svg"))  return "image/svg+xml";
        if (iequals(ext, ".svgz")) return "image/svg+xml";
        return "application/text";
    }

    // Report a failure
    static void fail(beast::error_code ec, char const* what)
    {
        std::cerr << what << ": " << ec.message() << "\n";
    }

    // Append an HTTP rel-path to a local filesystem path.
    // The returned path is normalized for the platform.
    static std::string path_cat(
        beast::string_view base,
        beast::string_view path)
    {
        if (base.empty())
            return std::string(path);
        std::string result(base);
#ifdef BOOST_MSVC
        char constexpr path_separator = '\\';
        if (result.back() == path_separator)
            result.resize(result.size() - 1);
        result.append(path.data(), path.size());
        for(auto& c : result) {
            if (c == '/')
                c = path_separator;
        }
#else
        char constexpr path_separator = '/';
        if (result.back() == path_separator)
            result.resize(result.size() - 1);
        result.append(path.data(), path.size());
#endif
        return result;
    }


    //////////////////////////////
    // Helper classes
    //////////////////////////////

    // This is the C++11 equivalent of a generic lambda.
    // The function object is used to send an HTTP message.
    template<class Stream>
    struct send_lambda {
        Stream& stream_;
        bool& close_;
        beast::error_code& ec_;

        explicit send_lambda(
            Stream& stream,
            bool& close,
            beast::error_code& ec)
            : stream_(stream)
              , close_(close)
              , ec_(ec)
        {
        }

        template<bool isRequest, class Body, class Fields>
        void
        operator()(http::message<isRequest, Body, Fields>&& msg) const
        {
            // Determine if we should close the connection after
            close_ = msg.need_eof();

            // We need the serializer here because the serializer requires
            // a non-const file_body, and the message oriented version of
            // http::write only works with const messages.
            http::serializer<isRequest, Body, Fields> sr{msg};
            http::write(stream_, sr, ec_);
        }
    };


    //////////////////////////////
    // Non-static functions
    //////////////////////////////

    // Handles an HTTP server connection
    void do_session(
        tcp::socket& socket)
    {
        bool close = false;
        beast::error_code ec;

        // This buffer is required to persist across reads
        beast::flat_buffer buffer;

        // This lambda is used to send messages
        send_lambda<tcp::socket> lambda{socket, close, ec};

        for(;;) {
            // Read a request
            http::request<http::string_body> req;
            http::read(socket, buffer, req, ec);
            if (ec == http::error::end_of_stream)
                break;
            if (ec)
                return fail(ec, "read");

            // Send the response
            handle_request(std::move(req), lambda);
            if (ec)
                return fail(ec, "write");
            if (close) {
                // This means we should close the connection, usually because
                // the response indicated the "Connection: close" semantic.
                break;
            }
        }

        // Send a TCP shutdown
        socket.shutdown(tcp::socket::shutdown_send, ec);

        // At this point the connection is closed gracefully
    }

    // This function produces an HTTP response for the given
    // request. The type of the response object depends on the
    // contents of the request, so the interface requires the
    // caller to pass a generic lambda for receiving the response.
    template<class Body, class Allocator, class Send>
    void handle_request(
        http::request<Body, http::basic_fields<Allocator>>&& req,
        Send&& send)
    {
        // Returns a bad request response
        auto const bad_request =
            [&req](beast::string_view why)
            {
                http::response<http::string_body> res{http::status::bad_request, req.version()};
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::content_type, "text/html");
                res.keep_alive(req.keep_alive());
                res.body() = std::string(why);
                res.prepare_payload();
                return res;
            };

        // Returns a not found response
        auto const not_found =
            [&req](beast::string_view target)
            {
                http::response<http::string_body> res{http::status::not_found, req.version()};
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::content_type, "text/html");
                res.keep_alive(req.keep_alive());
                res.body() = "The resource '" + std::string(target) + "' was not found.";
                res.prepare_payload();
                return res;
            };

        // Returns a server error response
        auto const server_error =
            [&req](beast::string_view what)
            {
                http::response<http::string_body> res{http::status::internal_server_error, req.version()};
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::content_type, "text/html");
                res.keep_alive(req.keep_alive());
                res.body() = "An error occurred: '" + std::string(what) + "'";
                res.prepare_payload();
                return res;
            };

        ////
        // Handle API calls.
        ////
        if (req.target().compare("/api/v1/cards") == 0) {
            if (req.method() == http::verb::post) {
                json card_json = json::parse(req.body());       // TODO error checking
                int id = m_db.create_card(card_json);
                json body_json = {{"id", id}};
                std::string body = body_json.dump();
                http::response<http::string_body> res(
                    http::status::ok,
                    req.version(),
                    body);
                //( std::piecewise_construct,
                //        std::make_tuple(std::move(body)),
                //        std::make_tuple(http::status::ok, req.version()));
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::content_type, "application/json");
                res.content_length(body.size());
                res.keep_alive(req.keep_alive());
                return send(std::move(res));
            } else if (req.method() == http::verb::get) {
             //   json::array json_cards = m_db.get_cards();
             //   http::response<http::file_body> res{
             //       std::piecewise_construct,
             //           std::make_tuple(std::move(body)),
             //           std::make_tuple(http::status::ok, req.version())};
             //   res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
             //   res.set(http::field::content_type, mime_type(path));
             //   res.content_length(size);
             //   res.keep_alive(req.keep_alive());
             //   return send(std::move(res));
            } else {
                return send(bad_request("Invalid HTTP-method"));
            }
        }
        // More API calls here ...

        ////
        // Handle static files.
        ////
        // Make sure we can handle the method
        if (req.method() != http::verb::get &&
            req.method() != http::verb::head)
            return send(bad_request("Unknown HTTP-method"));

        // Request path must be absolute and not contain "..".
        if (req.target().empty() ||
            req.target()[0] != '/' ||
            req.target().find("..") != beast::string_view::npos)
            return send(bad_request("Illegal request-target"));

        // Build the path to the requested file
        std::string path = path_cat(m_doc_root, req.target());
        if (req.target().back() == '/')
            path.append("index.html");

        // Attempt to open the file
        beast::error_code ec;
        http::file_body::value_type body;
        body.open(path.c_str(), beast::file_mode::scan, ec);

        // Handle the case where the file doesn't exist
        if (ec == beast::errc::no_such_file_or_directory)
            return send(not_found(req.target()));

        // Handle an unknown error
        if (ec)
            return send(server_error(ec.message()));

        // Cache the size since we need it after the move
        auto const size = body.size();

        // Respond to HEAD request
        if (req.method() == http::verb::head) {
            http::response<http::empty_body> res{http::status::ok, req.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, mime_type(path));
            res.content_length(size);
            res.keep_alive(req.keep_alive());
            return send(std::move(res));
        }

        // Respond to GET request
        http::response<http::file_body> res{
            std::piecewise_construct,
                std::make_tuple(std::move(body)),
                std::make_tuple(http::status::ok, req.version())};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return send(std::move(res));
    }

    SQLiteDatabase m_db;
    net::io_context m_ioctx;
    tcp::acceptor m_acceptor;
    std::string m_doc_root;
};


//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    try {
        // Check command line arguments.
        if (argc == 2 && std::string(argv[1]) == "create_database") {
            SQLiteDatabase db{"build/nerdbase.db"};
            if (!db.init())
                throw std::runtime_error("initializing database failed");
            return 0;
        } else if (argc == 4) {
            auto const address = net::ip::make_address(argv[1]);
            auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
            auto const doc_root = std::string(argv[3]);
            SQLiteDatabase db{"build/nerdbase.db"};

            HttpServer server(std::move(db), address, port, std::move(doc_root));
            server.run();
            return 0;
        } else  {
            std::cerr <<
                "Usage: nerd (create_database | <address> <port> <doc_root>)\n" <<
                "Example:\n" <<
                "    nerd 0.0.0.0 8080 .\n";
            return EXIT_FAILURE;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
