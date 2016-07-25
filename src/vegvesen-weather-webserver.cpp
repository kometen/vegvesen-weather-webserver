//============================================================================
// Name        : vegvesen-weather-webserver.cpp
// Author      : Claus Guttesen
// Version     : Webserver borrowed from https://github.com/eidheim/Simple-Web-Server/blob/master/http_examples.cpp
// Copyright   : Just let me know.
// Description : Hello World in C++, Ansi-style
//============================================================================

// http://tapoueh.org/blog/2013/08/05-earthdistance, install extensions as superuser
// OSX:
// PGUSER=postgres psql weather
// FreeBSD:
// PGUSER=pgsql psql weather
//
// create extension cube;
// create extension earthdistance;

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
    HttpServer server{8080, 8};
    gnome::Database database {25};

    // GET site id. Respond with weather statistics.
    server.resource["^/site/([0-9]{1,9})$"]["GET"] = [&database](HttpServer::Response& response, std::shared_ptr<HttpServer::Request> request) {
        const std::string nf = "{\"Status 404\": \"Not Found\"}";
        const int nfs = nf.size();
        std::string id = request->path_match[1];
        std::string result = database.site(id);
        if (result.size()) {
            response << "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: " << result.size() << "\r\n\r\n" << result;
        } else {
            response << "HTTP/1.1 404 Not Found\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: " << nfs << "\r\n\r\n" << nf;
        }
    };

    // Search for location nearby. Return five nearest sites as default.
    server.resource["^/graticule/([0-9]{1,3}.[0-9]{1,8})/([0-9]{1,3}.[0-9]{1,8})$"]["GET"] = [&database](HttpServer::Response& response, std::shared_ptr<HttpServer::Request> request) {
        const std::string nf = "{\"Status 404\": \"Not Found\"}";
        const int nfs = nf.size();
        std::string latitude = request->path_match[1];
        std::string longitude = request->path_match[2];
        std::string result = database.graticule(latitude, longitude, "5");
        if (result.size()) {
            response << "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: " << result.size() << "\r\n\r\n" << result;
        } else {
            response << "HTTP/1.1 404 Not Found\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: " << nfs << "\r\n\r\n" << nf;
        }
    };

    // Search for location nearby. Return up to nine nearest sites as default.
    server.resource["^/graticule/([0-9]{1,3}.[0-9]{1,8})/([0-9]{1,3}.[0-9]{1,8})/([1-9]{1,1})$"]["GET"] = [&database](HttpServer::Response& response, std::shared_ptr<HttpServer::Request> request) {
        const std::string nf = "{\"Status 404\": \"Not Found\"}";
        const int nfs = nf.size();
        std::string latitude = request->path_match[1];
        std::string longitude = request->path_match[2];
        std::string sites = request->path_match[3];
        std::string result = database.graticule(latitude, longitude, sites);
        if (result.size()) {
            response << "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: " << result.size() << "\r\n\r\n" << result;
        } else {
            response << "HTTP/1.1 404 Not Found\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: " << nfs << "\r\n\r\n" << nf;
        }
    };

    // Search for location within a range.
    // datachomp.com/archives/radius-queries-in-postgres/
    server.resource["^/erento/([0-9]{1,3}.[0-9]{1,8})/([0-9]{1,3}.[0-9]{1,8})/([0-9]{1,2})$"]["GET"] = [&database](HttpServer::Response& response, std::shared_ptr<HttpServer::Request> request) {
        const std::string nf = "{\"Status 404\": \"Not Found\"}";
        const int nfs = nf.size();
        std::string latitude = request->path_match[1];
        std::string longitude = request->path_match[2];
        std::string radius = request->path_match[3];
        std::string result = database.erento(latitude, longitude, radius);
        if (result.size()) {
            response << "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: " << result.size() << "\r\n\r\n" << result;
        } else {
            response << "HTTP/1.1 404 Not Found\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: " << nfs << "\r\n\r\n" << nf;
        }
    };

    // Info about client connecting.
    server.resource["^/info$"]["GET"] = [](HttpServer::Response& response, std::shared_ptr<HttpServer::Request> request) {
        std::stringstream cs;
        cs << "<!doctype html><html lang='en'><head><meta charset='utf-8'><title>Weather statistics</title></head><body>";
        cs << "<h1>Request from " << request->remote_endpoint_address << " (" << request->remote_endpoint_port << ")</h1>";
        cs << request->method << " " << request->path << " HTTP/" << request->http_version << "<br/>";
        for (auto& h : request->header) {
            cs << h.first << ": " << h.second << "<br/>";
        }
        cs << "</body></html>";

        // Find length of cs (content stream).
        cs.seekp(0, std::ios::end);

        response << "HTTP/1.1 200 OK\r\nContent-Length: " << cs.tellp() << "\r\n\r\n" << cs.rdbuf();
    };

    // Default response.
    server.default_resource["GET"] = [](HttpServer::Response& response, std::shared_ptr<HttpServer::Request> request) {
        const std::string msg = "<!doctype html><html lang='en'><head><meta charset='utf-8'><title>Weather statistics</title></head><body>Available paths<br/><a href='/info'>/info</a><br/><a href='/site/123'>/site/id</a>, id is numeric<br/><a href='/graticule/61.737785/6.4092124'>/graticule/latitude/longitude</a>, in decimal degrees<br/><a href='/graticule/61.737785/6.4092124/9'>/graticule/latitude/longitude/sites</a>, up to nine sites<br/><a href='/erento/61.737785/6.4092124/30'>/erento/latitude/longitude/radius</a>, radius in kilometer</body></html>";
        const int msgs = msg.size();
        response << "HTTP/1.1 404 Not Found\r\nContent-Length: " << msgs << "\r\n\r\n" << msg;
    };

    std::thread server_thread([&server]() {
       server.start();
    });

    std::this_thread::sleep_for(std::chrono::seconds(1));
    server_thread.join();

	return 0;
}
