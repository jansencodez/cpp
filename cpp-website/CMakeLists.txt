cmake_minimum_required(VERSION 3.16)
project(SimpleHttpServer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)

# Create executable
add_executable(server src/main.cpp src/HttpServer.cpp src/LessonLoader.cpp)

# Link libraries
target_link_libraries(server 
    Threads::Threads
)

# Include directories
target_include_directories(server PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Set compiler flags
if(MSVC)
    target_compile_options(server PRIVATE /W4)
else()
    target_compile_options(server PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Copy configuration file to build directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.json ${CMAKE_CURRENT_BINARY_DIR}/config.json COPYONLY)
