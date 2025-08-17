cmake_minimum_required(VERSION 3.10)
project(skirk)

set(CMAKE_CXX_STANDARD 17)

add_executable(skirk
    src/main.cpp
    src/cli/bridge.cpp
    src/commands/init.cpp
    src/utils/fileProd.cpp
    src/commands/hash-file.cpp
    src/commands/cat-file.cpp
    src/utils/objectClasses.cpp
    src/commands/checkout.cpp 
    src/commands/ls-tree.cpp 
    src/commands/show-ref.cpp 
    src/commands/tag.cpp 
    src/utils/refWorkers.cpp
     src/utils/index.cpp
    # add other source files as needed
)

find_package(ZLIB REQUIRED)
target_link_libraries(skirk PRIVATE ZLIB::ZLIB)

# C++17 filesystem handling - no separate package needed on macOS
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
  target_link_libraries(skirk stdc++fs)
endif()