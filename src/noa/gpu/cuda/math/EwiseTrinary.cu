#include "noa/gpu/cuda/math/Ewise.h"
#include "noa/gpu/cuda/memory/PtrDevice.h"
#include "noa/gpu/cuda/utils/EwiseTrinary.cuh"

namespace noa::cuda::math {
    template<typename Lhs, typename Mhs, typename Rhs, typename Out, typename TrinaryOp, typename>
    void ewise(const shared_t<Lhs[]>& lhs, dim4_t lhs_strides, Mhs mhs, Rhs rhs,
               const shared_t<Out[]>& output, dim4_t output_strides,
               dim4_t shape, TrinaryOp trinary_op, Stream& stream) {
        cuda::utils::ewise::trinary(
                "math::ewise",
                lhs.get(), lhs_strides, mhs, rhs,
                output.get(), output_strides,
                shape, true, stream, trinary_op);
        stream.attach(lhs, output);
    }

    template<typename Lhs, typename Mhs, typename Rhs, typename Out, typename TrinaryOp, typename>
    void ewise(const shared_t<Lhs[]>& lhs, dim4_t lhs_strides,
               const shared_t<Mhs[]>& mhs, dim4_t mhs_strides,
               Rhs rhs,
               const shared_t<Out[]>& output, dim4_t output_strides,
               dim4_t shape, TrinaryOp trinary_op, Stream& stream) {
        cuda::utils::ewise::trinary(
                "math::ewise",
                lhs.get(), lhs_strides,
                mhs.get(), mhs_strides,
                rhs,
                output.get(), output_strides,
                shape, true, stream, trinary_op);
        stream.attach(lhs, mhs, output);
    }

    template<typename Lhs, typename Mhs, typename Rhs, typename Out, typename TrinaryOp, typename>
    void ewise(const shared_t<Lhs[]>& lhs, dim4_t lhs_strides,
               Mhs mhs,
               const shared_t<Rhs[]>& rhs, dim4_t rhs_strides,
               const shared_t<Out[]>& output, dim4_t output_strides,
               dim4_t shape, TrinaryOp trinary_op, Stream& stream) {
        cuda::utils::ewise::trinary(
                "math::ewise",
                lhs.get(), lhs_strides,
                mhs,
                rhs.get(), rhs_strides,
                output.get(), output_strides,
                shape, true, stream, trinary_op);
        stream.attach(lhs, rhs, output);
    }

    template<typename Lhs, typename Mhs, typename Rhs, typename Out, typename TrinaryOp, typename>
    void ewise(const shared_t<Lhs[]>& lhs, dim4_t lhs_strides,
               const shared_t<Mhs[]>& mhs, dim4_t mhs_strides,
               const shared_t<Rhs[]>& rhs, dim4_t rhs_strides,
               const shared_t<Out[]>& output, dim4_t output_strides,
               dim4_t shape, TrinaryOp trinary_op, Stream& stream) {
        cuda::utils::ewise::trinary(
                "math::ewise",
                lhs.get(), lhs_strides,
                mhs.get(), mhs_strides,
                rhs.get(), rhs_strides,
                output.get(), output_strides,
                shape, true, stream, trinary_op);
        stream.attach(lhs, mhs, rhs, output);
    }

    #define NOA_INSTANTIATE_EWISE_TRINARY(T,U,V,W,O)        \
    template void ewise<T,U,V,W,O,void>(                    \
        const shared_t<T[]>&, dim4_t, U, V,                 \
        const shared_t<W[]>&, dim4_t, dim4_t, O, Stream&);  \
    template void ewise<T,U,V,W,O,void>(                    \
        const shared_t<T[]>&, dim4_t,                       \
        const shared_t<U[]>&, dim4_t, V,                    \
        const shared_t<W[]>&, dim4_t, dim4_t, O, Stream&);  \
    template void ewise<T,U,V,W,O,void>(                    \
        const shared_t<T[]>&, dim4_t, U,                    \
        const shared_t<V[]>&, dim4_t,                       \
        const shared_t<W[]>&, dim4_t, dim4_t, O, Stream&);  \
    template void ewise<T,U,V,W,O,void>(                    \
        const shared_t<T[]>&, dim4_t,                       \
        const shared_t<U[]>&, dim4_t,                       \
        const shared_t<V[]>&, dim4_t,                       \
        const shared_t<W[]>&, dim4_t, dim4_t, O, Stream&)

    #define NOA_INSTANTIATE_EWISE_TRINARY_WITHIN(T,W)               \
    NOA_INSTANTIATE_EWISE_TRINARY(T,T,T,W,::noa::math::within_t);   \
    NOA_INSTANTIATE_EWISE_TRINARY(T,T,T,W,::noa::math::within_equal_t)

    NOA_INSTANTIATE_EWISE_TRINARY_WITHIN(int16_t, int16_t);
    NOA_INSTANTIATE_EWISE_TRINARY_WITHIN(int32_t, int32_t);
    NOA_INSTANTIATE_EWISE_TRINARY_WITHIN(int64_t, int64_t);
    NOA_INSTANTIATE_EWISE_TRINARY_WITHIN(uint16_t, uint16_t);
    NOA_INSTANTIATE_EWISE_TRINARY_WITHIN(uint32_t, uint32_t);
    NOA_INSTANTIATE_EWISE_TRINARY_WITHIN(uint64_t, uint64_t);
    NOA_INSTANTIATE_EWISE_TRINARY_WITHIN(half_t, half_t);
    NOA_INSTANTIATE_EWISE_TRINARY_WITHIN(float, float);
    NOA_INSTANTIATE_EWISE_TRINARY_WITHIN(double, double);

