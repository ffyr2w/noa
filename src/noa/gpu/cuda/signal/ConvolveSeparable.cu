#include "noa/common/Assert.h"
#include "noa/common/Math.h"
#include "noa/gpu/cuda/math/Ewise.h"
#include "noa/gpu/cuda/memory/Copy.h"
#include "noa/gpu/cuda/memory/PtrDevice.h"
#include "noa/gpu/cuda/signal/Convolve.h"
#include "noa/gpu/cuda/util/Block.cuh"
#include "noa/gpu/cuda/util/Pointers.h"

namespace {
    using namespace ::noa;
    using namespace ::noa::cuda;

    constexpr dim3 BLOCK_SIZE(16, 16);
    constexpr int MAX_FILTER_SIZE = 129;
    constexpr int MAX_FILTER_BYTES = MAX_FILTER_SIZE * sizeof(double);
    __constant__ char cfilter[MAX_FILTER_BYTES];

    // This is identical to the convolve1_ kernel.
    template<typename T>
    __global__ __launch_bounds__(BLOCK_SIZE.x * BLOCK_SIZE.y)
    void convolveSeparableX_(const T* __restrict__ input, uint4_t input_strides,
                             T* __restrict__ output, uint4_t output_strides,
                             uint2_t shape /* YX */, int filter_size, uint blocks_x) {

        const uint2_t index = indexing::indexes(blockIdx.x, blocks_x);
        const int2_t tid{threadIdx.y, threadIdx.x};
        const int4_t gid{blockIdx.z,
                         blockIdx.y,
                         BLOCK_SIZE.y * index[0] + tid[0],
                         BLOCK_SIZE.x * index[1] + tid[1]};

        input += indexing::at(gid[0], gid[1], gid[2], input_strides);

        const int padding = filter_size - 1;
        const int halo = padding / 2;
        const int shared_len = static_cast<int>(BLOCK_SIZE.x) + padding;

        // Filter along x.
        T* shared = util::block::dynamicSharedResource<T>();
        if (gid[2] < shape[0]) {
            shared += tid[0] * shared_len; // focus on current row

            // Load shared memory. Loop to take into account padding.
            for (int lx = tid[1], gx = gid[3]; lx < shared_len; lx += BLOCK_SIZE.x, gx += BLOCK_SIZE.x) {
                int i_x = gx - halo;
                shared[lx] = i_x >= 0 && i_x < shape[1] ? input[i_x * input_strides[3]] : static_cast<T>(0);
            }
            util::block::synchronize();

            if (gid[3] < shape[1]) {
                // Weighted sum.
                const T* window = reinterpret_cast<T*>(cfilter);
                T result = static_cast<T>(0);
                for (int idx = 0; idx < filter_size; ++idx)
                    result += shared[tid[1] + idx] * window[idx];
                output[indexing::at(gid, output_strides)] = result;
            }
        }
    }

    template<typename T>
    __global__ __launch_bounds__(BLOCK_SIZE.x * BLOCK_SIZE.y)
    void convolveSeparableY_(const T* __restrict__ input, uint4_t input_strides,
                             T* __restrict__ output, uint4_t output_strides,
                             uint2_t shape /* YX */, int filter_size, uint blocks_x) {

        const uint2_t index = indexing::indexes(blockIdx.x, blocks_x);
        const int2_t tid{threadIdx.y, threadIdx.x};
        const int4_t gid{blockIdx.z,
                         blockIdx.y,
                         BLOCK_SIZE.y * index[0] + tid[0],
                         BLOCK_SIZE.x * index[1] + tid[1]};

        input += indexing::at(gid[0], gid[1], input_strides);

        const int padding = filter_size - 1;
        const int halo = padding / 2;
        const int shared_len_y = static_cast<int>(BLOCK_SIZE.y) + padding;

        // Filter along y.
        T* shared = util::block::dynamicSharedResource<T>();
        if (gid[3] < shape[1]) {
            for (int ly = tid[0], gy = gid[2]; ly < shared_len_y; ly += BLOCK_SIZE.y, gy += BLOCK_SIZE.y) {
                int i_y = gy - halo;
                shared[ly * BLOCK_SIZE.x + tid[1]] = i_y >= 0 && i_y < shape[0] ?
                                                     input[i_y * input_strides[2] + gid[3] * input_strides[3]] :
                                                     static_cast<T>(0);
            }
            util::block::synchronize();

            if (gid[2] < shape[0]) {
                const T* window = reinterpret_cast<T*>(cfilter);
                T result = static_cast<T>(0);
                for (int idx = 0; idx < filter_size; ++idx)
                    result += shared[(tid[0] + idx) * BLOCK_SIZE.x + tid[1]] * window[idx];
                output[indexing::at(gid, output_strides)] = result;
            }
        }
    }

