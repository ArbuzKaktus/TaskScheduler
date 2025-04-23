include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
    scheduler-lib-tests
    class_methods_tests.cpp
    dependence_tests.cpp
    lambda_tests.cpp
    special_cases.cpp
)

target_link_libraries(
    scheduler-lib-tests
    GTest::gtest_main
    GTest::gmock_main
    scheduler_lib
)

target_include_directories(scheduler-lib-tests PUBLIC ${PROJECT_SOURCE_DIR})

include(GoogleTest)

gtest_discover_tests(scheduler-lib-tests)