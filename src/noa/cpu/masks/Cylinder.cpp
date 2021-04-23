#include "noa/cpu/masks/Cylinder.h"

#include "noa/Math.h"
#include "noa/Profiler.h"

// Soft edges:
namespace {
    using namespace Noa;

    template<bool INVERT>
    float getSoftMask(float distance_xy_sqd, float radius_xy_sqd, float radius_xy, float radius_xy_sqd_with_taper,
                      float distance_z, float radius_z, float radius_z_with_taper, float taper_size) {
        constexpr float PI = Math::Constants<float>::PI;
        float mask_value;
        if constexpr (INVERT) {
            if (distance_z > radius_z_with_taper || distance_xy_sqd > radius_xy_sqd_with_taper) {
                mask_value = 1.f;
            } else {
                if (distance_xy_sqd <= radius_xy_sqd) {
                    mask_value = 1.f;
                } else {
                    float distance_xy = Math::sqrt(distance_xy_sqd);
                    mask_value = (1.f + Math::cos(PI * (distance_xy - radius_xy) / taper_size)) * 0.5f;
                }
                if (distance_z > radius_z)
                    mask_value *= (1.f + Math::cos(PI * (distance_z - radius_z) / taper_size)) * 0.5f;
                mask_value = 1 - mask_value;
            }
        } else {
            if (distance_z > radius_z_with_taper || distance_xy_sqd > radius_xy_sqd_with_taper) {
                mask_value = 0.f;
            } else {
                if (distance_xy_sqd <= radius_xy_sqd) {
                    mask_value = 1.f;
                } else {
                    float distance_xy = Math::sqrt(distance_xy_sqd);
                    mask_value = (1.f + Math::cos(PI * (distance_xy - radius_xy) / taper_size)) * 0.5f;
                }
                if (distance_z > radius_z)
                    mask_value *= (1.f + Math::cos(PI * (distance_z - radius_z) / taper_size)) * 0.5f;
            }
        }
        return mask_value;
    }

    template<bool INVERT, typename T>
    void cylinderSoft(T* inputs, T* outputs, size3_t shape, float3_t shifts, float radius_xy, float radius_z,
                      float taper_size, uint batches) {
        size_t elements = getElements(shape);
        float3_t center(shape / size_t{2});
        center += shifts;

        float radius_xy_sqd = radius_xy * radius_xy;
        float radius_xy_taper_sqd = Math::pow(radius_xy + taper_size, 2.f);
        float radius_z_taper = radius_z + taper_size;

        float distance_y_sqd, distance_xy_sqd, distance_z, mask_value;
        for (uint z = 0; z < shape.z; ++z) {
            distance_z = Math::abs(static_cast<float>(z) - center.z);
            for (uint y = 0; y < shape.y; ++y) {
                distance_y_sqd = Math::pow(static_cast<float>(y) - center.y, 2.f);
                size_t offset = (z * shape.y + y) * shape.x;
                for (uint x = 0; x < shape.x; ++x) {
                    distance_xy_sqd = Math::pow(static_cast<float>(x) - center.x, 2.f);
                    distance_xy_sqd += distance_y_sqd;
                    mask_value = getSoftMask<INVERT>(distance_xy_sqd, radius_xy_sqd, radius_xy, radius_xy_taper_sqd,
                                                     distance_z, radius_z, radius_z_taper, taper_size);
                    for (uint batch = 0; batch < batches; ++batch)
                        outputs[batch * elements + offset + x] =
                                inputs[batch * elements + offset + x] * static_cast<T>(mask_value);
                }
            }
        }
    }

    template<bool INVERT, typename T>
    void cylinderSoft(T* output_mask, size3_t shape, float3_t shifts,
                      float radius_xy, float radius_z, float taper_size) {
        float3_t center(shape / size_t{2});
        center += shifts;

        float radius_xy_sqd = radius_xy * radius_xy;
        float radius_xy_taper_sqd = Math::pow(radius_xy + taper_size, 2.f);
        float radius_z_taper = radius_z + taper_size;

        float distance_y_sqd, distance_xy_sqd, distance_z, mask_value;
        for (uint z = 0; z < shape.z; ++z) {
            distance_z = Math::abs(static_cast<float>(z) - center.z);
            for (uint y = 0; y < shape.y; ++y) {
                distance_y_sqd = Math::pow(static_cast<float>(y) - center.y, 2.f);
                size_t offset = (z * shape.y + y) * shape.x;
                for (uint x = 0; x < shape.x; ++x) {
                    distance_xy_sqd = Math::pow(static_cast<float>(x) - center.x, 2.f);
                    distance_xy_sqd += distance_y_sqd;
                    mask_value = getSoftMask<INVERT>(distance_xy_sqd, radius_xy_sqd, radius_xy, radius_xy_taper_sqd,
                                                     distance_z, radius_z, radius_z_taper, taper_size);
                    output_mask[offset + x] = static_cast<T>(mask_value);
                }
            }
        }
    }
}

