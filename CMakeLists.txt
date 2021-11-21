cmake_minimum_required(VERSION 3.18)
project(auction_house)

set(CMAKE_CXX_STANDARD 17)

Include(FetchContent)

## LOGS
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG        v1.9.2
)

FetchContent_GetProperties(spdlog)
if (NOT spdlog_POPULATED)
    FetchContent_Populate(spdlog)
    add_subdirectory(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR})
endif ()

## SERVER APP
add_executable(auction_house main.cpp)
target_link_libraries(auction_house spdlog::spdlog)

## TESTS
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v2.13.7
)

FetchContent_MakeAvailable(Catch2)

add_executable(tests tests/tests.cpp)
target_link_libraries(tests PRIVATE Catch2::Catch2)