    NOA_INSTANTIATE_EWISE_TRINARY_WITHIN(int16_t, bool);
    NOA_INSTANTIATE_EWISE_TRINARY_WITHIN(int32_t, bool);
    NOA_INSTANTIATE_EWISE_TRINARY_WITHIN(int64_t, bool);
    NOA_INSTANTIATE_EWISE_TRINARY_WITHIN(uint16_t, bool);
    NOA_INSTANTIATE_EWISE_TRINARY_WITHIN(uint32_t, bool);
    NOA_INSTANTIATE_EWISE_TRINARY_WITHIN(uint64_t, bool);
    NOA_INSTANTIATE_EWISE_TRINARY_WITHIN(half_t, bool);
    NOA_INSTANTIATE_EWISE_TRINARY_WITHIN(float, bool);
    NOA_INSTANTIATE_EWISE_TRINARY_WITHIN(double, bool);

    #define NOA_INSTANTIATE_EWISE_TRINARY_CLAMP(T) \
    NOA_INSTANTIATE_EWISE_TRINARY(T,T,T,T,::noa::math::clamp_t)

    NOA_INSTANTIATE_EWISE_TRINARY_CLAMP(int16_t);
    NOA_INSTANTIATE_EWISE_TRINARY_CLAMP(int32_t);
    NOA_INSTANTIATE_EWISE_TRINARY_CLAMP(int64_t);
    NOA_INSTANTIATE_EWISE_TRINARY_CLAMP(uint16_t);
    NOA_INSTANTIATE_EWISE_TRINARY_CLAMP(uint32_t);
    NOA_INSTANTIATE_EWISE_TRINARY_CLAMP(uint64_t);
    NOA_INSTANTIATE_EWISE_TRINARY_CLAMP(half_t);
    NOA_INSTANTIATE_EWISE_TRINARY_CLAMP(float);
    NOA_INSTANTIATE_EWISE_TRINARY_CLAMP(double);

    #define NOA_INSTANTIATE_EWISE_TRINARY_ARITHMETIC(T)                     \
    NOA_INSTANTIATE_EWISE_TRINARY(T,T,T,T,::noa::math::plus_t);             \
    NOA_INSTANTIATE_EWISE_TRINARY(T,T,T,T,::noa::math::plus_minus_t);       \
    NOA_INSTANTIATE_EWISE_TRINARY(T,T,T,T,::noa::math::plus_multiply_t);    \
    NOA_INSTANTIATE_EWISE_TRINARY(T,T,T,T,::noa::math::plus_divide_t);      \
    NOA_INSTANTIATE_EWISE_TRINARY(T,T,T,T,::noa::math::minus_t);            \
    NOA_INSTANTIATE_EWISE_TRINARY(T,T,T,T,::noa::math::minus_plus_t);       \
    NOA_INSTANTIATE_EWISE_TRINARY(T,T,T,T,::noa::math::minus_multiply_t);   \
    NOA_INSTANTIATE_EWISE_TRINARY(T,T,T,T,::noa::math::minus_divide_t);     \
    NOA_INSTANTIATE_EWISE_TRINARY(T,T,T,T,::noa::math::multiply_t);         \
    NOA_INSTANTIATE_EWISE_TRINARY(T,T,T,T,::noa::math::multiply_plus_t);    \
    NOA_INSTANTIATE_EWISE_TRINARY(T,T,T,T,::noa::math::multiply_minus_t);   \
    NOA_INSTANTIATE_EWISE_TRINARY(T,T,T,T,::noa::math::multiply_divide_t);  \
    NOA_INSTANTIATE_EWISE_TRINARY(T,T,T,T,::noa::math::divide_t);           \
    NOA_INSTANTIATE_EWISE_TRINARY(T,T,T,T,::noa::math::divide_plus_t);      \
    NOA_INSTANTIATE_EWISE_TRINARY(T,T,T,T,::noa::math::divide_minus_t);     \
    NOA_INSTANTIATE_EWISE_TRINARY(T,T,T,T,::noa::math::divide_multiply_t);  \
    NOA_INSTANTIATE_EWISE_TRINARY(T,T,T,T,::noa::math::divide_epsilon_t)

    NOA_INSTANTIATE_EWISE_TRINARY_ARITHMETIC(half_t);
    NOA_INSTANTIATE_EWISE_TRINARY_ARITHMETIC(float);
    NOA_INSTANTIATE_EWISE_TRINARY_ARITHMETIC(double);
    NOA_INSTANTIATE_EWISE_TRINARY_ARITHMETIC(int16_t);
    NOA_INSTANTIATE_EWISE_TRINARY_ARITHMETIC(int32_t);
    NOA_INSTANTIATE_EWISE_TRINARY_ARITHMETIC(int64_t);
    NOA_INSTANTIATE_EWISE_TRINARY_ARITHMETIC(uint16_t);
    NOA_INSTANTIATE_EWISE_TRINARY_ARITHMETIC(uint32_t);
    NOA_INSTANTIATE_EWISE_TRINARY_ARITHMETIC(uint64_t);
}
