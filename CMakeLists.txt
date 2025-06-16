cmake_minimum_required(VERSION 3.31)
project(fermity LANGUAGES C)

# compile lastest c
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
# Werror
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(COMPILE_WARNING_AS_ERROR ON)

message(STATUS "c flags ${CMAKE_C_FLAGS} build type ${CMAKE_BUILD_TYPE}")

# external 
find_package(OpenSSL REQUIRED)


if (CMAKE_SYSTEM_NAME STREQUAL "Linux" OR
    CMAKE_SYSTEM_NAME STREQUAL "Android")
    message(STATUS "Supported ${CMAKE_SYSTEM_NAME} system")
else()
    message(FATAL_ERROR "Unsupported ${CMAKE_SYSTEM_NAME} system")
endif()

# sets main project
add_executable(${PROJECT_NAME} 
  main/src/main.c
  main/src/util.c
)
target_include_directories(${PROJECT_NAME} PRIVATE main/include)
target_link_libraries(${PROJECT_NAME} OpenSSL::SSL)

# sets tools project
add_executable(${PROJECT_NAME}_tools
  tools/src/main.c
)
target_include_directories(${PROJECT_NAME}_tools PRIVATE)
target_link_libraries(${PROJECT_NAME}_tools)

# sets test project
add_executable(${PROJECT_NAME}_test
  main/src/util.c
  
  test/src/main.c
  test/src/util_test.c
)
target_include_directories(${PROJECT_NAME}_test PRIVATE main/include)
target_link_libraries(${PROJECT_NAME}_test)
