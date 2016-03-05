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
        auto lacking_db_conn = true;
        do {
            auto lock = lock_db_mutex();
            if (!dbpool.empty()) {
                D = dbpool.top();
                dbpool.pop();
                lacking_db_conn = false;
            } else {
                lock.release();
            }
        } while (lacking_db_conn);
    }

    std::string query = "select * from locations, readings where locations._id = $1 order by date limit 1";
    (*D).prepare(prepared, query);

    pqxx::nontransaction N(*D);
    pqxx::result R(N.prepared(prepared)(_id).exec());

    for (pqxx::result::iterator c = R.begin(); c != R.end(); ++c) {
        result = c[2].as<std::string>() + "|" + c[3].as<std::string>() + "|" + c[6].as<std::string>();
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
