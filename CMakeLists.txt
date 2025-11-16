cmake_minimum_required(VERSION 3.10)
project(frblock VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(EXECUTABLE_NAME frblock)
include(CTest)
include(FetchContent)

option(SKIP_TEST_FETCH "Skip fetching test dependencies (use system GTest or disable tests)" ON)

add_executable(${EXECUTABLE_NAME}
    src/main.cpp
    src/config.cpp
    src/updater.cpp
)

# install target (optional)
install(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION bin)

if (BUILD_TESTING)
    if (SKIP_TEST_FETCH)
        find_package(GTest REQUIRED)
    else()
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.17.0
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()

    add_executable(replace_code_tests
        tests/updater_tests.cpp
        src/updater.cpp
        src/config.cpp
    )
    target_include_directories(replace_code_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(replace_code_tests PRIVATE GTest::gtest_main)

    add_test(NAME replace_code_tests COMMAND replace_code_tests)
endif()
