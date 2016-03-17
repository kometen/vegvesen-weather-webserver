/*
 * Database.cpp
 *
 *  Created on: 03/03/2016
 *      Author: claus
 */

#include "Database.hpp"

namespace gnome {

Database::Database(const unsigned int poolsize) {
    increase_pool(poolsize);
}

// https://geidav.wordpress.com/2014/01/09/mutex-lock-guards-in-c11/
std::unique_lock<std::mutex> Database::lock_db_mutex() {
    std::unique_lock<std::mutex> lock(dbmutex);
    return lock;
}

void Database::increase_pool(const unsigned int poolsize) {
    for (unsigned int i = 0; i < poolsize; ++i) {
        try {
            auto* dbconn = new pqxx::connection(connectionString);
            dbpool.emplace(dbconn);
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }
}

const std::string Database::site(const std::string site_id) {
    const std::string prepared = "a";
    std::string result {};
    pqxx::connection *D;

    // Get connection from pool. If empty add connections.
    {
        auto lock = lock_db_mutex();
        if (dbpool.empty()) {
            increase_pool(5);
        }
        D = dbpool.top();
        dbpool.pop();
    }

    std::string query = "select description, coordinate[0] as longitude, coordinate[1] as latitude, doc from readings_json inner join locations on readings_json.site_id = locations.site_id where readings_json.site_id = $1 order by measurementtimedefault desc limit 1";
    (*D).prepare(prepared, query);

    pqxx::nontransaction N(*D);
    pqxx::result R(N.prepared(prepared)(site_id).exec());

    for (pqxx::result::iterator c = R.begin(); c != R.end(); ++c) {
        result = "{\"measurementSiteName\": \"" + c[0].as<std::string>() + "\", \"latitude\": \"" + c[2].as<std::string>() + "\", \"longitude\": \"" + c[1].as<std::string>() + "\", " + c[3].as<std::string>().erase(0,1);
    }

    // Put the connection back to the pool.
    {
        auto lock = lock_db_mutex();
        dbpool.push(D);
    }

    return result;
}

const std::string Database::graticule(const std::string latitude, const std::string longitude) {
    const double mile = 1.609344;
    std::string km {};
    const std::string prepared = "b";
    std::string result {};
    pqxx::connection *D;

    // Get connection from pool. If empty add connections.
    {
        auto lock = lock_db_mutex();
        if (dbpool.empty()) {
            increase_pool(5);
        }
        D = dbpool.top();
        dbpool.pop();
    }

    std::string query = "select miles, description, coordinate[0] as longitude, coordinate[1] as latitude, doc from (select l.miles, l.site_id, l.description, l.coordinate, doc from (select site_id, description, round((coordinate <@> point($1,$2))::numeric, 1) as miles, coordinate from locations order by miles limit 5) as l, readings_json where l.site_id = readings_json.site_id limit 5) as sub order by sub.miles";
    (*D).prepare(prepared, query);

    pqxx::nontransaction N(*D);
    pqxx::result R(N.prepared(prepared)(longitude)(latitude).exec());

    for (pqxx::result::iterator c = R.begin(); c != R.end(); ++c) {
        if (result.size()) {
            result += ", ";
        } else {
            result = "[";
        }
        km = std::to_string(c[0].as<double>() * mile);
        result += "{\"statute miles\": \"" + c[0].as<std::string>() + "\", \"kilometers\": \"" + km + "\", \"measurementSiteName\": \"" + c[1].as<std::string>() + "\", \"latitude\": \"" + c[3].as<std::string>() + "\", \"longitude\": \"" + c[2].as<std::string>() + "\", " + c[4].as<std::string>().erase(0,1);
    }
    result += "]";

    // Put the connection back to the pool.
    {
        auto lock = lock_db_mutex();
        dbpool.push(D);
    }

    return result;
}

Database::~Database() {
}

} /* namespace gnome */