    template<typename T>
    __global__ __launch_bounds__(BLOCK_SIZE.x * BLOCK_SIZE.y)
    void convolveSeparableZ_(const T* __restrict__ input, uint4_t input_strides,
                             T* __restrict__ output, uint4_t output_strides,
                             uint2_t shape /* ZX */, int filter_size, uint blocks_x) {
        T* shared = util::block::dynamicSharedResource<T>();

        const uint2_t index = indexing::indexes(blockIdx.x, blocks_x);
        const int2_t tid{threadIdx.y, threadIdx.x};
        const int4_t gid{blockIdx.z,
                         BLOCK_SIZE.y * index[0] + tid[0],
                         blockIdx.y,
                         BLOCK_SIZE.x * index[1] + tid[1]};

        input += gid[0] * input_strides[0];

        const int padding = filter_size - 1;
        const int halo = padding / 2;
        const int shared_len_z = static_cast<int>(BLOCK_SIZE.y) + padding;

        if (gid[3] < shape[1]) {
            for (int lz = tid[0], gz = gid[1]; lz < shared_len_z; lz += BLOCK_SIZE.y, gz += BLOCK_SIZE.y) {
                int i_z = gz - halo;
                shared[lz * BLOCK_SIZE.x + tid[1]] =
                        i_z >= 0 && i_z < shape[0] ?
                        input[i_z * input_strides[1] + gid[2] * input_strides[2] + gid[3] * input_strides[3]] :
                        static_cast<T>(0);
            }
            util::block::synchronize();

            // Weighted sum.
            if (gid[1] < shape[0]) {
                const T* window = reinterpret_cast<T*>(cfilter);
                T result = static_cast<T>(0);
                for (int idx = 0; idx < filter_size; ++idx)
                    result += shared[(tid[0] + idx) * BLOCK_SIZE.x + tid[1]] * window[idx];
                output[indexing::at(gid, output_strides)] = result;
            }
        }
    }

    template<typename T>
    void launchX(const T* input, size4_t input_strides, T* output, size4_t output_strides, size4_t shape,
                 const T* filter, uint filter_size, cuda::Stream& stream) {
        if (filter_size <= 1)
            return;

        NOA_ASSERT(filter_size * sizeof(T) <= MAX_FILTER_BYTES);
        NOA_THROW_IF(cudaMemcpyToSymbolAsync(cfilter, filter, filter_size * sizeof(T),
                                             0, cudaMemcpyDefault, stream.get()));

        const uint2_t u_shape(shape.get(2));
        const uint blocks_x = noa::math::divideUp(u_shape[1], BLOCK_SIZE.x);
        const uint blocks_y = noa::math::divideUp(u_shape[0], BLOCK_SIZE.y);
        const dim3 blocks(blocks_x * blocks_y, shape[1], shape[0]);
        const uint shared_bytes = (BLOCK_SIZE.x + filter_size - 1) * BLOCK_SIZE.y * sizeof(T);
        const LaunchConfig config{blocks, BLOCK_SIZE, shared_bytes};
        stream.enqueue("memory::convolveX", convolveSeparableX_<T>, config,
                       input, uint4_t(input_strides), output, uint4_t(output_strides), u_shape, filter_size, blocks_x);
    }

