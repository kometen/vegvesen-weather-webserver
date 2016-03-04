//============================================================================
// Name        : vegvesen-weather-webserver.cpp
// Author      : Claus Guttesen
// Version     : Webserver borrowed from https://github.com/eidheim/Simple-Web-Server/blob/master/http_examples.cpp
// Copyright   : Just let me know.
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>

#include <eidheim/server_http.hpp>

// Default resource.
#include <fstream>
#include <boost/filesystem.hpp>

// JSON
#define BOOST_SPIRIT_THREADSAFE
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

// Database.
#include "Database.hpp"

typedef SimpleWeb::Server<SimpleWeb::HTTP> HttpServer;

int main() {
    HttpServer server(8080, 4);
    gnome::Database database {10};

    // GET site id. Respond with weather statistics.
    server.resource["^/site/([0-9]{1,9})$"]["GET"] = [&database](HttpServer::Response& response, std::shared_ptr<HttpServer::Request> request) {
        std::string id = request->path_match[1];
        std::string result = database.get_site(id);
        response << "HTTP/1.1 200 OK\r\nContent-Length: " << result.size() << "\r\n\r\n" << result;
    };

    // Info about client connecting.
    server.resource["^/info$"]["GET"] = [](HttpServer::Response& response, std::shared_ptr<HttpServer::Request> request) {
        std::stringstream cs;
        cs << "<h1>Request from " << request->remote_endpoint_address << " (" << request->remote_endpoint_port << ")</h1>";
        cs << request->method << " " << request->path << " HTTP/" << request->http_version << "<br/>";
        for (auto& h : request->header) {
            cs << h.first << ": " << h.second << "<br/>";
        }

        // Find length of cs (content stream).
        cs.seekp(0, std::ios::end);

        response << "HTTP/1.1 200 OK\r\nContent-Length: " << cs.tellp() << "\r\n\r\n" << cs.rdbuf();
    };

    // Default response.
    server.default_resource["GET"] = [](HttpServer::Response& response, std::shared_ptr<HttpServer::Request> request) {
        std::string msg = "<!doctype html><html lang='en'><head><meta charset='utf-8'><title>Weather statistics</title></head>";
        msg += "<body>Available paths are <a href='/info'>/info</a> and <a href='/site/123'>/site/id</a> where id is numeric</body></html>";
        response << "HTTP/1.1 200 OK\r\nContent-Length: " << msg.size() << "\r\n\r\n" << msg;
    };

    std::thread server_thread([&server]() {
       server.start();
    });

    std::this_thread::sleep_for(std::chrono::seconds(1));
    server_thread.join();

    std::cout << "Blåbærsyltetøj" << std::endl;
	return 0;
}
