cmake_minimum_required(VERSION 3.10)

# Define the project name
project(vis_comp)

# Generate compilation commands for IDE indexers
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set the C++ standard to C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Add compile options for GCC/Clang
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find required packages
find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui calib3d)
# find_package(PahoMqttCpp REQUIRED)

# Define source files
file(GLOB SOURCES
    src/main.cpp
    src/Team.cpp
    src/Color.cpp
    src/Ball.cpp
    src/Video.cpp
    src/Calibration.cpp
    # src/Publisher.cpp
)

# Create the executable and link it to OpenCV libraries
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    include
    ${OpenCV_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${OpenCV_LIBS}
    # PahoMqttCpp::paho-mqttpp3
)

# Set preprocessor `MY_DEBUG` definition if build type is `Debug`
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE MY_DEBUG)
endif()