    template<typename T>
    void launchY(const T* input, size4_t input_strides, T* output, size4_t output_strides, size4_t shape,
                 const T* filter, uint filter_size, cuda::Stream& stream) {
        if (filter_size <= 1)
            return;

        NOA_ASSERT(filter_size * sizeof(T) <= MAX_FILTER_BYTES);
        NOA_THROW_IF(cudaMemcpyToSymbolAsync(cfilter, filter, filter_size * sizeof(T),
                                             0, cudaMemcpyDefault, stream.get()));

        const uint2_t u_shape(shape.get(2));
        const uint blocks_x = noa::math::divideUp(u_shape[1], BLOCK_SIZE.x);
        const uint blocks_y = noa::math::divideUp(u_shape[0], BLOCK_SIZE.y);
        const dim3 blocks(blocks_x * blocks_y, shape[1], shape[0]);
        const uint shared_bytes = BLOCK_SIZE.x * (BLOCK_SIZE.y + filter_size - 1) * sizeof(T);
        const LaunchConfig config{blocks, BLOCK_SIZE, shared_bytes};
        stream.enqueue("memory::convolveY", convolveSeparableY_<T>, config,
                       input, uint4_t(input_strides), output, uint4_t(output_strides), u_shape, filter_size, blocks_x);
    }

    template<typename T>
    void launchZ(const T* input, size4_t input_strides, T* output, size4_t output_strides, size4_t shape,
                 const T* filter, uint filter_size, cuda::Stream& stream) {
        if (filter_size <= 1)
            return;

        NOA_ASSERT(filter_size * sizeof(T) <= MAX_FILTER_BYTES);
        NOA_THROW_IF(cudaMemcpyToSymbolAsync(cfilter, filter, filter_size * sizeof(T),
                                             0, cudaMemcpyDefault, stream.get()));

        const uint2_t u_shape{shape[1], shape[3]};
        const uint blocks_x = noa::math::divideUp(u_shape[1], BLOCK_SIZE.x);
        const uint blocks_z = noa::math::divideUp(u_shape[0], BLOCK_SIZE.y);
        const dim3 blocks(blocks_x * blocks_z, shape[2], shape[0]);
        const uint shared_bytes = BLOCK_SIZE.x * (BLOCK_SIZE.y + filter_size - 1) * sizeof(T);
        const LaunchConfig config{blocks, BLOCK_SIZE, shared_bytes};
        stream.enqueue("memory::convolveZ", convolveSeparableZ_<T>, config,
                       input, uint4_t(input_strides), output, uint4_t(output_strides), u_shape, filter_size, blocks_x);
    }
}

namespace noa::cuda::signal {
    template<typename T, typename U, typename>
    void convolve(const shared_t<T[]>& input, size4_t input_strides,
                  const shared_t<T[]>& output, size4_t output_strides, size4_t shape,
                  const shared_t<U[]>& filter0, size_t filter0_size,
                  const shared_t<U[]>& filter1, size_t filter1_size,
                  const shared_t<U[]>& filter2, size_t filter2_size, Stream& stream,
                  const shared_t<T[]>& tmp, size4_t tmp_strides) {
        NOA_ASSERT(input != output);

        int count = 0;
        if (filter0)
            count += 1;
        if (filter1)
            count += 1;
        if (filter2)
            count += 1;
        const bool allocate = !tmp && count > 1;
        const shared_t<T[]> buf = allocate ? memory::PtrDevice<T>::alloc(shape.elements(), stream) : tmp;
        const size4_t buf_strides = allocate ? shape.strides() : tmp_strides;

        if (filter0 && filter1 && filter2) {
            NOA_ASSERT(filter0_size % 2);
            NOA_ASSERT(filter1_size % 2);
            NOA_ASSERT(filter2_size % 2);
            launchZ(input.get(), input_strides, output.get(), output_strides, shape, filter0.get(), filter0_size, stream);
            launchY(output.get(), output_strides, buf.get(), buf_strides, shape, filter1.get(), filter1_size, stream);
            launchX(buf.get(), buf_strides, output.get(), output_strides, shape, filter2.get(), filter2_size, stream);
            stream.attach(input, output, buf, filter0, filter1, filter2);

        } else if (filter0 && filter1) {
            NOA_ASSERT(filter0_size % 2);
            NOA_ASSERT(filter1_size % 2);
            launchZ(input.get(), input_strides, buf.get(), buf_strides, shape, filter0.get(), filter0_size, stream);
            launchY(buf.get(), buf_strides, output.get(), output_strides, shape, filter1.get(), filter1_size, stream);
            stream.attach(input, output, buf, filter0, filter1);

        } else if (filter0 && filter2) {
            NOA_ASSERT(filter0_size % 2);
            NOA_ASSERT(filter2_size % 2);
            launchZ(input.get(), input_strides, buf.get(), buf_strides, shape, filter0.get(), filter0_size, stream);
            launchX(buf.get(), buf_strides, output.get(), output_strides, shape, filter2.get(), filter2_size, stream);
            stream.attach(input, output, buf, filter0, filter2);

        } else if (filter1 && filter2) {
            NOA_ASSERT(filter1_size % 2);
            NOA_ASSERT(filter2_size % 2);
            launchY(input.get(), input_strides, buf.get(), buf_strides, shape, filter1.get(), filter1_size, stream);
            launchX(buf.get(), buf_strides, output.get(), output_strides, shape, filter2.get(), filter2_size, stream);
            stream.attach(input, output, buf, filter1, filter2);

        } else if (filter0) {
            NOA_ASSERT(filter0_size % 2);
            launchZ(input.get(), input_strides, output.get(), output_strides, shape, filter0.get(), filter0_size, stream);
            stream.attach(input, output, filter0);
        } else if (filter1) {
            NOA_ASSERT(filter1_size % 2);
            launchY(input.get(), input_strides, output.get(), output_strides, shape, filter1.get(), filter1_size, stream);
            stream.attach(input, output, filter1);
        } else if (filter2) {
            NOA_ASSERT(filter2_size % 2);
            launchX(input.get(), input_strides, output.get(), output_strides, shape, filter2.get(), filter2_size, stream);
            stream.attach(input, output, filter2);
        }
    }

