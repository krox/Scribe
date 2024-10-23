cmake_minimum_required(VERSION 3.16.0)
project(scribe LANGUAGES CXX)

option(SCRIBE_WITH_HDF5 "Enable HDF5 support" ON)

# dependencies
include(cmake/CPM.cmake)
CPMAddPackage("gh:fmtlib/fmt#10.1.1")
CPMAddPackage("gh:CLIUtils/CLI11@2.4.1")
CPMAddPackage("gh:nlohmann/json@3.11.3")
CPMAddPackage("gh:xtensor-stack/xtl#0.7.7")
CPMAddPackage("gh:xtensor-stack/xtensor#0.25.0")

if(SCRIBE_WITH_HDF5)
    CPMAddPackage(
        NAME HighFive
        GITHUB_REPOSITORY BlueBrain/HighFive
        GIT_TAG v2.10.0
        OPTIONS
        "HIGHFIVE_USE_BOOST OFF"
    )
endif()

set(SCRIBE_WARNING_OPTIONS -Wall -Wextra -Werror)

# main library
add_library(libscribe src/codegen.cpp src/io_hdf5.cpp src/io_json.cpp src/schema.cpp src/tome.cpp)
target_compile_features(libscribe PUBLIC cxx_std_20)
target_include_directories(libscribe PUBLIC include)
target_link_libraries(libscribe PUBLIC fmt::fmt nlohmann_json::nlohmann_json xtensor)
target_compile_options(libscribe PRIVATE ${SCRIBE_WARNING_OPTIONS})

if(SCRIBE_WITH_HDF5)
    find_package(HDF5 REQUIRED)
    add_compile_definitions(SCRIBE_WITH_HDF5)
    target_link_libraries(libscribe PUBLIC HighFive)
endif()

# executables
add_executable(scribe src/main.cpp)
target_compile_features(scribe PUBLIC cxx_std_20)
target_compile_options(scribe PRIVATE ${SCRIBE_WARNING_OPTIONS})
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

    add_executable(scribe_tests tests/tome.cpp tests/json.cpp tests/codegen.cpp)
    target_compile_features(scribe_tests PRIVATE cxx_std_20)
    target_link_libraries(scribe_tests PRIVATE Catch2::Catch2WithMain libscribe)
    target_compile_options(scribe_tests PUBLIC ${SCRIBE_WARNING_OPTIONS} -g)
endif()
