#include <cuda_bf16.h>
#include <cuda_fp16.h>
#include <cuda_runtime.h>
#include <cublas_v2.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr int kN = 4096;
constexpr int kWarmupSeconds = 2;
constexpr int kSamplesPerTrial = 30;
constexpr int kTrials = 3;
constexpr int kWarmupBatch = 8;
constexpr int kThreads = 256;
constexpr float kInputValue = 1.0f / 64.0f;

#define CUDA_CHECK(call) \
  do { \
    const cudaError_t status = (call); \
    if (status != cudaSuccess) { \
      throw std::runtime_error(std::string("CUDA: ") + cudaGetErrorString(status)); \
    } \
  } while (0)

#define CUBLAS_CHECK(call) \
  do { \
    const cublasStatus_t status = (call); \
    if (status != CUBLAS_STATUS_SUCCESS) { \
      throw std::runtime_error(std::string("cuBLAS status ") + std::to_string(static_cast<int>(status))); \
    } \
  } while (0)

__global__ void FillFloat(float *data, size_t count, float value) {
  const size_t index = static_cast<size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (index < count) {
    data[index] = value;
  }
}

__global__ void FillHalf(__half *data, size_t count, float value) {
  const size_t index = static_cast<size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (index < count) {
    data[index] = __float2half_rn(value);
  }
}

__global__ void FillBFloat16(__nv_bfloat16 *data, size_t count, float value) {
  const size_t index = static_cast<size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (index < count) {
    data[index] = __float2bfloat16(value);
  }
}

enum class InputKind {
  Float32,
  Float16,
  BFloat16,
};

struct Mode {
  const char *name;
  InputKind input;
  cublasComputeType_t compute;
  cublasGemmAlgo_t algorithm;
};

struct Buffers {
  void *a = nullptr;
  void *b = nullptr;
  float *c = nullptr;
  cublasHandle_t handle = nullptr;
  cudaEvent_t start = nullptr;
  cudaEvent_t stop = nullptr;

  ~Buffers() {
    if (start != nullptr) cudaEventDestroy(start);
    if (stop != nullptr) cudaEventDestroy(stop);
    if (handle != nullptr) cublasDestroy(handle);
    if (a != nullptr) cudaFree(a);
    if (b != nullptr) cudaFree(b);
    if (c != nullptr) cudaFree(c);
  }
};

double Median(std::vector<double> values) {
  if (values.empty()) {
    throw std::runtime_error("cannot compute a median of no samples");
  }
  std::sort(values.begin(), values.end());
  const size_t middle = values.size() / 2;
  if ((values.size() % 2) != 0) {
    return values[middle];
  }
  return (values[middle - 1] + values[middle]) / 2.0;
}

size_t InputBytes(InputKind input) {
  const size_t elements = static_cast<size_t>(kN) * kN;
  return elements * ((input == InputKind::Float32) ? sizeof(float) : sizeof(uint16_t));
}

cudaDataType_t InputDataType(InputKind input) {
  switch (input) {
    case InputKind::Float16: return CUDA_R_16F;
    case InputKind::BFloat16: return CUDA_R_16BF;
    case InputKind::Float32: return CUDA_R_32F;
  }
  throw std::runtime_error("unknown input type");
}

void Prepare(const Mode &mode, Buffers *buffers) {
  const size_t elements = static_cast<size_t>(kN) * kN;
  const unsigned int blocks = static_cast<unsigned int>((elements + kThreads - 1) / kThreads);
  const size_t inputBytes = InputBytes(mode.input);
  const size_t outputBytes = elements * sizeof(float);

  CUDA_CHECK(cudaMalloc(&buffers->a, inputBytes));
  CUDA_CHECK(cudaMalloc(&buffers->b, inputBytes));
  CUDA_CHECK(cudaMalloc(&buffers->c, outputBytes));

  if (mode.input == InputKind::Float16) {
    FillHalf<<<blocks, kThreads>>>(static_cast<__half *>(buffers->a), elements, kInputValue);
    FillHalf<<<blocks, kThreads>>>(static_cast<__half *>(buffers->b), elements, kInputValue);
  } else if (mode.input == InputKind::BFloat16) {
    FillBFloat16<<<blocks, kThreads>>>(static_cast<__nv_bfloat16 *>(buffers->a), elements, kInputValue);
    FillBFloat16<<<blocks, kThreads>>>(static_cast<__nv_bfloat16 *>(buffers->b), elements, kInputValue);
  } else {
    FillFloat<<<blocks, kThreads>>>(static_cast<float *>(buffers->a), elements, kInputValue);
    FillFloat<<<blocks, kThreads>>>(static_cast<float *>(buffers->b), elements, kInputValue);
  }
  CUDA_CHECK(cudaGetLastError());
  CUDA_CHECK(cudaDeviceSynchronize());

  CUBLAS_CHECK(cublasCreate(&buffers->handle));
  const cublasMath_t mathMode = (mode.compute == CUBLAS_COMPUTE_32F_PEDANTIC)
                                    ? CUBLAS_DEFAULT_MATH
                                    : CUBLAS_TENSOR_OP_MATH;
  CUBLAS_CHECK(cublasSetMathMode(buffers->handle, mathMode));
  CUDA_CHECK(cudaEventCreate(&buffers->start));
  CUDA_CHECK(cudaEventCreate(&buffers->stop));
}

