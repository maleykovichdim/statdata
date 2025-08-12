cmake_minimum_required(VERSION 3.10)
project(statdata_processor C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Main application
add_executable(statdata_processor 
    src/main.c 
    src/statdata.c)

# Test executable
add_executable(statdata_tests
    tests/all_tests.c
    src/statdata.c)
target_include_directories(statdata_tests PRIVATE src)

# Copy executables to build root
add_custom_command(TARGET statdata_processor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:statdata_processor> ${CMAKE_BINARY_DIR}
    COMMENT "Copying main executable to build directory"
)

add_custom_command(TARGET statdata_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:statdata_tests> ${CMAKE_BINARY_DIR}
    COMMENT "Copying test executable to build directory"
)

# Test configuration
enable_testing()
add_test(NAME statdata_tests COMMAND statdata_tests WORKING_DIRECTORY ${CMAKE_BINARY_DIR})