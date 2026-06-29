cmake_minimum_required(VERSION 3.15)

# Define the project name
project(strategy VERSION 0.0.0 LANGUAGES CXX)

# Generate compilation commands for IDE indexers
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set the C++ standard to C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Define source files
file(GLOB SOURCES src/*.cpp)

# Create the executable and link it to OpenCV libraries
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    include
    "${CMAKE_CURRENT_SOURCE_DIR}/../common/include/"
)
