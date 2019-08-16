#include <iostream>
#include <regex>
#include <string>   // string, stoi
#include <thread>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <json.hpp>

#include "http_server.h"
#include "names.h"

namespace {

// Wrapper for std::regex_match for string_views.
bool sv_regex_match(
    const beast::string_view& sv,
    std::match_results<beast::string_view::const_iterator>& matches,
    const std::regex& r)
{
    return std::regex_match(sv.begin(), sv.end(), matches, r);
}

}

namespace nerd {

void HttpServer::run()
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


//////////////////////////////
// Static functions.
//////////////////////////////

template<class Body, class Allocator>
http::response<http::string_body> HttpServer::build_json_response(
    const http::request<Body, http::basic_fields<Allocator>>& req,
    const json& response_json)
{
    std::string body = response_json.dump();
    http::response<http::string_body> resp(
        http::status::ok,
        req.version(),
        body);
    resp.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    resp.set(http::field::content_type, "application/json");
    resp.content_length(body.size());
    resp.keep_alive(req.keep_alive());
    return std::move(resp);
}

beast::string_view HttpServer::mime_type(beast::string_view path)
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

void HttpServer::fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

// Append an HTTP rel-path to a local filesystem path.
// The returned path is normalized for the platform.
std::string HttpServer::path_cat(
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
// Non-static functions
//////////////////////////////

void HttpServer::do_session(tcp::socket& socket)
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

template<class Body, class Allocator, class Send>
void HttpServer::handle_request(
    http::request<Body, http::basic_fields<Allocator>> req,
    Send&& send)
{
    // Returns a bad request response
    auto const bad_request =
        [&req](beast::string_view why)
        {
            http::response<http::string_body> resp{http::status::bad_request, req.version()};
            resp.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            resp.set(http::field::content_type, "text/html");
            resp.keep_alive(req.keep_alive());
            resp.body() = std::string(why);
            resp.prepare_payload();
            return resp;
        };

    // Returns a not found response
    auto const not_found = [&req](beast::string_view target)
    {
        http::response<http::string_body> resp{http::status::not_found, req.version()};
        resp.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        resp.set(http::field::content_type, "text/html");
        resp.keep_alive(req.keep_alive());
        resp.body() = "The resource '" + std::string(target) + "' was not found.";
        resp.prepare_payload();
        return resp;
    };

    // Returns a server error response
    auto const server_error = [&req](beast::string_view what)
    {
        http::response<http::string_body> resp{http::status::internal_server_error, req.version()};
        resp.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        resp.set(http::field::content_type, "text/html");
        resp.keep_alive(req.keep_alive());
        resp.body() = "An error occurred: '" + std::string(what) + "'";
        resp.prepare_payload();
        return resp;
    };

    ////
    // Handle API calls.
    ////
    {   // Card API.
        const std::regex regex(R"(/api/v1/cards/(\d+))");
        std::match_results<beast::string_view::const_iterator> matches;
        // TODO Check Content-type field.
        if (sv_regex_match(req.target(), matches, regex)) {
            int card_id = std::stoi(matches[1]);
            if (req.method() == http::verb::get) {          // get card
                json card_json = m_db.get_card(card_id);
                auto resp = build_json_response(req, card_json);
                return send(std::move(resp));
            } else if (req.method() == http::verb::put) {   // update card
                json resp_json;
                try {
                    json req_json = json::parse(req.body());
                    m_db.update_card(card_id, req_json);
                    resp_json["success"] = true;
                } catch (const std::exception& e) {
                    resp_json["success"] = false;
                    resp_json["error_msg"] = e.what();
                }
                auto resp = build_json_response(req, resp_json);
                return send(std::move(resp));
            } else if (req.method() == http::verb::delete_) {   // delete card
                json resp_json;
                try {
                    m_db.delete_card(card_id);
                    resp_json["success"] = true;
                } catch (const std::exception& e) {
                    resp_json["success"] = false;
                    resp_json["error_msg"] = e.what();
                }
                auto resp = build_json_response(req, resp_json);
                return send(std::move(resp));   // TODO Create api_success and api_error functions
            } else {
                return send(bad_request("Invalid HTTP-method"));    // TODO send {error_msg: "..."}
            }
        } else if (req.target().compare("/api/v1/cards") == 0) {
            if (req.method() == http::verb::post) {
                json card_json = json::parse(req.body());       // TODO error checking
                int id = m_db.create_card(card_json);
                json id_json = {{"id", id}};
                auto resp = build_json_response(req, id_json);
                return send(std::move(resp));
            } else if (req.method() == http::verb::get) {
                json cards_json = {{"cards", m_db.get_cards()}};
                auto resp = build_json_response(req, cards_json);
                return send(std::move(resp));
            } else {
                return send(bad_request("Invalid HTTP-method"));
            }
        } 
    }
    {   // Topic API.
        const std::regex regex(R"(/api/v1/topics/(\d+))");
        std::match_results<beast::string_view::const_iterator> matches;
        // TODO Check Content-type field.
        if (sv_regex_match(req.target(), matches, regex)) {
            int topic_id = std::stoi(matches[1]);
            if (req.method() == http::verb::put) {   // update topic
                json resp_json;
                try {
                    json req_json = json::parse(req.body());
                    m_db.update_topic(topic_id, req_json);
                    resp_json["success"] = true;
                } catch (const std::exception& e) {
                    resp_json["success"] = false;
                    resp_json["error_msg"] = e.what();
                }
                auto resp = build_json_response(req, resp_json);
                return send(std::move(resp));
            } else if (req.method() == http::verb::delete_) {   // delete topic
                json resp_json;
                try {
                    m_db.delete_topic(topic_id);
                    resp_json["success"] = true;
                } catch (const std::exception& e) {
                    resp_json["success"] = false;
                    resp_json["error_msg"] = e.what();
                }
                auto resp = build_json_response(req, resp_json);
                return send(std::move(resp));   // TODO Create api_success and api_error functions
            } else {
                return send(bad_request("Invalid HTTP-method"));    // TODO send {error_msg: "..."}
            }
        } else if (req.target().compare("/api/v1/topics") == 0) {
            if (req.method() == http::verb::post) {
                json topic_json = json::parse(req.body());       // TODO error checking
                int id = m_db.create_topic(topic_json);
                json id_json = {{"id", id}};
                auto resp = build_json_response(req, id_json);
                return send(std::move(resp));
            } else if (req.method() == http::verb::get) {
                json topics_json = {{"topics", m_db.get_topics()}};
                auto resp = build_json_response(req, topics_json);
                return send(std::move(resp));
            } else {
                return send(bad_request("Invalid HTTP-method"));
            }
        } 
    }

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
        http::response<http::empty_body> resp{http::status::ok, req.version()};
        resp.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        resp.set(http::field::content_type, mime_type(path));
        resp.content_length(size);
        resp.keep_alive(req.keep_alive());
        return send(std::move(resp));
    }

    // Respond to GET request
    http::response<http::file_body> resp{
        std::piecewise_construct,
            std::make_tuple(std::move(body)),
            std::make_tuple(http::status::ok, req.version())};
    resp.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    resp.set(http::field::content_type, mime_type(path));
    resp.content_length(size);
    resp.keep_alive(req.keep_alive());
    return send(std::move(resp));
}

}   // nerd
