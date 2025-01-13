cmake_minimum_required(VERSION 3.10)
project(HTTP-SERVER)

# Set the C and C++ standards
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Set the include directory for header files
include_directories(${CMAKE_SOURCE_DIR}/include())

# Add the server target
add_executable(server src/server.c)

# Add the client target
add_executable(client src/client.c)
