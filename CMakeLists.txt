project("cpp-generes")
cmake_minimum_required(VERSION 3.2)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /EHsc")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wold-style-cast -Wextra -Wundef -Wsign-compare -Wconversion -Wpointer-arith -Weffc++ -Wnon-virtual-dtor")
endif()

if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 11)
endif()

include_directories(src)
include_directories(third_party)

file(GLOB_RECURSE PROJECT_SOURCES "src/*.cpp")

set(SRC_LIST ${PROJECT_SOURCES})

add_executable(${PROJECT_NAME} ${SRC_LIST})
