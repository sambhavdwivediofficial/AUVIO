find_package(Catch2 3 CONFIG REQUIRED)

add_executable(AUVIO_Tests
    test_config.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Config.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Logger.cpp
)

target_include_directories(AUVIO_Tests PRIVATE ${CMAKE_SOURCE_DIR}/src/utils)
target_link_libraries(AUVIO_Tests PRIVATE Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(AUVIO_Tests)
