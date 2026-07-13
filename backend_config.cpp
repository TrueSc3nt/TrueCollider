#include "backend_config.h"

struct BackendConfig g_backend_config = {
    CPU_VECTOR_NONE, 1,
    0, GPU_BACKEND_NONE, 0, 8192, 0, /* gpu_batch_size, gpu_batch_user_set */
    0, 0,                            /* memory_budget_bytes, memory_auto */
    MODE_ADDRESS
};
