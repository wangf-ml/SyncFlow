cmake_minimum_required(VERSION 3.15)
project(SyncFlow VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/modules)

add_library(syncflow_core 
    src/ring_packet_pool.cpp 
    src/channel.cpp
)

enable_testing()

add_subdirectory(test)