cmake_minimum_required(VERSION 3.18)
project(GravitySim)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SFML_STATIC_LIBRARIES FALSE)
find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)
add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SFML::Graphics 
    SFML::Window 
    SFML::System
)