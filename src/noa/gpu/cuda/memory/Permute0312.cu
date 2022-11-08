#include "noa/common/Math.h"
#include "noa/gpu/cuda/Exception.h"
#include "noa/gpu/cuda/utils/Traits.h"
#include "noa/gpu/cuda/memory/Permute.h"

#include "noa/gpu/cuda/utils/Block.cuh"

namespace {
    using namespace ::noa;
    using namespace ::noa::cuda;

    // Transpose XY plane (by chunk of 32x32 tiles) for every Z.
    constexpr uint32_t TILE_DIM = 32;
    constexpr dim3 BLOCK_SIZE(TILE_DIM, 256 / TILE_DIM);

    // Out-of-place.
    // The XY tile along Z becomes X'Z' (X'=Y, Z'=X) along Y' (Y'=Z)
    template<typename T, bool IS_MULTIPLE_OF_TILE>
    __global__ __launch_bounds__(BLOCK_SIZE.x * BLOCK_SIZE.y)
    void permute0312_(AccessorRestrict<const T, 4, uint32_t> input,
                      AccessorRestrict<T, 4, uint32_t> output_swapped,
                      uint2_t shape /* YX */ , uint32_t blocks_x) {
        using uninit_t = cuda::utils::traits::uninitialized_type_t<T>;
        __shared__ uninit_t buffer[TILE_DIM][TILE_DIM + 1];
        T(& tile)[TILE_DIM][TILE_DIM + 1] = *reinterpret_cast<T(*)[TILE_DIM][TILE_DIM + 1]>(&buffer);

        const auto input_ = input[blockIdx.z][blockIdx.y];
        const auto output_swapped_ = output_swapped[blockIdx.z][blockIdx.y];

        const uint2_t tid{threadIdx.y, threadIdx.x};
        const uint2_t index = indexing::indexes(blockIdx.x, blocks_x);
        const uint2_t offset = TILE_DIM * index;

        // Read tile to shared memory.
        const uint2_t old_gid = offset + tid;
        for (uint32_t repeat = 0; repeat < TILE_DIM; repeat += BLOCK_SIZE.y) {
            const uint32_t gy = old_gid[0] + repeat;
            if (IS_MULTIPLE_OF_TILE || (old_gid[1] < shape[1] && gy < shape[0]))
                tile[tid[0] + repeat][tid[1]] = input_(gy, old_gid[1]);
        }

        utils::block::synchronize();

        // Write permuted tile to global memory.
        const uint2_t new_gid = offset.flip() + tid;
        for (uint32_t repeat = 0; repeat < TILE_DIM; repeat += BLOCK_SIZE.y) {
            const uint32_t gz = new_gid[0] + repeat;
            if (IS_MULTIPLE_OF_TILE || (new_gid[1] < shape[0] && gz < shape[1]))
                output_swapped_(gz, new_gid[1]) = tile[tid[1]][tid[0] + repeat];
        }
    }

    // Since all axes are permuted, in-place permute cannot easily be expressed as a 2D transposition
    // along a common plane. https://www.aldapa.eus/res/cuTranspose/Readme.html has an implementation
    // based on a 3D shared memory array, but since it is unlikely to be used anyway, don't bother for now.
}

namespace noa::cuda::memory::details {
    template<typename T>
    void permute0312(const shared_t<T[]>& input, dim4_t input_strides,
                     const shared_t<T[]>& output, dim4_t output_strides,
                     dim4_t shape, Stream& stream) {
        NOA_ASSERT_DEVICE_PTR(input.get(), stream.device());
        NOA_ASSERT_DEVICE_PTR(output.get(), stream.device());
        const auto uint_shape = safe_cast<uint2_t>(dim2_t(shape.get(2)));
        const bool are_multiple_tile = all((uint_shape % TILE_DIM) == 0);

        const uint32_t blocks_x = math::divideUp(uint_shape[1], TILE_DIM);
        const uint32_t blocks_y = math::divideUp(uint_shape[0], TILE_DIM);
        const dim3 blocks(blocks_x * blocks_y, shape[1], shape[0]);

        const AccessorRestrict<const T, 4, uint32_t> input_accessor(input.get(), safe_cast<uint4_t>(input_strides));
        const AccessorRestrict<T, 4, uint32_t> output_accessor(output.get(), safe_cast<uint4_t>(output_strides));
        const auto swapped_output = output_accessor.swap(1, 2);

        if (are_multiple_tile) {
            stream.enqueue("memory::permute0312", permute0312_<T, true>, {blocks, BLOCK_SIZE},
                           input_accessor, swapped_output, uint_shape, blocks_x);
        } else {
            stream.enqueue("memory::permute0312", permute0312_<T, false>, {blocks, BLOCK_SIZE},
                           input_accessor, swapped_output, uint_shape, blocks_x);
        }
        stream.attach(input, output);
    }
}

#define NOA_INSTANTIATE_TRANSPOSE_(T) \
template void noa::cuda::memory::details::permute0312<T>(const shared_t<T[]>&, dim4_t, const shared_t<T[]>&, dim4_t, dim4_t, Stream&)

NOA_INSTANTIATE_TRANSPOSE_(bool);
NOA_INSTANTIATE_TRANSPOSE_(int8_t);
NOA_INSTANTIATE_TRANSPOSE_(int16_t);
NOA_INSTANTIATE_TRANSPOSE_(int32_t);
NOA_INSTANTIATE_TRANSPOSE_(int64_t);
NOA_INSTANTIATE_TRANSPOSE_(uint8_t);
NOA_INSTANTIATE_TRANSPOSE_(uint16_t);
NOA_INSTANTIATE_TRANSPOSE_(uint32_t);
NOA_INSTANTIATE_TRANSPOSE_(uint64_t);
NOA_INSTANTIATE_TRANSPOSE_(half_t);
NOA_INSTANTIATE_TRANSPOSE_(float);
NOA_INSTANTIATE_TRANSPOSE_(double);
NOA_INSTANTIATE_TRANSPOSE_(chalf_t);
NOA_INSTANTIATE_TRANSPOSE_(cfloat_t);
NOA_INSTANTIATE_TRANSPOSE_(cdouble_t);
