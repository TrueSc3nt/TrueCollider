# OpenCL GPU backend

find_package(OpenCL)

if(NOT OpenCL_FOUND)
    message(FATAL_ERROR "OpenCL not found. Install OpenCL headers/runtime.")
endif()

add_library(truecollider_opencl STATIC
    opencl_backend.cpp
)

target_include_directories(truecollider_opencl PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/hash
    ${OpenCL_INCLUDE_DIRS}
)

target_sources(truecollider_opencl PRIVATE
    ${CMAKE_SOURCE_DIR}/hash/sha256.cpp
    ${CMAKE_SOURCE_DIR}/hash/ripemd160.cpp
)

target_link_libraries(truecollider_opencl ${OpenCL_LIBRARIES})
target_link_libraries(keyhunt truecollider_opencl)
