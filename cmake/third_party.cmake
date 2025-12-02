include(cmake/CPM.cmake)

CPMFindPackage(
    NAME GTest
    GITHUB_REPOSITORY google/googletest
    VERSION 1.17.0
    OPTIONS "BUILD_GMOCK OFF"
)

CPMAddPackage(
    NAME loguru
    GITHUB_REPOSITORY emilk/loguru
    GIT_TAG master
)

CPMAddPackage(
    NAME cxxopts
    GITHUB_REPOSITORY jarro2783/cxxopts
    GIT_TAG v3.3.1
)

