#pragma once

#include <cufft.h>

#include "noa/Definitions.h"
#include "noa/gpu/cuda/Types.h"
#include "noa/gpu/cuda/Exception.h"
#include "noa/gpu/cuda/util/Stream.h"
#include "noa/gpu/cuda/fourier/Plan.h"

namespace Noa::CUDA::Fourier {
    enum : int { FORWARD = CUFFT_FORWARD, BACKWARD = CUFFT_INVERSE };

    /**
     * Computes the R2C transform (i.e forward transform) using the @a plan.
     * @param[in] input     Should match the layout (shape, etc.) used to create @a plan.
     * @param[out] output   Should match the layout (shape, etc.) used to create @a plan.
     * @param plan          Existing plan.
     * @warning This functions is asynchronous with respect to the host. All operations are enqueued to the
     *          stream associated with the @a plan.
     */
    NOA_IH void R2C(float* input, cfloat_t* output, const Plan<float>& plan) {
        NOA_THROW_IF(cufftExecR2C(plan.get(), input, reinterpret_cast<cufftComplex*>(output)));
    }

    NOA_IH void R2C(double* input, cdouble_t* output, const Plan<double>& plan) {
        NOA_THROW_IF(cufftExecD2Z(plan.get(), input, reinterpret_cast<cufftDoubleComplex*>(output)));
    }

    /**
     * Computes the C2R transform (i.e backward transform) using the @a plan.
     * @param[in] input     Should match the output used to create @a plan.
     * @param[out] output   Should match the output used to create @a plan.
     * @param plan          Existing plan.
     * @warning This functions is asynchronous with respect to the host. All operations are enqueued to the
     *          stream associated with the @a plan.
     */
    NOA_IH void C2R(cfloat_t* input, float* output, const Plan<float>& plan) {
        NOA_THROW_IF(cufftExecC2R(plan.get(), reinterpret_cast<cufftComplex*>(input), output));
    }

    NOA_IH void C2R(cdouble_t* input, double* output, const Plan<double>& plan) {
        NOA_THROW_IF(cufftExecZ2D(plan.get(), reinterpret_cast<cufftDoubleComplex*>(input), output));
    }

    /**
     * Computes the C2C transform using the @a plan.
     * @param[in] input     Should match the output used to create @a plan.
     * @param[out] output   Should match the output used to create @a plan.
     * @param plan          Existing plan.
     * @param sign          Sign of the exponent in the formula that defines the Fourier transform.
         *                  It can be −1 (@c FORWARD) or +1 (@c BACKWARD).
     * @warning This functions is asynchronous with respect to the host. All operations are enqueued to the
     *          stream associated with the @a plan.
     */
    NOA_IH void C2C(cfloat_t* input, cfloat_t* output, const Plan<float>& plan, int sign) {
        NOA_THROW_IF(cufftExecC2C(plan.get(),
                                  reinterpret_cast<cufftComplex*>(input),
                                  reinterpret_cast<cufftComplex*>(output), sign));
    }

    NOA_IH void C2C(cdouble_t* input, cdouble_t* output, const Plan<double>& plan, int sign) {
        NOA_THROW_IF(cufftExecZ2Z(plan.get(),
                                  reinterpret_cast<cufftDoubleComplex*>(input),
                                  reinterpret_cast<cufftDoubleComplex*>(output), sign));
    }

    /* ----------------------------- */
    /* --- "One time" transforms --- */
    /* ----------------------------- */

    /**
     * Computes the R2C transform (i.e forward transform).
     * @see Fourier::Plan<float> for more details.
     * @note @a input and @a output can be the same, which will trigger an in-place transform.
     * @note This function runs synchronously. The transform is enqueued to the default stream and
     *       synchronises the stream before exiting.
     */
    NOA_IH void R2C(float* input, cfloat_t* output, size3_t shape, uint batch) {
        Plan<float> fast_plan(shape, batch, Fourier::Type::R2C);
        R2C(input, output, fast_plan);
        Stream::synchronize(Stream());
    }

    NOA_IH void R2C(double* input, cdouble_t* output, size3_t shape, uint batch) {
        Plan<double> fast_plan(shape, batch, Fourier::Type::R2C);
        R2C(input, output, fast_plan);
        Stream::synchronize(Stream());
    }

    /**
     * Computes the C2R transform (i.e backward transform).
     * @see Fourier::Plan<float> for more details.
     * @note @a input and @a output can be the same, which will trigger an in-place transform.
     * @note This function runs synchronously. The transform is enqueued to the default stream and
     *       synchronises the stream before exiting.
     */
    NOA_IH void C2R(cfloat_t* input, float* output, size3_t shape, uint batch) {
        Plan<float> fast_plan(shape, batch, Fourier::Type::C2R);
        C2R(input, output, fast_plan);
        Stream::synchronize(Stream());
    }

    NOA_IH void C2R(cdouble_t* input, double* output, size3_t shape, uint batch) {
        Plan<double> fast_plan(shape, batch, Fourier::Type::C2R);
        C2R(input, output, fast_plan);
        Stream::synchronize(Stream());
    }

    /**
     * Computes the C2C transform, either forward or backward depending on @a sign.
     * @see Fourier::Plan<float> for more details.
     * @note @a input and @a output can be the same, which will trigger an in-place transform.
     * @note This function runs synchronously. The transform is enqueued to the default stream and
     *       synchronises the stream before exiting.
     */
    NOA_IH void C2C(cfloat_t* input, cfloat_t* output, size3_t shape, uint batch, int sign) {
        Plan<float> fast_plan(shape, batch, Fourier::Type::C2C);
        C2C(input, output, fast_plan, sign);
        Stream::synchronize(Stream());
    }

    NOA_IH void C2C(cdouble_t* input, cdouble_t* output, size3_t shape, uint batch, int sign) {
        Plan<double> fast_plan(shape, batch, Fourier::Type::C2C);
        C2C(input, output, fast_plan, sign);
        Stream::synchronize(Stream());
    }
}
