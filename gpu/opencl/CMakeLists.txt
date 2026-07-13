# OpenCL GPU backend (experimental / scaffolding).
#
# To enable:
#   cmake -B build -DENABLE_OPENCL=ON
#
# This directory is a placeholder for the future OpenCL implementation.
# OpenCL gives broader device support than CUDA (AMD, Intel, Apple, mobile).

find_package(OpenCL)

if(NOT OpenCL_FOUND)
    message(FATAL_ERROR "OpenCL not found. Install OpenCL headers/runtime.")
endif()

add_library(truecollider_opencl STATIC
    opencl_backend.cpp
)

target_include_directories(truecollider_opencl PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${OpenCL_INCLUDE_DIRS}
)

target_link_libraries(truecollider_opencl ${OpenCL_LIBRARIES})
target_link_libraries(keyhunt truecollider_opencl)
