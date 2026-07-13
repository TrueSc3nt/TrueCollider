# CMakeLists.txt for TrueCollider verification tests.

add_executable(test_binaryfuse test_binaryfuse.cpp)
target_include_directories(test_binaryfuse PRIVATE ${CMAKE_SOURCE_DIR})

add_test(NAME binaryfuse_filter COMMAND test_binaryfuse)

enable_testing()
