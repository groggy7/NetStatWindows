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

add_executable(NetStatBar WIN32
    ${PROJECT_SOURCE_DIR}/app.manifest
    app/App.cpp
    app/Main.cpp
    app/SamplingWorker.cpp
    app/SingleInstance.cpp
    ui/TrayIcon.cpp
)

target_include_directories(NetStatBar PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_features(NetStatBar PRIVATE cxx_std_20)
target_compile_definitions(NetStatBar PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)
target_compile_options(NetStatBar PRIVATE
    /W4 /WX /permissive- /EHsc /utf-8
)
target_link_libraries(NetStatBar PRIVATE
    netstat_windows_platform
    comctl32
    shell32
    user32
    wtsapi32
)

set_target_properties(NetStatBar PROPERTIES
    OUTPUT_NAME "NetStatBar"
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:NetStatBar>"
)
