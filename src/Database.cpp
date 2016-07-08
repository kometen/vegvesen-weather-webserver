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

    std::string query = "select doc from readings_json where site_id = $1 order by id desc limit 1";
    (*D).prepare(prepared, query);

    pqxx::nontransaction N(*D);
    pqxx::result R(N.prepared(prepared)(site_id).exec());

    for (pqxx::result::iterator c = R.begin(); c != R.end(); ++c) {
        result = c[0].as<std::string>();
    }

    // Put the connection back to the pool.
    {
        auto lock = lock_db_mutex();
        dbpool.push(D);
    }

    return result;
}

const std::string Database::graticule(const std::string latitude, const std::string longitude, const std::string sites) {
    const double miles_to_km = 1.609344;
    std::string km {};
    const std::string prepared_b = "prepared_b";
    const std::string prepared_c = "prepared_c";
    std::string result {};
    pqxx::connection *D1;
    pqxx::connection *D2;

    // Get connections from pool. If empty add connections.
    {
        auto lock = lock_db_mutex();
        if (dbpool.empty()) {
            increase_pool(5);
        }
        D1 = dbpool.top();
        dbpool.pop();
        D2 = dbpool.top();
        dbpool.pop();
    }

    // Get nearest sites.
    std::string query = "select site_id, round((coordinate <@> point($1,$2))::numeric, 6) as miles from readings_json group by site_id, miles order by miles limit $3";
    (*D1).prepare(prepared_b, query);

    pqxx::nontransaction N1(*D1);
    pqxx::result R1(N1.prepared(prepared_b)(longitude)(latitude)(sites).exec());

    for (pqxx::result::iterator c1 = R1.begin(); c1 != R1.end(); ++c1) {
        // Get detailed information for each site_id, pack it into json-format.
        query = "select doc from readings_json where site_id = $1 order by id desc limit 1";
        (*D2).prepare(prepared_c, query);

        pqxx::nontransaction N2(*D2);
        pqxx::result R2(N2.prepared(prepared_c)(c1[0].as<std::string>()).exec());

        for (pqxx::result::iterator c2 = R2.begin(); c2 != R2.end(); ++c2) {
            if (result.size()) {
                result += ", ";
            } else {
                result = "[";
            }
            km = std::to_string(c1[1].as<double>() * miles_to_km);
            result += "{\"statute miles\": \"" + c1[1].as<std::string>() + "\", \"kilometers\": \"" + km + "\", " + c2[0].as<std::string>().erase(0,1);
        }
    }
    result += "]";

    // Put the connections back to the pool.
    {
        auto lock = lock_db_mutex();
        dbpool.push(D1);
        dbpool.push(D2);
    }

    return result;
}

const std::string Database::erento(const std::string latitude, const std::string longitude, const std::string radius) {
    const double miles_to_km = 1.609344;
    double miles = std::stod(radius) / miles_to_km;
    std::string km {};
    const std::string prepared_bb = "prepared_bb";
    const std::string prepared_cc = "prepared_cc";
    std::string result {};
    pqxx::connection *D11;
    pqxx::connection *D12;

    // Get connections from pool. If empty add connections.
    {
        auto lock = lock_db_mutex();
        if (dbpool.empty()) {
            increase_pool(5);
        }
        D11 = dbpool.top();
        dbpool.pop();
        D12 = dbpool.top();
        dbpool.pop();
    }

    // Get nearest sites.
    std::string query = "select site_id, round((coordinate <@> point($1,$2))::numeric, 6) as miles from readings_json where (point($3,$4) <@> coordinate) < $5 group by site_id, miles order by miles";
    (*D11).prepare(prepared_bb, query);

    pqxx::nontransaction N11(*D11);
    pqxx::result R11(N11.prepared(prepared_bb)(longitude)(latitude)(longitude)(latitude)(miles).exec());

    for (pqxx::result::iterator c11 = R11.begin(); c11 != R11.end(); ++c11) {
        // Get detailed information for each site_id, pack it into json-format.
        query = "select doc from readings_json where site_id = $1 order by id desc limit 1";
        (*D12).prepare(prepared_cc, query);

        pqxx::nontransaction N12(*D12);
        pqxx::result R12(N12.prepared(prepared_cc)(c11[0].as<std::string>()).exec());

        for (pqxx::result::iterator c12 = R12.begin(); c12 != R12.end(); ++c12) {
            if (result.size()) {
                result += ", ";
            } else {
                result = "[";
            }
            km = std::to_string(c11[1].as<double>() * miles_to_km);
            result += "{\"statute miles\": \"" + c11[1].as<std::string>() + "\", \"kilometers\": \"" + km + "\", " + c12[0].as<std::string>().erase(0,1);
        }
    }
    result += "]";

    // Put the connections back to the pool.
    {
        auto lock = lock_db_mutex();
        dbpool.push(D11);
        dbpool.push(D12);
    }

    return result;
}

Database::~Database() {
}

} /* namespace gnome */
