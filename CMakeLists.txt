cmake_minimum_required(VERSION 3.20)
project(FuturesQueue1)

set(CMAKE_CXX_STANDARD 11)

add_executable(FuturesQueue1 main.cpp)
target_link_libraries(FuturesQueue1 -lpthread)
