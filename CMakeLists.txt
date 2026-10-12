cmake_minimum_required(VERSION 3.10)
project(UnifiedBot CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

file(GLOB SOURCES "src/*.cpp")

add_executable(UnifiedBot WIN32 ${SOURCES})

target_link_libraries(UnifiedBot
    winhttp
    gdiplus
    ole32
    psapi
    ws2_32
    shell32
    advapi32
)
