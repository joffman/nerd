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

#include <cstdlib>      // EXIT_FAILURE
#include <exception>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <utility>      // move

#include "http_server.h"
#include "names.h"
#include "sqlite_database.h"

using namespace nerd;

int main(int argc, char* argv[])
{
    try {
        // Check command line arguments.
        if (argc == 2 && std::string(argv[1]) == "create_database") {
            SQLiteDatabase db{"nerdbase.db"};
            db.init();
            return 0;
        } else if (argc == 4) {
            auto const address = net::ip::make_address(argv[1]);
            auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
            auto const doc_root = std::string(argv[3]);
            SQLiteDatabase db{"nerdbase.db"};

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
