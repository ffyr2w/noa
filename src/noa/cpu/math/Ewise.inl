#pragma once

#ifndef NOA_EWISE_INL_
#error "This is an internal header. Include the corresponding .h file instead"
#endif

// TODO OpenMP support? I don't want to add #pragma omp parallel for everywhere since it prevents constant-evaluated
//      expression to be moved to outer-loops. Benchmarking will be necessary.

namespace noa::cpu::math {
    template<typename T, typename U, typename UnaryOp>
    void ewise(const shared_t<T[]>& input, dim4_t input_strides,
               const shared_t<U[]>& output, dim4_t output_strides,
               dim4_t shape, UnaryOp unary_op, Stream& stream) {
        NOA_ASSERT(input && output && all(shape > 0));
        stream.enqueue([=]() mutable {
            const dim4_t order = indexing::order(output_strides, shape);
            input_strides = indexing::reorder(input_strides, order);
            output_strides = indexing::reorder(output_strides, order);
            shape = indexing::reorder(shape, order);

            const T* iptr = input.get();
            U* optr = output.get();
            for (dim_t i = 0; i < shape[0]; ++i)
                for (dim_t j = 0; j < shape[1]; ++j)
                    for (dim_t k = 0; k < shape[2]; ++k)
                        for (dim_t l = 0; l < shape[3]; ++l)
                            optr[indexing::at(i, j, k, l, output_strides)] =
                                    static_cast<U>(unary_op(iptr[indexing::at(i, j, k, l, input_strides)]));
        });
    }

    template<typename T, typename U, typename V, typename BinaryOp, typename>
    void ewise(const shared_t<T[]>& lhs, dim4_t lhs_strides, U rhs,
               const shared_t<V[]>& output, dim4_t output_strides,
               dim4_t shape, BinaryOp binary_op, Stream& stream) {
        NOA_ASSERT(lhs && output && all(shape > 0));
        stream.enqueue([=]() mutable {
            const dim4_t order = indexing::order(output_strides, shape);
            lhs_strides = indexing::reorder(lhs_strides, order);
            output_strides = indexing::reorder(output_strides, order);
            shape = indexing::reorder(shape, order);

            const T* lptr = lhs.get();
            V* optr = output.get();
            for (dim_t i = 0; i < shape[0]; ++i)
                for (dim_t j = 0; j < shape[1]; ++j)
                    for (dim_t k = 0; k < shape[2]; ++k)
                        for (dim_t l = 0; l < shape[3]; ++l)
                            optr[indexing::at(i, j, k, l, output_strides)] =
                                    static_cast<V>(binary_op(lptr[indexing::at(i, j, k, l, lhs_strides)], rhs));
        });
    }

    template<typename T, typename U, typename V, typename BinaryOp, typename>
    void ewise(T lhs, const shared_t<U[]>& rhs, dim4_t rhs_strides,
               const shared_t<V[]>& output, dim4_t output_strides,
               dim4_t shape, BinaryOp binary_op, Stream& stream) {
        NOA_ASSERT(rhs && output && all(shape > 0));
        stream.enqueue([=]() mutable {
            const dim4_t order = indexing::order(output_strides, shape);
            rhs_strides = indexing::reorder(rhs_strides, order);
            output_strides = indexing::reorder(output_strides, order);
            shape = indexing::reorder(shape, order);

            const U* rptr = rhs.get();
            V* optr = output.get();
            for (dim_t i = 0; i < shape[0]; ++i)
                for (dim_t j = 0; j < shape[1]; ++j)
                    for (dim_t k = 0; k < shape[2]; ++k)
                        for (dim_t l = 0; l < shape[3]; ++l)
                            optr[indexing::at(i, j, k, l, output_strides)] =
                                    static_cast<V>(binary_op(lhs, rptr[indexing::at(i, j, k, l, rhs_strides)]));
        });
    }

    template<typename T, typename U, typename V, typename BinaryOp>
    void ewise(const shared_t<T[]>& lhs, dim4_t lhs_strides,
               const shared_t<U[]>& rhs, dim4_t rhs_strides,
               const shared_t<V[]>& output, dim4_t output_strides,
               dim4_t shape, BinaryOp binary_op, Stream& stream) {
        NOA_ASSERT(lhs && rhs && output && all(shape > 0));
        stream.enqueue([=]() mutable {
            const dim4_t order = indexing::order(output_strides, shape);
            lhs_strides = indexing::reorder(lhs_strides, order);
            rhs_strides = indexing::reorder(rhs_strides, order);
            output_strides = indexing::reorder(output_strides, order);
            shape = indexing::reorder(shape, order);

            const T* lptr = lhs.get();
            const U* rptr = rhs.get();
            V* optr = output.get();
            for (dim_t i = 0; i < shape[0]; ++i)
                for (dim_t j = 0; j < shape[1]; ++j)
                    for (dim_t k = 0; k < shape[2]; ++k)
                        for (dim_t l = 0; l < shape[3]; ++l)
                            optr[indexing::at(i, j, k, l, output_strides)] =
                                    static_cast<V>(binary_op(lptr[indexing::at(i, j, k, l, lhs_strides)],
                                                             rptr[indexing::at(i, j, k, l, rhs_strides)]));
        });
    }

