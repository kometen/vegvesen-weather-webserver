cmake_minimum_required(VERSION 3.3)
project(vegvesen-weather-webserver)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

    include_directories(/opt/local/include)
    link_directories(/opt/local/lib)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++11 -Wall -O2 -Werror -Wno-deprecated-register")

    include_directories(/usr/local/include)
    link_libraries(/usr/local/lib/libboost_system.so)
    link_libraries(/usr/local/lib/libboost_coroutine.so)
    link_libraries(/usr/local/lib/libboost_regex.so)
    link_libraries(/usr/local/lib/libpqxx.so)
endif()

set(SOURCE_FILES vegvesen-weather-webserver.cpp Database.cpp)
add_executable(vegvesen-weather-webserver ${SOURCE_FILES})
