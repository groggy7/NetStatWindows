cmake_minimum_required(VERSION 3.24)

project(
    NetStatBarWindows
    VERSION 0.1.0
    DESCRIPTION "Native Windows taskbar network speed monitor"
    LANGUAGES CXX
)

option(NETSTAT_BUILD_TESTS "Build the portable unit tests" ON)

add_library(netstat_core STATIC
    src/core/RateCalculator.cpp
    src/core/RateFormatter.cpp
    src/core/Settings.cpp
)

target_include_directories(netstat_core PUBLIC src)
target_compile_features(netstat_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(netstat_core PRIVATE /W4 /WX /permissive- /EHsc /utf-8)
else()
    target_compile_options(netstat_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(WIN32)
    add_subdirectory(src/windows)
endif()

if(NETSTAT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