    template<typename T, typename U, typename>
    void convolve(const shared_t<T[]>& input, size4_t input_strides,
                  const shared_t<T[]>& output, size4_t output_strides, size4_t shape,
                  const shared_t<U[]>& filter, size3_t filter_shape, Stream& stream) {
        NOA_ASSERT(all(filter_shape >= 1));
        const size_t ndim = filter_shape.ndim();

        // If there's a single dimension, use separable convolution kernels:
        if (ndim == 1 || (ndim == 3 && filter_shape[1] == 1 && filter_shape[2])) {
            if (all(filter_shape == 1)) {
                // Make sure the (single) value is dereferenceable.
                U filter_value;
                U* filter_ = util::hostPointer(filter.get());
                if (filter_) {
                    filter_value = filter_[0];
                } else {
                    memory::copy(filter.get(), &filter_value, 1, stream);
                    stream.attach(filter);
                }
                math::ewise(input, input_strides, static_cast<T>(filter_value),
                            output, output_strides, shape,
                            noa::math::multiply_t{}, stream);
            } else if (filter_shape[2] > 1) {
                convolve<T, U>(input, input_strides, output, output_strides, shape,
                               nullptr, 0, nullptr, 0, filter, filter_shape[2], stream);
            } else if (filter_shape[1] > 1) {
                convolve<T, U>(input, input_strides, output, output_strides, shape,
                               nullptr, 0, filter, filter_shape[1], nullptr, 0, stream);
            } else {
                convolve<T, U>(input, input_strides, output, output_strides, shape,
                               filter, filter_shape[0], nullptr, 0, nullptr, 0, stream);
            }
            return;
        } else if (ndim == 2) {
            return convolve2(input, input_strides, output, output_strides,
                             shape, filter, {filter_shape[1], filter_shape[2]}, stream);
        } else {
            return convolve3(input, input_strides, output, output_strides,
                             shape, filter, filter_shape, stream);
        }
    }

    #define NOA_INSTANTIATE_CONV_(T)                                         \
    template void convolve<T,T,void>(const shared_t<T[]>&, size4_t,          \
                                     const shared_t<T[]>&, size4_t, size4_t, \
                                     const shared_t<T[]>&, size_t,           \
                                     const shared_t<T[]>&, size_t,           \
                                     const shared_t<T[]>&, size_t,  Stream&, \
                                     const shared_t<T[]>&, size4_t);         \
    template void convolve<T,T,void>(const shared_t<T[]>&, size4_t,          \
                                     const shared_t<T[]>&, size4_t, size4_t, \
                                     const shared_t<T[]>&, size3_t,  Stream&)

    NOA_INSTANTIATE_CONV_(half_t);
    NOA_INSTANTIATE_CONV_(float);
    NOA_INSTANTIATE_CONV_(double);
}
