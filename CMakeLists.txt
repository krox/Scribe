cmake_minimum_required(VERSION 3.16.0)
project(scribe LANGUAGES CXX)

# dependencies
include(cmake/CPM.cmake)
CPMAddPackage("gh:fmtlib/fmt#10.1.1")
CPMAddPackage("gh:CLIUtils/CLI11@2.4.1")
CPMAddPackage("gh:nlohmann/json@3.11.3")

# main library
add_library(libscribe src/codegen.cpp src/read_json.cpp src/schema.cpp src/tome.cpp src/validate_json.cpp)
target_compile_features(libscribe PUBLIC cxx_std_20)
target_include_directories(libscribe PUBLIC include)
target_link_libraries(libscribe PUBLIC fmt::fmt nlohmann_json::nlohmann_json)
target_compile_options(libscribe PRIVATE -Wall -Wextra -Wno-deprecated-declarations -Wno-stringop-overflow -Werror -O2)

# executables
add_executable(scribe src/main.cpp)
target_compile_features(scribe PUBLIC cxx_std_20)
target_compile_options(scribe PRIVATE -Wall -Wextra -Wno-deprecated-declarations -Wno-stringop-overflow -Werror -O2)
target_link_libraries(scribe libscribe CLI11::CLI11 nlohmann_json::nlohmann_json)

# install
file(GLOB files_h "src/include/scribe/*.h")
install(TARGETS libscribe DESTINATION lib)
install(FILES ${files_h} DESTINATION include/scribe)

# unittests
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    CPMAddPackage("gh:catchorg/Catch2#v3.3.1")

    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)

    add_executable(tests tests/tome.cpp tests/codegen.cpp)
    target_compile_features(tests PRIVATE cxx_std_20)
    target_link_libraries(tests PRIVATE Catch2::Catch2WithMain libscribe)
    target_compile_options(tests PUBLIC -Wall -Wextra -Werror -g)
endif()
