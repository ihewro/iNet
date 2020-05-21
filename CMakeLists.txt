cmake_minimum_required(VERSION 3.16)
project(iNet)

set(CMAKE_CXX_STANDARD 14)


add_executable(${PROJECT_NAME}
        Boot.cpp
        src/Server.cpp
        src/Client.cpp
        utils/cmdParser.cpp
        src/UdpConnection.cpp)


target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        )