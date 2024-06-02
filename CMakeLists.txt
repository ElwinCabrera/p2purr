cmake_minimum_required(VERSION 3.11)
# https://cmake.org/cmake/help/book/mastering-cmake/chapter/System%20Inspection.html
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckSymbolExists)
#include(FetchContent)


project(p2purr_chat 
        VERSION 1.0
        DESCRIPTION ""
        LANGUAGES CXX)

#include(CTest)
#include(Catch)




if(CMAKE_HOST_SYSTEM MATCHES Linux)
   message(STATUS "Build host runs Linux")
endif()

if(CMAKE_HOST_SYSTEM MATCHES Windows)
    message(STATUS "Build host runs Windows")
    if(CMAKE_BUILD_SYSTEM MATCHES CYGWIN) #Cygwin environment for Windows
        message(STATUS "Target build system is ${CMAKE_BUILD_SYSTEM}")
    endif()
    if(CMAKE_BUILD_SYSTEM MATCHES Midipix) # POSIX-compatible layer for Windows
        message(STATUS "Target build system is ${CMAKE_BUILD_SYSTEM}")
    endif()

    # Check for header files
    set(WIN_INCLUDES "")
    check_include_files("${WIN_INCLUDES};winsock.h" HAVE_WINSOCK_H)

    if(HAVE_WINSOCK_H)
        set(WIN_INCLUDES ${WIN_INCLUDES} winsock.h)
    endif()

    check_include_files("${WIN_INCLUDES};io.h" HAVE_IO_H)
    if (HAVE_IO_H)
        set(WIN_INCLUDES ${WIN_INCLUDES} io.h)
    endif()

    # Check for all needed libraries
    set(WIN_LIBS "")
    check_library_exists("dl;${WIN_LIBS}" dlopen "" HAVE_LIBDL)
    if(HAVE_LIBDL)
        set(WIN_LIBS ${WIN_LIBS} dl)
    endif()

    check_library_exists("ucb;${WIN_LIBS}" gethostname "" HAVE_LIBUCB)
    if(HAVE_LIBUCB)
        set(WIN_LIBS ${WIN_LIBS} ucb)
    endif()
endif()

# Check for some functions that are used
check_symbol_exists(socket "${WIN_INCLUDES}" HAVE_SOCKET)
check_symbol_exists(poll "${WIN_INCLUDES}" HAVE_POLL)



option(DEBUG_BUILD "Build with extra debug print messages.")

if(DEBUG_BUILD)
  target_compile_definitions(mytarget PUBLIC DEBUG_BUILD)
endif()




# Add the libraries we found to the libraries to use when
# looking for symbols with the check_symbol_exists macro
# set(CMAKE_REQUIRED_LIBRARIES ${LIBS})






file(GLOB SOURCE_FILES "src/*.cpp")
set(SOURCE_FILES ${SOURCE_FILES} main.cpp)
file(GLOB TEST_FILES "tests/*.cpp" "src/*.cpp")

set(TEST_NAME ${PROJECT_NAME}_tests)
 
add_executable(${PROJECT_NAME} ${SOURCE_FILES})
add_executable(${TEST_NAME} ${TEST_FILES})


# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_compile_options(${PROJECT_NAME} PRIVATE -g -Wall -Wextra -Wpedantic)
target_compile_options(${TEST_NAME} PRIVATE -g -Wall -Wextra -Wpedantic -D_TEST)
#target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_BINARY_DIR}" )

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)