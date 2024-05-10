cmake_minimum_required(VERSION 3.24)
project(P2Pconn VERSION 1.0)
 
# Our Project
set(SOURCE_FILES src/*.cpp)

    

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
 
add_executable(${PROJECT_NAME} ${SOURCE_FILES})