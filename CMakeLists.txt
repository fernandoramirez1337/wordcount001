cmake_minimum_required(VERSION 3.10)
project(OptimizedWordCount VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native -flto")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -Wall -Wextra -Wpedantic")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG /GL")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi /Wall")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Threads REQUIRED)

include_directories(include)

set(CORE_SOURCES
    src/wordcount.cpp
)

add_library(wordcount_core STATIC ${CORE_SOURCES})
target_link_libraries(wordcount_core PRIVATE Threads::Threads)
target_include_directories(wordcount_core PUBLIC include)

add_executable(wordcount src/main.cpp)
target_link_libraries(wordcount PRIVATE wordcount_core Threads::Threads)

add_executable(search src/search.cpp)
target_link_libraries(search PRIVATE wordcount_core Threads::Threads)

set_target_properties(wordcount search
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/data")

install(TARGETS wordcount search wordcount_core
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/ DESTINATION include)

message(STATUS "=== Multithreaded Word Count System ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Release flags: ${CMAKE_CXX_FLAGS_RELEASE}")
else()
    message(STATUS "Debug flags: ${CMAKE_CXX_FLAGS_DEBUG}")
endif()
message(STATUS "Output directory: ${CMAKE_BINARY_DIR}/bin")
message(STATUS "Data directory: ${CMAKE_BINARY_DIR}/data")

add_custom_target(info
    COMMAND ${CMAKE_COMMAND} -E echo "=== CMake Targets ==="
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Build targets:"
    COMMAND ${CMAKE_COMMAND} -E echo "  all               - Build all executables"
    COMMAND ${CMAKE_COMMAND} -E echo "  wordcount         - Build main application"
    COMMAND ${CMAKE_COMMAND} -E echo "  search            - Build search-only application"
    COMMAND ${CMAKE_COMMAND} -E echo "  wordcount_core    - Build core library"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Direct usage:"
    COMMAND ${CMAKE_COMMAND} -E echo "  ./wordcount build <file> <threads> <block_mb> <index_file>"
    COMMAND ${CMAKE_COMMAND} -E echo "  ./wordcount search <index_file> <word>"
    COMMAND ${CMAKE_COMMAND} -E echo "  ./search <index_file> <word>"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Supported configurations:"
    COMMAND ${CMAKE_COMMAND} -E echo "  File size: 20GB"
    COMMAND ${CMAKE_COMMAND} -E echo "  Threads: 1, 2, 8"
    COMMAND ${CMAKE_COMMAND} -E echo "  Block sizes: 16MB, 32MB, 64MB"
    COMMENT "Display help information"
)