# CUDA runtime packaging (custom CUDA edition)

`keyhunt_cuda.exe` is linked with the CUDA toolkit. On many PCs the driver already provides the runtime; if Windows reports a missing DLL, copy from your toolkit:

```
%CUDA_PATH%\bin\cudart64_12.dll   (CUDA 12.x)
```

`build_cuda_vs2022.bat` / `bats\00_build\build_cuda_vs2022.bat` copy `cudart64_12.dll` next to the exe when `CUDA_PATH` is set.

This DLL is a **dependency**, not a speedup. Throughput comes from device GRP / EC kernels.
