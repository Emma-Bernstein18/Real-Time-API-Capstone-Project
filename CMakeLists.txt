cmake_minimum_required(VERSION 4.1)
project(Capstone)

set(CMAKE_CXX_STANDARD 17)

add_executable(Capstone main.cpp)
find_package(CURL REQUIRED)
target_link_libraries(Capstone CURL::libcurl)

