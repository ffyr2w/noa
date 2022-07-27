#include "noa/cpu/geometry/fft/Polar.h"
#include "noa/cpu/geometry/Interpolator.h"

namespace {
    using namespace ::noa;

    template<typename T, InterpMode INTERP>
    void cartesian2polar_(const T* cartesian, size3_t cartesian_strides, size3_t cartesian_shape,
                          T* polar, size3_t polar_strides, size3_t polar_shape,
                          float2_t frequency_range, float2_t angle_range,
                          bool log, size_t threads) {
        using real_t = traits::value_type_t<T>;
        const size_t offset = cartesian_shape[0] == 1 ? 0 : cartesian_strides[0];
        const size2_t strides(cartesian_strides.get(1));
        const size2_t shape(cartesian_shape.get(1));
        const cpu::geometry::Interpolator2D<T> interp(cartesian, strides, shape.fft(), 0);

        // Since the aspect ratio of the cartesian transform might not be 1,
        // each axis has should have its own magnitude.
        const float2_t half_shape(shape / 2);
        const float2_t radius_y_range(frequency_range * 2 * half_shape[0]);
        const float2_t radius_x_range(frequency_range * 2 * half_shape[1]);
        const float2_t size{polar_shape[1] - 1, polar_shape[2] - 1};

        const auto step_angle = (angle_range[1] - angle_range[0]) / size[0];
        float step_radius_y, step_radius_x;
        if (log) {
            step_radius_y = math::log(radius_y_range[1] - radius_y_range[0]) / size[1];
            step_radius_x = math::log(radius_x_range[1] - radius_x_range[0]) / size[1];
        } else {
            step_radius_y = (radius_y_range[1] - radius_y_range[0]) / size[1];
            step_radius_x = (radius_x_range[1] - radius_x_range[0]) / size[1];
        }

        const float start_angle = angle_range[0];
        const float2_t start_radius{radius_y_range[0], radius_x_range[0]};
        const float center = half_shape[0];

        #pragma omp parallel for collapse(3) default(none) num_threads(threads)  \
        shared(polar, polar_strides, polar_shape, log, interp, offset,           \
               step_angle, step_radius_y, step_radius_x, start_angle, start_radius, center)

        for (size_t batch = 0; batch < polar_shape[0]; ++batch) {
            for (size_t phi = 0; phi < polar_shape[1]; ++phi) {
                for (size_t rho = 0; rho < polar_shape[2]; ++rho) {

                    const float2_t polar_coordinate{phi, rho};
                    const float angle_rad = polar_coordinate[0] * step_angle + start_angle;
                    float radius_y, radius_x;
                    if (log) {
                        radius_y = math::exp(polar_coordinate[1] * step_radius_y) - 1 + start_radius[0];
                        radius_x = math::exp(polar_coordinate[1] * step_radius_x) - 1 + start_radius[1];
                    } else {
                        radius_y = polar_coordinate[1] * step_radius_y + start_radius[0];
                        radius_x = polar_coordinate[1] * step_radius_x + start_radius[1];
                    }

                    float2_t cartesian_coordinates{radius_y * math::sin(angle_rad),
                                                   radius_x * math::cos(angle_rad)};
                    [[maybe_unused]] real_t conj = 1;
                    if (cartesian_coordinates[1] < 0) {
                        cartesian_coordinates = -cartesian_coordinates;
                        if constexpr (traits::is_complex_v<T>)
                            conj = -1;
                    }
                    cartesian_coordinates[0] += center; // center_x = 0

                    T value = interp.template get<INTERP, BORDER_ZERO>(cartesian_coordinates, offset * batch);
                    if constexpr (traits::is_complex_v<T>)
                        value.imag *= conj;

                    polar[indexing::at(batch, phi, rho, polar_strides)] = value;
                }
            }
        }
    }
}

namespace noa::cpu::geometry::fft {
    template<Remap, typename T, typename>
    void cartesian2polar(const shared_t<T[]>& cartesian, size4_t cartesian_strides, size4_t cartesian_shape,
                         const shared_t<T[]>& polar, size4_t polar_strides, size4_t polar_shape,
                         float2_t frequency_range, float2_t angle_range,
                         bool log, InterpMode interp, Stream& stream) {
        NOA_ASSERT(cartesian.get() != polar.get());
        NOA_ASSERT(cartesian_shape[1] == 1 && polar_shape[1] == 1);
        NOA_ASSERT(cartesian_shape[0] == 1 || cartesian_shape[0] == polar_shape[0]);

        const size3_t src_shape{cartesian_shape[0], cartesian_shape[2], cartesian_shape[3]};
        const size3_t src_strides{cartesian_strides[0], cartesian_strides[2], cartesian_strides[3]};
        const size3_t dst_shape{polar_shape[0], polar_shape[2], polar_shape[3]};
        const size3_t dst_strides{polar_strides[0], polar_strides[2], polar_strides[3]};

        const size_t threads = stream.threads();
        switch (interp) {
            case INTERP_NEAREST:
                return stream.enqueue([=]() {
                    cartesian2polar_<T, INTERP_NEAREST>(
                            cartesian.get(), src_strides, src_shape, polar.get(), dst_strides, dst_shape,
                            frequency_range, angle_range, log, threads);
                });
            case INTERP_LINEAR_FAST:
            case INTERP_LINEAR:
                return stream.enqueue([=]() {
                    cartesian2polar_<T, INTERP_LINEAR>(
                            cartesian.get(), src_strides, src_shape, polar.get(), dst_strides, dst_shape,
                            frequency_range, angle_range, log, threads);
                });
            case INTERP_COSINE_FAST:
            case INTERP_COSINE:
                return stream.enqueue([=]() {
                    cartesian2polar_<T, INTERP_COSINE>(
                            cartesian.get(), src_strides, src_shape, polar.get(), dst_strides, dst_shape,
                            frequency_range, angle_range, log, threads);
                });
            case INTERP_CUBIC:
                return stream.enqueue([=]() {
                    cartesian2polar_<T, INTERP_CUBIC>(
                            cartesian.get(), src_strides, src_shape, polar.get(), dst_strides, dst_shape,
                            frequency_range, angle_range, log, threads);
                });
            case INTERP_CUBIC_BSPLINE_FAST:
            case INTERP_CUBIC_BSPLINE:
                NOA_THROW("{} is not supported", interp);
        }
    }

    #define INSTANTIATE_POLAR(T) \
    template void cartesian2polar<Remap::HC2FC, T, void>(const shared_t<T[]>&, size4_t, size4_t, const shared_t<T[]>&, size4_t, size4_t, float2_t, float2_t, bool, InterpMode, Stream&)

    INSTANTIATE_POLAR(float);
    INSTANTIATE_POLAR(double);
    INSTANTIATE_POLAR(cfloat_t);
    INSTANTIATE_POLAR(cdouble_t);
}
