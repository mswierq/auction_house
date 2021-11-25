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
set(lib_src
        src/user_account.cpp
        src/tasks_queue.cpp
        src/auctions.cpp
        src/session.cpp
        src/tasks.cpp
        src/command.cpp
        src/auction_processor.cpp)
add_library(lib_auction_engine ${lib_src})
target_include_directories(lib_auction_engine PUBLIC include)

add_executable(auction_house main.cpp)
target_include_directories(auction_house PUBLIC include)
target_link_libraries(auction_house lib_auction_engine spdlog::spdlog)

## TESTS
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v2.13.7
)

FetchContent_MakeAvailable(Catch2)

set(tests_src
        tests/tests.cpp
        tests/test_accounts.cpp
        tests/test_tasks_queue.cpp
        tests/test_auctions.cpp
        tests/test_session.cpp
        tests/test_commands.cpp
        tests/test_auction_processor.cpp)
add_executable(tests ${tests_src})
target_link_libraries(tests PRIVATE Catch2::Catch2)
target_include_directories(tests PRIVATE include)
target_link_libraries(tests PRIVATE lib_auction_engine pthread)