    template<typename T, typename U, typename V, typename W, typename TrinaryOp, typename>
    void ewise(const shared_t<T[]>& lhs, dim4_t lhs_strides, U mhs, V rhs,
               const shared_t<W[]>& output, dim4_t output_strides,
               dim4_t shape, TrinaryOp trinary_op, Stream& stream) {
        NOA_ASSERT(lhs && output && all(shape > 0));
        stream.enqueue([=]() mutable {
            const dim4_t order = indexing::order(output_strides, shape);
            lhs_strides = indexing::reorder(lhs_strides, order);
            output_strides = indexing::reorder(output_strides, order);
            shape = indexing::reorder(shape, order);

            const T* lptr = lhs.get();
            W* optr = output.get();
                for (dim_t i = 0; i < shape[0]; ++i)
                    for (dim_t j = 0; j < shape[1]; ++j)
                        for (dim_t k = 0; k < shape[2]; ++k)
                            for (dim_t l = 0; l < shape[3]; ++l)
                                optr[indexing::at(i, j, k, l, output_strides)] =
                                        static_cast<W>(trinary_op(lptr[indexing::at(i, j, k, l, lhs_strides)],
                                                                  mhs, rhs));
        });
    }

    template<typename T, typename U, typename V, typename W, typename TrinaryOp, typename>
    void ewise(const shared_t<T[]>& lhs, dim4_t lhs_strides,
               const shared_t<U[]>& mhs, dim4_t mhs_strides, V rhs,
               const shared_t<W[]>& output, dim4_t output_strides,
               dim4_t shape, TrinaryOp trinary_op, Stream& stream) {
        NOA_ASSERT(lhs && mhs && output && all(shape > 0));
        stream.enqueue([=]() mutable {
            const dim4_t order = indexing::order(output_strides, shape);
            lhs_strides = indexing::reorder(lhs_strides, order);
            mhs_strides = indexing::reorder(mhs_strides, order);
            output_strides = indexing::reorder(output_strides, order);
            shape = indexing::reorder(shape, order);

            const T* lptr = lhs.get();
            const U* mptr = mhs.get();
            W* optr = output.get();
            for (dim_t i = 0; i < shape[0]; ++i)
                for (dim_t j = 0; j < shape[1]; ++j)
                    for (dim_t k = 0; k < shape[2]; ++k)
                        for (dim_t l = 0; l < shape[3]; ++l)
                            optr[indexing::at(i, j, k, l, output_strides)] =
                                    static_cast<W>(trinary_op(lptr[indexing::at(i, j, k, l, lhs_strides)],
                                                              mptr[indexing::at(i, j, k, l, mhs_strides)],
                                                              rhs));
        });
    }

    template<typename T, typename U, typename V, typename W, typename TrinaryOp, typename>
    void ewise(const shared_t<T[]>& lhs, dim4_t lhs_strides, U mhs,
               const shared_t<V[]>& rhs, dim4_t rhs_strides,
               const shared_t<W[]>& output, dim4_t output_strides,
               dim4_t shape, TrinaryOp trinary_op, Stream& stream) {
        NOA_ASSERT(lhs && rhs && output && all(shape > 0));
        stream.enqueue([=]() mutable {
            const dim4_t order = indexing::order(output_strides, shape);
            lhs_strides = indexing::reorder(lhs_strides, order);
            rhs_strides = indexing::reorder(rhs_strides, order);
            output_strides = indexing::reorder(output_strides, order);
            shape = indexing::reorder(shape, order);

            const T* lptr = lhs.get();
            const V* rptr = rhs.get();
            W* optr = output.get();
            for (dim_t i = 0; i < shape[0]; ++i)
                for (dim_t j = 0; j < shape[1]; ++j)
                    for (dim_t k = 0; k < shape[2]; ++k)
                        for (dim_t l = 0; l < shape[3]; ++l)
                            optr[indexing::at(i, j, k, l, output_strides)] =
                                    static_cast<W>(trinary_op(lptr[indexing::at(i, j, k, l, lhs_strides)],
                                                              mhs,
                                                              rptr[indexing::at(i, j, k, l, rhs_strides)]));
        });
    }

    template<typename T, typename U, typename V, typename W, typename TrinaryOp>
    void ewise(const shared_t<T[]>& lhs, dim4_t lhs_strides,
               const shared_t<U[]>& mhs, dim4_t mhs_strides,
               const shared_t<V[]>& rhs, dim4_t rhs_strides,
               const shared_t<W[]>& output, dim4_t output_strides,
               dim4_t shape, TrinaryOp trinary_op, Stream& stream) {
        NOA_ASSERT(lhs && mhs && rhs && output && all(shape > 0));
        stream.enqueue([=]() mutable {
            const dim4_t order = indexing::order(output_strides, shape);
            lhs_strides = indexing::reorder(lhs_strides, order);
            mhs_strides = indexing::reorder(mhs_strides, order);
            rhs_strides = indexing::reorder(rhs_strides, order);
            output_strides = indexing::reorder(output_strides, order);
            shape = indexing::reorder(shape, order);

            const T* lptr = lhs.get();
            const U* mptr = mhs.get();
            const V* rptr = rhs.get();
            W* optr = output.get();
            for (dim_t i = 0; i < shape[0]; ++i)
                for (dim_t j = 0; j < shape[1]; ++j)
                    for (dim_t k = 0; k < shape[2]; ++k)
                        for (dim_t l = 0; l < shape[3]; ++l)
                            optr[indexing::at(i, j, k, l, output_strides)] =
                                    static_cast<W>(trinary_op(lptr[indexing::at(i, j, k, l, lhs_strides)],
                                                              mptr[indexing::at(i, j, k, l, mhs_strides)],
                                                              rptr[indexing::at(i, j, k, l, rhs_strides)]));
        });
    }
}
