cmake_minimum_required(VERSION 3.16.0)
project(scribe LANGUAGES CXX)

# dependencies
include(cmake/CPM.cmake)
CPMAddPackage("gh:fmtlib/fmt#10.1.1")
CPMAddPackage("gh:CLIUtils/CLI11@2.3.2")
CPMAddPackage("gh:nlohmann/json@3.11.2")

# main library
add_library(libscribe src/verify_json.cpp)
target_compile_features(libscribe PUBLIC cxx_std_20)
target_include_directories(libscribe PUBLIC include)
target_link_libraries(libscribe PUBLIC fmt::fmt nlohmann_json::nlohmann_json)
target_compile_options(libscribe PRIVATE -Wall -Wextra -Wno-deprecated-declarations -Wno-stringop-overflow -Werror -O2 )

# executables
add_executable(scribe src/main.cpp src/schema.cpp src/verify_json.cpp)
target_compile_features(scribe PUBLIC cxx_std_20)
target_compile_options(scribe PRIVATE -Wall -Wextra -Wno-deprecated-declarations -Wno-stringop-overflow -Werror -O2)
target_link_libraries(scribe libscribe CLI11::CLI11 nlohmann_json::nlohmann_json)

# install
file(GLOB files_h "src/include/scribe/*.h")
install(TARGETS libscribe DESTINATION lib)
install(FILES ${files_h} DESTINATION include/scribe)
