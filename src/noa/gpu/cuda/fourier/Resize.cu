#include "noa/Math.h"
#include "noa/gpu/cuda/memory/Copy.h"
#include "noa/gpu/cuda/fourier/Exception.h"
#include "noa/gpu/cuda/fourier/Resize.h"

namespace {
    using namespace noa;

    template<class T>
    __global__ void crop_(const T* in, uint3_t shape_in, uint in_pitch, T* out, uint3_t shape_out, uint out_pitch) {
        // Rebase to the current batch.
        in += in_pitch * shape_in.y * shape_in.z * blockIdx.z;
        out += out_pitch * shape_out.y * shape_out.z * blockIdx.z;

        // Rebase to the current row.
        uint out_y = blockIdx.x, out_z = blockIdx.y;
        uint in_y = out_y < (shape_out.y + 1) / 2 ? out_y : out_y + shape_in.y - shape_out.y;
        uint in_z = out_z < (shape_out.z + 1) / 2 ? out_z : out_z + shape_in.z - shape_out.z;

        in += (in_z * shape_in.y + in_y) * in_pitch;
        out += (out_z * shape_out.y + out_y) * out_pitch;

        // Copy the row.
        for (uint x = threadIdx.x; x < shape_out.x / 2 + 1; x += blockDim.x)
            out[x] = in[x];
    }

    template<class T>
    __global__ void cropFull_(const T* in, uint3_t shape_in, uint in_pitch, T* out, uint3_t shape_out, uint out_pitch) {
        // Rebase to the current batch.
        in += in_pitch * shape_in.y * shape_in.z * blockIdx.z;
        out += out_pitch * shape_out.y * shape_out.z * blockIdx.z;

        // Rebase to the current row.
        uint out_y = blockIdx.x, out_z = blockIdx.y;
        uint in_y = out_y < (shape_out.y + 1) / 2 ? out_y : out_y + shape_in.y - shape_out.y;
        uint in_z = out_z < (shape_out.z + 1) / 2 ? out_z : out_z + shape_in.z - shape_out.z;
        in += (in_z * shape_in.y + in_y) * in_pitch;
        out += (out_z * shape_out.y + out_y) * out_pitch;

        // Similarly to the other dimension, if half in new x is passed, add offset to skip cropped elements.
        for (uint out_x = threadIdx.x; out_x < shape_out.x; out_x += blockDim.x) {
            uint in_x = out_x < (shape_out.x + 1) / 2 ? out_x : out_x + shape_in.x - shape_out.x;
            out[out_x] = in[in_x];
        }
    }

    template<class T>
    __global__ void pad_(const T* in, uint3_t shape_in, uint in_pitch, T* out, uint3_t shape_out, uint out_pitch) {
        // Rebase to the current batch.
        in += in_pitch * shape_in.y * shape_in.z * blockIdx.z;
        out += out_pitch * shape_out.y * shape_out.z * blockIdx.z;

        // Rebase to the current row.
        uint in_y = blockIdx.x, in_z = blockIdx.y;
        uint out_y = in_y < (shape_in.y + 1) / 2 ? in_y : in_y + shape_out.y - shape_in.y;
        uint out_z = in_z < (shape_in.z + 1) / 2 ? in_z : in_z + shape_out.z - shape_in.z;
        in += (in_z * shape_in.y + in_y) * in_pitch;
        out += (out_z * shape_out.y + out_y) * out_pitch;

        // Copy the row.
        for (uint x = threadIdx.x; x < shape_in.x / 2 + 1; x += blockDim.x)
            out[x] = in[x];
    }

    template<class T>
    __global__ void padFull_(const T* in, uint3_t shape_in, uint in_pitch, T* out, uint3_t shape_out, uint out_pitch) {
        // Rebase to the current batch.
        in += in_pitch * shape_in.y * shape_in.z * blockIdx.z;
        out += out_pitch * shape_out.y * shape_out.z * blockIdx.z;

        // Rebase to the current row.
        uint in_y = blockIdx.x, in_z = blockIdx.y;
        uint out_y = in_y < (shape_in.y + 1) / 2 ? in_y : in_y + shape_out.y - shape_in.y;
        uint out_z = in_z < (shape_in.z + 1) / 2 ? in_z : in_z + shape_out.z - shape_in.z;
        in += (in_z * shape_in.y + in_y) * in_pitch;
        out += (out_z * shape_out.y + out_y) * out_pitch;

        // Similarly to the other dimension, if half in new x is passed, add offset to skip padded elements.
        for (uint in_x = threadIdx.x; in_x < shape_in.x; in_x += blockDim.x) {
            uint out_x = in_x < (shape_in.x + 1) / 2 ? in_x : in_x + shape_out.x - shape_in.x;
            out[out_x] = in[in_x];
        }
    }
}

