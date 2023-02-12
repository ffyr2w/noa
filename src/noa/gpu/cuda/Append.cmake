# Included files for noa/gpu/cuda:
if (NOT NOA_ENABLE_CUDA)
    return()
endif ()

set(NOA_CUDA_HEADERS
        # noa::cuda
        gpu/cuda/Device.h
        gpu/cuda/Event.h
        gpu/cuda/Ewise.h
        gpu/cuda/Exception.h
        gpu/cuda/Find.h
        gpu/cuda/Sort.h
        gpu/cuda/Stream.h
        gpu/cuda/Types.h

        # noa::cuda::utils
        gpu/cuda/utils/Block.cuh
        gpu/cuda/utils/EwiseBinary.cuh
        gpu/cuda/utils/EwiseTrinary.cuh
        gpu/cuda/utils/EwiseUnary.cuh
        gpu/cuda/utils/Iwise.cuh
        gpu/cuda/utils/Pointers.h
        gpu/cuda/utils/ReduceUnary.cuh
        gpu/cuda/utils/ReduceBinary.cuh
        gpu/cuda/utils/Version.h
        gpu/cuda/utils/Warp.cuh

        # noa::cuda::memory
        gpu/cuda/memory/Arange.h
        gpu/cuda/memory/Cast.h
        gpu/cuda/memory/Copy.h
        gpu/cuda/memory/Index.h
        gpu/cuda/memory/Iota.h
        gpu/cuda/memory/Linspace.h
        gpu/cuda/memory/MemoryPool.h
        gpu/cuda/memory/Permute.h
        gpu/cuda/memory/PtrArray.h
        gpu/cuda/memory/PtrDevice.h
        gpu/cuda/memory/PtrDevicePadded.h
        gpu/cuda/memory/PtrManaged.h
        gpu/cuda/memory/PtrPinned.h
        gpu/cuda/memory/PtrTexture.h
        gpu/cuda/memory/Resize.h
        gpu/cuda/memory/Set.h
        gpu/cuda/memory/Subregion.hpp

#        # noa::cuda::fft
#        gpu/cuda/fft/Exception.h
#        gpu/cuda/fft/Plan.h
#        gpu/cuda/fft/Remap.h
#        gpu/cuda/fft/Resize.h
#        gpu/cuda/fft/Transforms.h
#
#        # noa::cuda::math
        gpu/cuda/math/Blas.h
        gpu/cuda/math/Complex.h
        gpu/cuda/math/Random.h
        gpu/cuda/math/Reduce.h

#        # noa::cuda::signal
#        gpu/cuda/signal/fft/Bandpass.h
#        gpu/cuda/signal/fft/Correlate.h
#        gpu/cuda/signal/fft/FSC.h
#        gpu/cuda/signal/fft/Shape.h
#        gpu/cuda/signal/fft/Shift.h
#        gpu/cuda/signal/fft/Standardize.h
#        gpu/cuda/signal/Convolve.h
#        gpu/cuda/signal/Median.h
#        gpu/cuda/signal/Shape.h
#
#        # noa::cuda::geometry
#        gpu/cuda/geometry/fft/Polar.h
#        gpu/cuda/geometry/fft/Project.h
#        gpu/cuda/geometry/fft/Transform.h
#        gpu/cuda/geometry/Interpolator.h
#        gpu/cuda/geometry/Polar.h
#        gpu/cuda/geometry/Prefilter.h
#        gpu/cuda/geometry/Transform.h

        )

set(NOA_CUDA_SOURCES
        gpu/cuda/Device.cpp
        gpu/cuda/utils/test.cu
        gpu/cuda/utils/test1.cpp

        gpu/cuda/EwiseBinaryArithmetic.cu
        gpu/cuda/EwiseBinaryComparison.cu
        gpu/cuda/EwiseTrinaryArithmetic.cu
        gpu/cuda/EwiseTrinaryComparison.cu
        gpu/cuda/EwiseTrinaryComplexDivide.cu
        gpu/cuda/EwiseTrinaryComplexMinus.cu
        gpu/cuda/EwiseTrinaryComplexMultiply.cu
        gpu/cuda/EwiseTrinaryComplexPlus.cu
        gpu/cuda/EwiseUnary.cu
        gpu/cuda/FindMin.cu
        gpu/cuda/FindMax.cu
        gpu/cuda/Sort.cu

#        # noa::cuda::fft
#        gpu/cuda/fft/Exception.cpp
#        gpu/cuda/fft/Plan.cpp
#        gpu/cuda/fft/Remap.cu
#        gpu/cuda/fft/Resize.cu

        # noa::cuda::memory
        gpu/cuda/memory/Arange.cu
        gpu/cuda/memory/Cast.cu
        gpu/cuda/memory/Copy.cu
        gpu/cuda/memory/Index.cu
        gpu/cuda/memory/Iota.cu
        gpu/cuda/memory/Linspace.cu
        gpu/cuda/memory/Permute0132.cu
        gpu/cuda/memory/Permute0213.cu
        gpu/cuda/memory/Permute0231.cu
        gpu/cuda/memory/Permute0312.cu
        gpu/cuda/memory/Permute0321.cu
        gpu/cuda/memory/Resize.cu
        gpu/cuda/memory/Set.cu
        gpu/cuda/memory/Subregion.cu

        # noa::cuda::math
        gpu/cuda/math/Blas.cu
        gpu/cuda/math/Complex.cu
        gpu/cuda/math/Random.cu
        gpu/cuda/math/Reduce.cu
        gpu/cuda/math/ReduceAxes.cu
        gpu/cuda/math/ReduceAxesVariance.cu

#        # noa::cuda::signal
#        gpu/cuda/signal/fft/Bandpass.cu
#        gpu/cuda/signal/fft/Correlate.cu
#        gpu/cuda/signal/fft/CorrelatePeak.cu
#        gpu/cuda/signal/fft/FSC.cu
#        gpu/cuda/signal/fft/Shape2D.cu
#        gpu/cuda/signal/fft/Shape3D.cu
#        gpu/cuda/signal/fft/Shift.cu
#        gpu/cuda/signal/fft/Standardize.cu
#        gpu/cuda/signal/Convolve1.cu
#        gpu/cuda/signal/Convolve2.cu
#        gpu/cuda/signal/Convolve3.cu
#        gpu/cuda/signal/ConvolveSeparable.cu
#        gpu/cuda/signal/Median.cu
#
#        # noa::cuda::geometry
#        gpu/cuda/geometry/fft/Polar.cu
#        gpu/cuda/geometry/fft/Project.cu
#        gpu/cuda/geometry/fft/Transform.cu
#        gpu/cuda/geometry/fft/TransformTexture.cu
#        gpu/cuda/geometry/Polar.cu
#        gpu/cuda/geometry/Prefilter.cu
#        gpu/cuda/geometry/Transform.cu
#        gpu/cuda/geometry/TransformTexture.cu

        )

set(NOA_HEADERS ${NOA_HEADERS} ${NOA_CUDA_HEADERS})
set(NOA_SOURCES ${NOA_SOURCES} ${NOA_CUDA_SOURCES})
