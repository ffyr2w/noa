#pragma once

#include "noa/core/Enums.hpp"
#include "noa/core/fft/Frequency.hpp"
#include "noa/core/signal/StandardizeIFFT.hpp"
#include "noa/unified/fft/Transform.hpp"
#include "noa/unified/ReduceAxesEwise.hpp"
#include "noa/unified/Factory.hpp"

namespace noa::signal {
    /// Standardizes (mean=0, stddev=1) a real-space signal, by modifying its Fourier coefficients.
    /// \tparam REMAP       Remapping operator. Should be H2H, HC2HC, F2F or FC2FC.
    /// \param[in] input    Input (r)FFT.
    /// \param[out] output  Output (r)FFT. Can be equal to \p input.
    ///                     The c2r transform of \p output has its mean set to 0 and its stddev set to 1.
    /// \param shape        BDHW logical shape of \p input and \p output.
    /// \param norm         Normalization mode of \p input.
    template<Remap REMAP,
             nt::readable_varray_decay_of_complex Input,
             nt::writable_varray_decay_of_complex Output>
    requires (REMAP.is_any(Remap::H2H, Remap::HC2HC, Remap::F2F, Remap::FC2FC))
    void standardize_ifft(
        const Input& input,
        const Output& output,
        const Shape4<i64>& shape,
        noa::fft::Norm norm = noa::fft::NORM_DEFAULT
    ) {
        using Norm = noa::fft::Norm;

        constexpr bool is_full = REMAP == Remap::F2F or REMAP == Remap::FC2FC;
        constexpr bool is_centered = REMAP == Remap::FC2FC or REMAP == Remap::HC2HC;
        const auto actual_shape = is_full ? shape : shape.rfft();

        check(not input.is_empty() and not output.is_empty(), "Empty array detected");
        check(all(input.shape() == actual_shape) and all(output.shape() == actual_shape),
              "The input {} and output {} {}redundant FFTs don't match the expected shape {}",
              input.shape(), output.shape(), is_full ? "" : "non-", actual_shape);
        check(input.device() == output.device(),
              "The input and output arrays must be on the same device, but got input:device={}, output:device={}",
              input.device(), output.device());

        using real_t = nt::value_type_twice_t<Output>;
        using complex_t = Complex<real_t>;
        const auto count = static_cast<f64>(shape.pop_front().n_elements());
        const auto scale = norm == Norm::FORWARD ? 1 : norm == Norm::ORTHO ? sqrt(count) : count;
        const auto options = ArrayOption{input.device(), Allocator::DEFAULT_ASYNC};

        auto dc_position = ni::make_subregion<4>(
            ni::FullExtent{},
            is_centered ? noa::fft::fftshift(i64{}, shape[1]) : 0,
            is_centered ? noa::fft::fftshift(i64{}, shape[2]) : 0,
            is_centered and is_full ? noa::fft::fftshift(i64{}, shape[3]) : 0);

        if constexpr (REMAP == Remap::F2F or REMAP == Remap::FC2FC) {
            // Compute the energy of the input (excluding the dc).
            auto dc_components = input.view().subregion(dc_position);
            auto energies = Array<real_t>(dc_components.shape(), options);
            reduce_axes_ewise(
                input.view(), real_t{}, wrap(energies.view(), dc_components),
                guts::FFTSpectrumEnergy{scale});

            // Standardize.
            ewise(wrap(input, std::move(energies)), output.view(), Multiply{});
            fill(output.subregion(dc_position), {});

        } else if constexpr (REMAP == Remap::H2H or REMAP == Remap::HC2HC) {
            const bool is_even = noa::is_even(shape[3]);

            auto energies = zeros<real_t>({shape[0], 3, 1, 1}, options);
            auto energies_0 = energies.view().subregion(ni::FullExtent{}, 0, 0, 0);
            auto energies_1 = energies.view().subregion(ni::FullExtent{}, 1, 0, 0);
            auto energies_2 = energies.view().subregion(ni::FullExtent{}, 2, 0, 0);

            // Reduce unique chunk:
            reduce_axes_ewise(input.view().subregion(ni::Ellipsis{}, ni::Slice{1, input.shape()[3] - is_even}),
                              real_t{}, energies_0, guts::rFFTSpectrumEnergy{});

            // Reduce common column/plane containing the DC:
            reduce_axes_ewise(input.view().subregion(ni::Ellipsis{}, 0),
                              real_t{}, energies_1, guts::rFFTSpectrumEnergy{});

            if (is_even) {
                // Reduce common column/plane containing the real Nyquist:
                reduce_axes_ewise(input.view().subregion(ni::Ellipsis{}, -1),
                                  real_t{}, energies_2, guts::rFFTSpectrumEnergy{});
            }

            // Standardize.
            ewise(wrap(input.view().subregion(dc_position), energies_1, energies_2), energies_0,
                  guts::CombineSpectrumEnergies<complex_t, real_t>{static_cast<real_t>(scale)}); // if batch=1, this is one single value...
            ewise(wrap(input, energies.subregion(ni::FullExtent{}, 0, 0, 0)), output.view(), Multiply{});
            fill(output.subregion(dc_position), {});

        } else {
            static_assert(nt::always_false<>);
        }
    }
}
