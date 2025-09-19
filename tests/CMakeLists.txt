cmake_minimum_required(VERSION 3.10)

project(AppGatewayTests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Test sources
set(TEST_SOURCES
    test_appgateway.cpp
    test_appgateway_events.cpp
)

add_executable(appgateway_tests ${TEST_SOURCES})

# Include paths for plugin headers
target_include_directories(appgateway_tests PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/..
    ${CMAKE_CURRENT_LIST_DIR}/../AppGateway
    ${CMAKE_CURRENT_LIST_DIR}/../LaunchDelegate
    ${CMAKE_CURRENT_LIST_DIR}/../App2AppProvider
    ${CMAKE_CURRENT_LIST_DIR}/../AppNotifications
)

# No external Thunder libraries are linked; we use header-only and stub interfaces.
# If Thunder libs are available in CI, they can be linked here.

# Enable test with CTest if parent integrates
enable_testing()
add_test(NAME appgateway_tests COMMAND appgateway_tests)
