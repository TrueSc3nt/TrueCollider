#include "backend_config.h"

struct BackendConfig g_backend_config = {
    CPU_VECTOR_AUTO, 1,
    0, GPU_BACKEND_NONE, 0, 65536,
    MODE_ADDRESS
};
