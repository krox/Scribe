cmake_minimum_required(VERSION 3.10)
project(Example_Project)

# Step 2: Define the generated header file and the command to produce it
set(GENERATED_HEADER "${CMAKE_BINARY_DIR}/mystruct.h")

add_custom_command(
    OUTPUT ${GENERATED_HEADER}
    COMMAND scribe codegen --schema ${CMAKE_SOURCE_DIR}/tests/example_project/mystruct.json > ${GENERATED_HEADER}

    DEPENDS scribe ${CMAKE_SOURCE_DIR}/tests/example_project/mystruct.json
    COMMENT "Generating the header file using 'scribe codegen'"
)

# Step 3: Add the main program and specify dependencies
add_executable(example example.cpp)

# Ensure the main program depends on the generated header file
add_dependencies(example scribe)
target_include_directories(example PRIVATE ${CMAKE_BINARY_DIR})
target_sources(example PRIVATE ${GENERATED_HEADER})
target_link_libraries(example PRIVATE libscribe)

# Include the binary directory so main_program can locate the generated header
