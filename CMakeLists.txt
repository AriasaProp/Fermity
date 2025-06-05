cmake_minimum_required(VERSION 3.31)
project(fermity)

# compile flags
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

# external 
find_package(OpenSSL REQUIRED)


if (CMAKE_SYSTEM_NAME STREQUAL "Linux" OR
    CMAKE_SYSTEM_NAME STREQUAL "Android")
    message(STATUS "Supported ${CMAKE_SYSTEM_NAME} system")
else()
    message(FATAL_ERROR "Unsupported ${CMAKE_SYSTEM_NAME} system")
endif()

file(GLOB_RECURSE Main_Src "main/src/*.c")

# sets main project
add_executable(${PROJECT_NAME} ${Main_Src})
target_include_directories(${PROJECT_NAME} PRIVATE main/include)
target_link_libraries(
  ${PROJECT_NAME}
  OpenSSL::SSL
)

# sets test project
