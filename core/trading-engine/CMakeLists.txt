cmake_minimum_required(VERSION 3.16)
project(trading_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(trading_engine STATIC ${SOURCES})
target_include_directories(trading_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(trading_engine PRIVATE -O3 -march=native -fno-exceptions -fno-rtti)

# Build a small demo executable that links against the static library
add_executable(trading_engine_demo ${CMAKE_CURRENT_SOURCE_DIR}/src/engine_main.cpp)
target_link_libraries(trading_engine_demo PRIVATE trading_engine)
target_compile_options(trading_engine_demo PRIVATE -O3 -march=native -fno-exceptions -fno-rtti)
