cmake_minimum_required(VERSION 3.10)
project(SideScroller)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add raylib as a subdirectory
add_subdirectory(raylib)

# Create the executable
add_executable(${PROJECT_NAME} main.cpp)

# Link raylib to our project
target_link_libraries(${PROJECT_NAME} raylib) 