// Hard edges:
namespace {
    using namespace Noa;

    template<bool INVERT>
    float getHardMask(float distance_xy_sqd, float radius_xy_sqd, float distance_z, float radius_z) {
        float mask_value;
        if constexpr (INVERT) {
            if (distance_z > radius_z || distance_xy_sqd > radius_xy_sqd)
                mask_value = 1.f;
            else
                mask_value = 0.f;
        } else {
            if (distance_z > radius_z || distance_xy_sqd > radius_xy_sqd)
                mask_value = 0.f;
            else
                mask_value = 1.f;
        }
        return mask_value;
    }

    template<bool INVERT, typename T>
    void cylinderHard(T* inputs, T* outputs, size3_t shape, float3_t shifts,
                      float radius_xy, float radius_z, uint batches) {
        size_t elements = getElements(shape);
        float3_t center(shape / size_t{2});
        center += shifts;

        float radius_xy_sqd = radius_xy * radius_xy;

        // Compute the mask using single precision, even if T is double.
        float distance_y_sqd, distance_xy_sqd, distance_z, mask_value;
        for (uint z = 0; z < shape.z; ++z) {
            distance_z = Math::abs(static_cast<float>(z) - center.z);
            for (uint y = 0; y < shape.y; ++y) {
                distance_y_sqd = Math::pow(static_cast<float>(y) - center.y, 2.f);
                size_t offset = (z * shape.y + y) * shape.x;
                for (uint x = 0; x < shape.x; ++x) {
                    distance_xy_sqd = Math::pow(static_cast<float>(x) - center.x, 2.f);
                    distance_xy_sqd += distance_y_sqd;
                    mask_value = getHardMask<INVERT>(distance_xy_sqd, radius_xy_sqd, distance_z, radius_z);
                    for (uint batch = 0; batch < batches; ++batch)
                        outputs[batch * elements + offset + x] =
                                inputs[batch * elements + offset + x] * static_cast<T>(mask_value);
                }
            }
        }
    }

    template<bool INVERT, typename T>
    void cylinderHard(T* output_mask, size3_t shape, float3_t shifts, float radius_xy, float radius_z) {
        float3_t center(shape / size_t{2});
        center += shifts;

        float radius_xy_sqd = radius_xy * radius_xy;
        float distance_y_sqd, distance_xy_sqd, distance_z, mask_value;
        for (uint z = 0; z < shape.z; ++z) {
            distance_z = Math::abs(static_cast<float>(z) - center.z);
            for (uint y = 0; y < shape.y; ++y) {
                distance_y_sqd = Math::pow(static_cast<float>(y) - center.y, 2.f);
                size_t offset = (z * shape.y + y) * shape.x;
                for (uint x = 0; x < shape.x; ++x) {
                    distance_xy_sqd = Math::pow(static_cast<float>(x) - center.x, 2.f);
                    distance_xy_sqd += distance_y_sqd;
                    mask_value = getHardMask<INVERT>(distance_xy_sqd, radius_xy_sqd, distance_z, radius_z);
                    output_mask[offset + x] = static_cast<T>(mask_value);
                }
            }
        }
    }
}

// Definitions & Instantiations:
namespace Noa::Mask {
    template<bool INVERT, typename T>
    void cylinder(T* inputs, T* outputs, size3_t shape,
                  float3_t shifts, float radius_xy, float radius_z, float taper_size, uint batches) {
        NOA_PROFILE_FUNCTION();
        if (taper_size > 1e-5f)
            cylinderSoft<INVERT>(inputs, outputs, shape, shifts, radius_xy, radius_z, taper_size, batches);
        else
            cylinderHard<INVERT>(inputs, outputs, shape, shifts, radius_xy, radius_z, batches);
    }

    template<bool INVERT, typename T>
    void cylinder(T* output_mask, size3_t shape, float3_t shifts, float radius_xy, float radius_z, float taper_size) {
        NOA_PROFILE_FUNCTION();
        if (taper_size > 1e-5f)
            cylinderSoft<INVERT>(output_mask, shape, shifts, radius_xy, radius_z, taper_size);
        else
            cylinderHard<INVERT>(output_mask, shape, shifts, radius_xy, radius_z);
    }

    #define INSTANTIATE_CYLINDER(T)                                                         \
    template void cylinder<true, T>(T*, T*, size3_t, float3_t, float, float, float, uint);  \
    template void cylinder<false, T>(T*, T*, size3_t, float3_t, float, float, float, uint); \
    template void cylinder<true, T>(T*, size3_t, float3_t, float, float, float);            \
    template void cylinder<false, T>(T*, size3_t, float3_t, float, float, float)

    INSTANTIATE_CYLINDER(float);
    INSTANTIATE_CYLINDER(double);
}
