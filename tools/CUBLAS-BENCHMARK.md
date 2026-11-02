# 4096 cuBLAS GEMM benchmark

This standalone CUDA program reproduces the configuration provided for the
FP16/BF16 comparison and also reports the FP32-pedantic and TF32 modes. It does
not modify GPU settings; it only allocates matrices and measures cuBLAS GEMM.

Configuration:

- 4096 × 4096 × 4096 GEMM, with FP32 output in all four modes.
- FP32 pedantic: FP32 inputs, FP32 compute, FP32 output.
- FP16 Tensor: FP16 inputs, FP32 compute/accumulation, FP32 output.
- BF16 Tensor: BF16 inputs, FP32 compute/accumulation, FP32 output.
- TF32 Tensor: FP32 inputs, TF32 Tensor Core compute, FP32 output.
- Two seconds of warmup per precision, then 30 event-timed samples per trial
  over three trials. It reports each trial's median and the median of those
  three medians.
- A correctness check expects the first output element to be approximately 1.

The FP16 and BF16 `cublasGemmEx` type combinations use 16-bit inputs with
`CUBLAS_COMPUTE_32F` and FP32 output, as supported by NVIDIA's cuBLAS API. The
program prints the CUDA runtime, driver, cuBLAS, GPU, and
`NVIDIA_TF32_OVERRIDE` values with the results.

## Results

RTX 4050 Laptop GPU 6 GB · Acer Nitro V 15 ANV15-41 · 4096 × 4096 CUDA/cuBLAS:

| Precision | Before (TFLOPS) | After (TFLOPS) | Change | Relative speed |
|---|---:|---:|---:|---:|
| FP16 | 41.57 | 78.87 | +37.30 (+89.7%) | 1.90× |
| BF16 | 41.51 | 78.67 | +37.16 (+89.5%) | 1.90× |

## Build and run on Windows

Open a Visual Studio 2022 Developer PowerShell with CUDA Toolkit 12.6 installed,
then run from the repository root:

```powershell
nvcc -std=c++17 .\tools\cublas_4096_benchmark.cu -o .\tools\cublas_4096_benchmark.exe -lcublas
.\tools\cublas_4096_benchmark.exe
```

