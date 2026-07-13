#include "iGameGPSplineSurfaceCUTLASS.h"

#include <cuda_runtime_api.h>
#include <functional>
#include <algorithm>

#include "Util/iGameGP_CUDA_Macros.cuh"

#include "iGameGPSplineKernel.cuh"
#include "iGameGPSplineKernelCPU.h"


