cmake_minimum_required(VERSION 3.16)
project(fairtrade)

# Use a consistent C++ standard for subprojects
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Delegate to the trading engine subdirectory
add_subdirectory(core/trading-engine)
