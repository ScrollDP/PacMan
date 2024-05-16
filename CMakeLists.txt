cmake_minimum_required(VERSION 3.5)

project(PacMan)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Widgets REQUIRED)

add_executable(PacMan
        main.cpp
)

target_link_libraries(PacMan PRIVATE
        Qt6::Core
        Qt6::Widgets
)
