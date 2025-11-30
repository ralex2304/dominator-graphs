cmake_minimum_required(VERSION 3.16)

project(graphs LANGUAGES CXX)

enable_testing()

include(cmake/tools.cmake)

include(cmake/defaults.cmake)

include(cmake/third_party.cmake)

add_executable(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/source/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/graph.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)

target_link_libraries(${PROJECT_NAME} PRIVATE graphs-defaults loguru::loguru cxxopts::cxxopts)

