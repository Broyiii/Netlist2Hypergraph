cmake_minimum_required(VERSION 3.8)
project(nl2hg)

include_directories("./src/")

SET(DEBUG off)
IF(DEBUG)
    set(CMAKE_BUILD_TYPE "DEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -ggdb3 -fPIC")
ELSE()
    set(CMAKE_BUILD_TYPE "RELEASE")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -fPIC -fpermissive")
ENDIF()

set(SOURCES 
    ./src/main.cpp 
    ./src/parser.cpp
    ./src/acyclic.cpp
)

# set(CMAKE_EXE_LINKER_FLAGS "-static")

add_executable(${PROJECT_NAME} ${SOURCES})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_link_libraries(nl2hg  stdc++fs )