namespace noa::cuda::fourier {
    template<typename T>
    void crop(const T* inputs, size_t inputs_pitch, size3_t inputs_shape,
              T* outputs, size_t outputs_pitch, size3_t outputs_shape, uint batches, Stream& stream) {
        if (all(inputs_shape == outputs_shape)) {
            memory::copy(inputs, inputs_pitch, outputs, outputs_pitch, getShapeFFT(inputs_shape), stream);
            return;
        }
        uint3_t old_shape(inputs_shape), new_shape(outputs_shape);
        uint threads = math::min(256U, math::nextMultipleOf(new_shape.x / 2U + 1, Limits::WARP_SIZE));
        dim3 blocks{new_shape.y, new_shape.z, batches};
        crop_<<<blocks, threads, 0, stream.get()>>>(inputs, old_shape, inputs_pitch, outputs, new_shape, outputs_pitch);
        NOA_THROW_IF(cudaPeekAtLastError());
    }

    template<typename T>
    void cropFull(const T* inputs, size_t inputs_pitch, size3_t inputs_shape,
                  T* outputs, size_t outputs_pitch, size3_t outputs_shape, uint batches, Stream& stream) {
        if (all(inputs_shape == outputs_shape)) {
            memory::copy(inputs, inputs_pitch, outputs, outputs_pitch, inputs_shape, stream);
            return;
        }
        uint3_t old_shape(inputs_shape), new_shape(outputs_shape);
        uint threads = math::min(256U, math::nextMultipleOf(new_shape.x, Limits::WARP_SIZE));
        dim3 blocks{new_shape.y, new_shape.z, batches};
        cropFull_<<<blocks, threads, 0, stream.get()>>>(
                inputs, old_shape, inputs_pitch, outputs, new_shape, outputs_pitch);
        NOA_THROW_IF(cudaPeekAtLastError());
    }

    // TODO: not a priority, but maybe replace memset with a single kernel that loops through output.
    template<typename T>
    void pad(const T* inputs, size_t inputs_pitch, size3_t inputs_shape,
             T* outputs, size_t outputs_pitch, size3_t outputs_shape, uint batches, Stream& stream) {
        if (all(inputs_shape == outputs_shape)) {
            memory::copy(inputs, inputs_pitch, outputs, outputs_pitch, getShapeFFT(inputs_shape), stream);
            return;
        }
        NOA_THROW_IF(cudaMemsetAsync(outputs, 0, outputs_pitch * getRows(outputs_shape) * sizeof(T), stream.get()));

        uint3_t old_shape(inputs_shape), new_shape(outputs_shape);
        uint threads = math::min(256U, math::nextMultipleOf(old_shape.x / 2U + 1U, Limits::WARP_SIZE));
        dim3 blocks{old_shape.y, old_shape.z, batches};
        pad_<<<blocks, threads, 0, stream.get()>>>(
                inputs, old_shape, inputs_pitch, outputs, new_shape, outputs_pitch);
        NOA_THROW_IF(cudaPeekAtLastError());
    }

    // TODO: not a priority, but maybe replace memset with kernel that loops through output.
    template<typename T>
    void padFull(const T* inputs, size_t inputs_pitch, size3_t inputs_shape,
                 T* outputs, size_t outputs_pitch, size3_t outputs_shape,
                 uint batches, Stream& stream) {
        if (all(inputs_shape == outputs_shape)) {
            memory::copy(inputs, inputs_pitch, outputs, outputs_pitch, inputs_shape, stream);
            return;
        }
        NOA_THROW_IF(cudaMemsetAsync(outputs, 0, outputs_pitch * getRows(outputs_shape) * sizeof(T), stream.get()));

        uint3_t old_shape(inputs_shape), new_shape(outputs_shape);
        uint threads = math::min(256U, math::nextMultipleOf(old_shape.x, Limits::WARP_SIZE));
        dim3 blocks{old_shape.y, old_shape.z, batches};
        padFull_<<<blocks, threads, 0, stream.get()>>>(
                inputs, old_shape, inputs_pitch, outputs, new_shape, outputs_pitch);
        NOA_THROW_IF(cudaPeekAtLastError());
    }

    #define INSTANTIATE_CROP(T) \
    template void crop<T>(const T*, size_t, size3_t, T*, size_t, size3_t, uint, Stream&);       \
    template void cropFull<T>(const T*, size_t, size3_t, T*, size_t, size3_t, uint, Stream&);   \
    template void pad<T>(const T*, size_t, size3_t, T*, size_t, size3_t, uint, Stream&);        \
    template void padFull<T>(const T*, size_t, size3_t, T*, size_t, size3_t, uint, Stream&)

    INSTANTIATE_CROP(cfloat_t);
    INSTANTIATE_CROP(float);
    INSTANTIATE_CROP(cdouble_t);
    INSTANTIATE_CROP(double);
}
