add_library(netstat_windows_platform STATIC
    platform/MonotonicClock.cpp
    platform/RegistrySettingsStore.cpp
    platform/WindowsNetworkSnapshotProvider.cpp
)

target_include_directories(netstat_windows_platform PUBLIC
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_features(netstat_windows_platform PUBLIC cxx_std_20)
target_compile_definitions(netstat_windows_platform PUBLIC
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)
target_compile_options(netstat_windows_platform PRIVATE
    /W4 /WX /permissive- /EHsc /utf-8
)
target_link_libraries(netstat_windows_platform
    PUBLIC netstat_core
    PRIVATE advapi32 iphlpapi
)
