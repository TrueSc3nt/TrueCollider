# CUDA GPU backend (experimental / scaffolding).
#
# To enable:
#   cmake -B build -DENABLE_CUDA=ON
#
# This directory is a placeholder for the future CUDA implementation.
# A full implementation would add:
#   - CUDA kernels for secp256k1 point multiplication
#   - Batch hash160 generation
#   - Bloom / binary-fuse filter lookups on device
#   - Integration into keyhunt.cpp via a GPU search dispatcher

if(NOT CMAKE_CUDA_COMPILER)
    message(FATAL_ERROR "CUDA compiler not found. Install the CUDA toolkit.")
endif()

set(CMAKE_CUDA_STANDARD 14)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

add_library(truecollider_cuda STATIC
    cuda_backend.cu
    hash160_cuda.cu
    secp256k1_cuda.cu
)

target_include_directories(truecollider_cuda PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/secp256k1
    ${CMAKE_SOURCE_DIR}/hash
)

set_target_properties(truecollider_cuda PROPERTIES
    CUDA_SEPARABLE_COMPILATION OFF
    POSITION_INDEPENDENT_CODE ON
    CUDA_RESOLVE_DEVICE_SYMBOLS ON
)

target_link_libraries(keyhunt truecollider_cuda CUDA::cudart)
