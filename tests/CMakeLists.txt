set(catch2_SOURCE_DIR lib/Catch2)
add_subdirectory(${catch2_SOURCE_DIR})
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

# Include miso
include_directories(..)

add_executable(tests
    01_simple_connect.cpp
    02_class_connect.cpp
    03_sender.cpp
    04_simulate_gui.cpp
)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)

set_property(TARGET tests PROPERTY CXX_STANDARD 14)

include(CTest)
include(Catch)
catch_discover_tests(tests)
