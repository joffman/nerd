#ifndef NERD_HTTP_SERVER_H
#define NERD_HTTP_SERVER_H

#include <string>

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include "names.h"
#include "sqlite_database.h"


namespace nerd {

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

    void run();

private:

    //////////////////////////////
    // Static functions.
    //////////////////////////////

    template<class Body, class Allocator>
    http::response<http::string_body> build_json_response(
        const http::request<Body, http::basic_fields<Allocator>>& req,
        const json& response_json);

    // Return a reasonable mime type based on the extension of a file.
    static beast::string_view mime_type(beast::string_view path);

    // Report a failure
    static void fail(beast::error_code ec, char const* what);
    
    // Append an HTTP rel-path to a local filesystem path.
    // The returned path is normalized for the platform.
    static std::string path_cat(
        beast::string_view base,
        beast::string_view path);


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
    void do_session(tcp::socket& socket);

    // This function produces an HTTP response for the given
    // request. The type of the response object depends on the
    // contents of the request, so the interface requires the
    // caller to pass a generic lambda for receiving the response.
    template<class Body, class Allocator, class Send>
    void handle_request(
        //http::request<Body, http::basic_fields<Allocator>>&& req, // because of gdb bug
        http::request<Body, http::basic_fields<Allocator>> req,
        Send&& send);

private:
    SQLiteDatabase m_db;
    net::io_context m_ioctx;
    tcp::acceptor m_acceptor;
    std::string m_doc_root;
};

}   // nerd

#endif  // NERD_HTTP_SERVER_H
