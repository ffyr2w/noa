#include "noa/cpu/fourier/Plan.h"

namespace noa::fourier::details {
    std::mutex Mutex::mutex;

    int getThreads(size3_t shape, uint batches, int rank) {
        double geom_size;
        if (rank == 1)
            geom_size = (math::sqrt(static_cast<double>(shape.x) * batches) + batches) / 2.;
        else
            geom_size = math::pow(static_cast<double>(getElements(shape)), 1. / rank);
        int threads = static_cast<int>((math::log(geom_size) / math::log(2.) - 5.95) * 2.);
        return math::clamp(threads, 1, all(getNiceShape(shape) == shape) ? 8 : 4);
    }
}

namespace noa::fourier {
    bool Plan<float>::is_initialized{false};
    int Plan<float>::max_threads{};
    bool Plan<double>::is_initialized{false};
    int Plan<double>::max_threads{};
}