void Gemm(const Mode &mode, const Buffers &buffers) {
  const float alpha = 1.0f;
  const float beta = 0.0f;
  const cudaDataType_t inputType = InputDataType(mode.input);
  CUBLAS_CHECK(cublasGemmEx(
      buffers.handle,
      CUBLAS_OP_N,
      CUBLAS_OP_N,
      kN,
      kN,
      kN,
      &alpha,
      buffers.a,
      inputType,
      kN,
      buffers.b,
      inputType,
      kN,
      &beta,
      buffers.c,
      CUDA_R_32F,
      kN,
      mode.compute,
      mode.algorithm));
}

void Warmup(const Mode &mode, const Buffers &buffers) {
  const auto start = std::chrono::steady_clock::now();
  do {
    for (int index = 0; index < kWarmupBatch; ++index) {
      Gemm(mode, buffers);
    }
    CUDA_CHECK(cudaDeviceSynchronize());
  } while (std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() < kWarmupSeconds);
}

double MeasureOne(const Mode &mode, const Buffers &buffers) {
  CUDA_CHECK(cudaEventRecord(buffers.start));
  Gemm(mode, buffers);
  CUDA_CHECK(cudaEventRecord(buffers.stop));
  CUDA_CHECK(cudaEventSynchronize(buffers.stop));

  float milliseconds = 0.0f;
  CUDA_CHECK(cudaEventElapsedTime(&milliseconds, buffers.start, buffers.stop));
  if (!(milliseconds > 0.0f)) {
    throw std::runtime_error("CUDA event returned a non-positive duration");
  }

  const double operations = 2.0 * static_cast<double>(kN) * kN * kN;
  return operations / (static_cast<double>(milliseconds) * 1.0e9);
}

double RunMode(const Mode &mode) {
  Buffers buffers;
  Prepare(mode, &buffers);
  Warmup(mode, buffers);

  std::vector<double> trialMedians;
  trialMedians.reserve(kTrials);
  for (int trial = 0; trial < kTrials; ++trial) {
    std::vector<double> samples;
    samples.reserve(kSamplesPerTrial);
    for (int sample = 0; sample < kSamplesPerTrial; ++sample) {
      samples.push_back(MeasureOne(mode, buffers));
    }
    const double trialMedian = Median(samples);
    trialMedians.push_back(trialMedian);
    std::printf("TRIAL|%s|%d|%.2f TFLOPS\n", mode.name, trial + 1, trialMedian);
  }

  float firstOutput = 0.0f;
  CUDA_CHECK(cudaMemcpy(&firstOutput, buffers.c, sizeof(firstOutput), cudaMemcpyDeviceToHost));
  if (!std::isfinite(firstOutput) || std::fabs(firstOutput - 1.0f) > 0.01f) {
    throw std::runtime_error(std::string(mode.name) + " correctness check failed; C[0]=" + std::to_string(firstOutput));
  }

  const double result = Median(trialMedians);
  std::printf("RESULT|%s|%.2f TFLOPS|median of 3 trial medians\n", mode.name, result);
  return result;
}

}  // namespace

int main() {
  try {
    int deviceCount = 0;
    CUDA_CHECK(cudaGetDeviceCount(&deviceCount));
    if (deviceCount < 1) {
      throw std::runtime_error("no CUDA device found");
    }
    CUDA_CHECK(cudaSetDevice(0));

    cudaDeviceProp properties{};
    CUDA_CHECK(cudaGetDeviceProperties(&properties, 0));
    int runtimeVersion = 0;
    int driverVersion = 0;
    CUDA_CHECK(cudaRuntimeGetVersion(&runtimeVersion));
    CUDA_CHECK(cudaDriverGetVersion(&driverVersion));
    cublasHandle_t versionHandle = nullptr;
    CUBLAS_CHECK(cublasCreate(&versionHandle));
    int cublasVersion = 0;
    const cublasStatus_t versionStatus = cublasGetVersion(versionHandle, &cublasVersion);
    cublasDestroy(versionHandle);
    CUBLAS_CHECK(versionStatus);

    std::printf("GPU|%s|cc=%d.%d\n", properties.name, properties.major, properties.minor);
    std::printf("VERSIONS|CUDA_runtime=%d|CUDA_driver=%d|cuBLAS=%d\n", runtimeVersion, driverVersion, cublasVersion);
    const char *tf32Override = std::getenv("NVIDIA_TF32_OVERRIDE");
    std::printf("NVIDIA_TF32_OVERRIDE|%s\n", tf32Override == nullptr ? "unset" : tf32Override);
    std::printf("CONFIG|N=%d|warmup_seconds=%d_per_precision|samples=%d_per_trial|trials=%d\n",
                kN, kWarmupSeconds, kSamplesPerTrial, kTrials);
    std::printf("STATISTIC|median_of_30_samples_per_trial_then_median_of_3_trial_medians\n");

    const Mode modes[] = {
      {"FP32_PEDANTIC", InputKind::Float32, CUBLAS_COMPUTE_32F_PEDANTIC, CUBLAS_GEMM_DEFAULT},
      {"FP16_TENSOR", InputKind::Float16, CUBLAS_COMPUTE_32F, CUBLAS_GEMM_DEFAULT_TENSOR_OP},
      {"BF16_TENSOR", InputKind::BFloat16, CUBLAS_COMPUTE_32F, CUBLAS_GEMM_DEFAULT_TENSOR_OP},
      {"TF32_TENSOR", InputKind::Float32, CUBLAS_COMPUTE_32F_FAST_TF32, CUBLAS_GEMM_DEFAULT_TENSOR_OP},
    };

    for (const Mode &mode : modes) {
      RunMode(mode);
    }
    return 0;
  } catch (const std::exception &error) {
    std::fprintf(stderr, "benchmark failed: %s\n", error.what());
    return 1;
  }
}
