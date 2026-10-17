cmake_minimum_required(VERSION 3.16)

project(snow_simul LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SFML 3 REQUIRED COMPONENTS Graphics)

add_executable(snow_simul src/main.cpp)
target_link_libraries(snow_simul PRIVATE SFML::Graphics)
