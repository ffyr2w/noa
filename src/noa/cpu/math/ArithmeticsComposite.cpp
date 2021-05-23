#include <algorithm>
#include <execution>

#include "noa/Profiler.h"
#include "noa/cpu/math/ArithmeticsComposite.h"

namespace Noa::Math {
    template<typename T>
    void multiplyAddArray(T* inputs, T* multipliers, T* addends, T* outputs, size_t elements, uint batches) {
        NOA_PROFILE_FUNCTION();
        for (uint batch{0}; batch < batches; ++batch)
            for (size_t idx{0}; idx < elements; ++idx)
                outputs[batch * elements + idx] = inputs[batch * elements + idx] * multipliers[idx] + addends[idx];
    }

    template<typename T>
    void squaredDistanceFromValue(T* inputs, T* values, T* outputs, size_t elements, uint batches) {
        NOA_PROFILE_FUNCTION();
        for (uint batch = 0; batch < batches; ++batch) {
            T& value = values[batch];
            size_t batch_offset = elements * static_cast<size_t>(batch);
            std::transform(std::execution::par_unseq,
                           inputs + batch_offset, inputs + batch_offset + elements, outputs + batch_offset,
                           [value](T a) -> T {
                               T distance = a - value;
                               return distance * distance;
                           });
        }
    }

    template<typename T>
    void squaredDistanceFromArray(T* inputs, T* array, T* outputs, size_t elements, uint batches) {
        NOA_PROFILE_FUNCTION();
        for (uint batch = 0; batch < batches; ++batch) {
            size_t batch_offset = elements * static_cast<size_t>(batch);
            std::transform(std::execution::par_unseq,
                           inputs + batch_offset, inputs + batch_offset + elements, array, outputs + batch_offset,
                           [](T a, T b) {
                               T distance = a - b;
                               return distance * distance;
                           });
        }
    }

    #define INSTANTIATE_ARITHMETICS_COMPOSITE(T)                            \
    template void multiplyAddArray<T>(T*, T*, T*, T*, size_t, uint);        \
    template void squaredDistanceFromValue<T>(T*, T*, T*, size_t, uint);    \
    template void squaredDistanceFromArray<T>(T*, T*, T*, size_t, uint)

    INSTANTIATE_ARITHMETICS_COMPOSITE(int);
    INSTANTIATE_ARITHMETICS_COMPOSITE(uint);
    INSTANTIATE_ARITHMETICS_COMPOSITE(float);
    INSTANTIATE_ARITHMETICS_COMPOSITE(double);
}
