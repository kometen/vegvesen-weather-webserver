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

const std::string Database::get_site(const std::string _id) {
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

    std::string query = "select description, coordinate, doc from readings_json inner join locations on readings_json.site_id = locations.site_id where readings_json.site_id = $1 order by measurementtimedefault desc limit 1";
    (*D).prepare(prepared, query);

    pqxx::nontransaction N(*D);
    pqxx::result R(N.prepared(prepared)(_id).exec());

    for (pqxx::result::iterator c = R.begin(); c != R.end(); ++c) {
        result = c[0].as<std::string>() + "|" + c[1].as<std::string>() + "|" + c[2].as<std::string>();
    }

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
