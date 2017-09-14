cmake_minimum_required(VERSION 2.8)
project( parallel )
find_package( OpenCV REQUIRED )
include_directories("./../fastflow")
add_definitions(-std=c++11)
add_executable( parallel parallel.cpp )
target_link_libraries( parallel ${OpenCV_LIBS} )


