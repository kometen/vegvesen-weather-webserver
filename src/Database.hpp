/*
 * Database.hpp
 *
 *  Created on: 03/03/2016
 *      Author: claus
 */

#ifndef DATABASE_HPP_
#define DATABASE_HPP_

#include <iostream>
#include <mutex>
#include <stack>
#include <thread>
#include <pqxx/pqxx>

namespace gnome {

class Database {
private:
    const std::string connectionString = "dbname=weather user=claus hostaddr=127.0.0.1 port=5432";
    std::stack<pqxx::connection*> dbpool;
    std::mutex dbmutex;
    std::unique_lock<std::mutex> lock_db_mutex();
    void increase_pool(const unsigned int);

public:
    Database(const unsigned int);
    const std::string get_site(const std::string);
    virtual ~Database();
};

} /* namespace gnome */

#endif /* DATABASE_HPP_ */
