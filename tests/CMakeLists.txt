add_executable(netstat_core_tests CoreTests.cpp)
target_link_libraries(netstat_core_tests PRIVATE netstat_core)
target_compile_features(netstat_core_tests PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(netstat_core_tests PRIVATE /W4 /WX /permissive- /EHsc /utf-8)
else()
    target_compile_options(netstat_core_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME netstat_core_tests COMMAND netstat_core_tests)

if(WIN32)
    add_executable(netstat_windows_platform_tests WindowsPlatformTests.cpp)
    target_link_libraries(
        netstat_windows_platform_tests PRIVATE netstat_windows_platform
    )
    target_compile_features(netstat_windows_platform_tests PRIVATE cxx_std_20)
    if(MSVC)
        target_compile_options(netstat_windows_platform_tests PRIVATE
            /W4 /WX /permissive- /EHsc /utf-8
        )
    else()
        target_compile_options(netstat_windows_platform_tests PRIVATE
            -Wall -Wextra -Wpedantic -Werror
        )
    endif()
    add_test(
        NAME netstat_windows_platform_tests
        COMMAND netstat_windows_platform_tests
    )
endif()
