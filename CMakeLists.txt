cmake_minimum_required (VERSION 2.6)

project( subtool )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")

include_directories(include)
add_executable( subtool src/main.cpp include/sub.h src/sub.cpp )

