#pragma once

#include "noa/core/Config.hpp"
#include "noa/core/Traits.hpp"
#include "noa/core/math/Comparison.hpp"
#include "noa/core/math/Generic.hpp"
#include "noa/core/types/Vec.hpp"
#include "noa/core/types/Tuple.hpp"

namespace noa::inline types {
    template<typename T, size_t R, size_t C>
    class alignas(16) Mat;
}

namespace noa {
    template<typename T, size_t R, size_t C>
    NOA_HD constexpr auto transpose(const Mat<T, R, C>& m) noexcept -> Mat<T, C, R> {
        return [&]<size_t... I>(std::index_sequence<I...>) {
            return Mat<T, C, R>::from_columns(m[I]...);
        }(std::make_index_sequence<R>{});
    }

    template<typename T, size_t R0, size_t C0, size_t R1, size_t C1> requires (C0 == R1)
    NOA_HD constexpr auto matmul(const Mat<T, R0, C0>& m0, const Mat<T, R1, C1>& m1) noexcept -> Mat<T, R0, C1> {
        using output_t = Mat<T, R0, C1>;
        output_t out{};
        for (size_t r{}; r < R0; ++r)
            for (size_t c{}; c < C0; ++c)
                out[r] += m0[r][c] * m1[c];
        return out;
    }

    template<typename T, size_t R, size_t C> requires (R == C)
    [[nodiscard]] NOA_HD constexpr auto determinant(const Mat<T, R, C>& m) noexcept -> T {
        if constexpr (R == 2) {
            return m[0][0] * m[1][1] - m[0][1] * m[1][0];

        } else if constexpr (R == 3) {
            return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
                   m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
                   m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);

        } else if constexpr (R == 4) {
            const auto s00 = m[2][2] * m[3][3] - m[2][3] * m[3][2];
            const auto s01 = m[1][2] * m[3][3] - m[1][3] * m[3][2];
            const auto s02 = m[1][2] * m[2][3] - m[1][3] * m[2][2];
            const auto s03 = m[0][2] * m[3][3] - m[0][3] * m[3][2];
            const auto s04 = m[0][2] * m[2][3] - m[0][3] * m[2][2];
            const auto s05 = m[0][2] * m[1][3] - m[0][3] * m[1][2];

            Vec c{+(m[1][1] * s00 - m[2][1] * s01 + m[3][1] * s02),
                  -(m[0][1] * s00 - m[2][1] * s03 + m[3][1] * s04),
                  +(m[0][1] * s01 - m[1][1] * s03 + m[3][1] * s05),
                  -(m[0][1] * s02 - m[1][1] * s04 + m[2][1] * s05)};

            return m[0][0] * c[0] + m[1][0] * c[1] +
                   m[2][0] * c[2] + m[3][0] * c[3];
        } else {
            static_assert(nt::always_false<T>);
        }
    }

    template<typename T, size_t R, size_t C> requires (R == C)
    NOA_HD constexpr auto inverse(const Mat<T, R, C>& m) noexcept -> Mat<T, R, C> {
        if constexpr (R == 2) {
            const auto det = determinant(m);
            NOA_ASSERT(not allclose(det, T{})); // non singular
            const auto one_over_determinant = 1 / det;
            return {{{+m[1][1] * one_over_determinant, -m[0][1] * one_over_determinant},
                     {-m[1][0] * one_over_determinant, +m[0][0] * one_over_determinant}}};

        } else if constexpr (R == 3) {
            const auto det = determinant(m);
            NOA_ASSERT(not allclose(det, T{})); // non singular
            const auto one_over_determinant = 1 / det;
            return Mat<T, 3, 3>::from_values(
                    +(m[1][1] * m[2][2] - m[1][2] * m[2][1]) * one_over_determinant,
                    -(m[0][1] * m[2][2] - m[0][2] * m[2][1]) * one_over_determinant,
                    +(m[0][1] * m[1][2] - m[0][2] * m[1][1]) * one_over_determinant,
                    -(m[1][0] * m[2][2] - m[1][2] * m[2][0]) * one_over_determinant,
                    +(m[0][0] * m[2][2] - m[0][2] * m[2][0]) * one_over_determinant,
                    -(m[0][0] * m[1][2] - m[0][2] * m[1][0]) * one_over_determinant,
                    +(m[1][0] * m[2][1] - m[1][1] * m[2][0]) * one_over_determinant,
                    -(m[0][0] * m[2][1] - m[0][1] * m[2][0]) * one_over_determinant,
                    +(m[0][0] * m[1][1] - m[0][1] * m[1][0]) * one_over_determinant);

        } else if constexpr (R == 4) {
            // From https://stackoverflow.com/a/44446912 and https://github.com/willnode/N-Matrix-Programmer
            const auto A2323 = m[2][2] * m[3][3] - m[2][3] * m[3][2];
            const auto A1323 = m[2][1] * m[3][3] - m[2][3] * m[3][1];
            const auto A1223 = m[2][1] * m[3][2] - m[2][2] * m[3][1];
            const auto A0323 = m[2][0] * m[3][3] - m[2][3] * m[3][0];
            const auto A0223 = m[2][0] * m[3][2] - m[2][2] * m[3][0];
            const auto A0123 = m[2][0] * m[3][1] - m[2][1] * m[3][0];
            const auto A2313 = m[1][2] * m[3][3] - m[1][3] * m[3][2];
            const auto A1313 = m[1][1] * m[3][3] - m[1][3] * m[3][1];
            const auto A1213 = m[1][1] * m[3][2] - m[1][2] * m[3][1];
            const auto A2312 = m[1][2] * m[2][3] - m[1][3] * m[2][2];
            const auto A1312 = m[1][1] * m[2][3] - m[1][3] * m[2][1];
            const auto A1212 = m[1][1] * m[2][2] - m[1][2] * m[2][1];
            const auto A0313 = m[1][0] * m[3][3] - m[1][3] * m[3][0];
            const auto A0213 = m[1][0] * m[3][2] - m[1][2] * m[3][0];
            const auto A0312 = m[1][0] * m[2][3] - m[1][3] * m[2][0];
            const auto A0212 = m[1][0] * m[2][2] - m[1][2] * m[2][0];
            const auto A0113 = m[1][0] * m[3][1] - m[1][1] * m[3][0];
            const auto A0112 = m[1][0] * m[2][1] - m[1][1] * m[2][0];

            auto det = m[0][0] * (m[1][1] * A2323 - m[1][2] * A1323 + m[1][3] * A1223) -
                       m[0][1] * (m[1][0] * A2323 - m[1][2] * A0323 + m[1][3] * A0223) +
                       m[0][2] * (m[1][0] * A1323 - m[1][1] * A0323 + m[1][3] * A0123) -
                       m[0][3] * (m[1][0] * A1223 - m[1][1] * A0223 + m[1][2] * A0123);
            NOA_ASSERT(not allclose(det, T{})); // non singular
            det = 1 / det;

            return Mat<T, 4, 4>::from_values(
                    det * +(m[1][1] * A2323 - m[1][2] * A1323 + m[1][3] * A1223),
                    det * -(m[0][1] * A2323 - m[0][2] * A1323 + m[0][3] * A1223),
                    det * +(m[0][1] * A2313 - m[0][2] * A1313 + m[0][3] * A1213),
                    det * -(m[0][1] * A2312 - m[0][2] * A1312 + m[0][3] * A1212),
                    det * -(m[1][0] * A2323 - m[1][2] * A0323 + m[1][3] * A0223),
                    det * +(m[0][0] * A2323 - m[0][2] * A0323 + m[0][3] * A0223),
                    det * -(m[0][0] * A2313 - m[0][2] * A0313 + m[0][3] * A0213),
                    det * +(m[0][0] * A2312 - m[0][2] * A0312 + m[0][3] * A0212),
                    det * +(m[1][0] * A1323 - m[1][1] * A0323 + m[1][3] * A0123),
                    det * -(m[0][0] * A1323 - m[0][1] * A0323 + m[0][3] * A0123),
                    det * +(m[0][0] * A1313 - m[0][1] * A0313 + m[0][3] * A0113),
                    det * -(m[0][0] * A1312 - m[0][1] * A0312 + m[0][3] * A0112),
                    det * -(m[1][0] * A1223 - m[1][1] * A0223 + m[1][2] * A0123),
                    det * +(m[0][0] * A1223 - m[0][1] * A0223 + m[0][2] * A0123),
                    det * -(m[0][0] * A1213 - m[0][1] * A0213 + m[0][2] * A0113),
                    det * +(m[0][0] * A1212 - m[0][1] * A0212 + m[0][2] * A0112));
        } else {
            static_assert(nt::always_false<T>);
        }
    }

    template<typename T, size_t R, size_t C>
    [[nodiscard]] NOA_HD constexpr auto ewise_multiply(
        Mat<T, R, C> m1,
        const Mat<T, R, C>& m2
    ) noexcept -> Mat<T, R, C> {
        for (size_t i{}; i < R; ++i)
            m1[i] *= m2[i];
        return m1;
    }

    template<typename T, size_t C, size_t R, size_t A0, size_t A1>
    [[nodiscard]] NOA_HD constexpr auto outer_product(
        const Vec<T, R, A0>& column,
        const Vec<T, C, A1>& row
    ) noexcept -> Mat<T, R, C> {
        Mat<T, R, C> out;
        for (size_t i{}; i < R; ++i)
            out[i] = column[i] * row;
        return out;
    }

    template<i32 ULP = 2, typename T, size_t C, size_t R>
    [[nodiscard]] NOA_HD constexpr bool allclose(
        const Mat<T, R, C>& m1,
        const Mat<T, R, C>& m2,
        std::type_identity_t<T> epsilon = static_cast<T>(1e-6)
    ) noexcept {
        for (size_t r{}; r < R; ++r)
            for (size_t c{}; c < C; ++c)
                if (not allclose<ULP>(m1[r][c], m2[r][c], epsilon))
                    return false;
        return true;
    }
}

namespace noa::inline types {
    /// Aggregate type representing a RxC geometric (row-major) matrix.
    template<typename T, size_t R, size_t C>
    class alignas(16) Mat {
    public:
        static_assert(nt::real<T> and R > 0 and C > 0);
        using value_type = T;
        using mutable_value_type = value_type;
        using row_type = Vec<value_type, C>;
        using column_type = Vec<value_type, R>;

        static constexpr size_t ROWS = R;
        static constexpr size_t COLS = C;
        static constexpr size_t DIAG = std::min(ROWS, COLS);
        static constexpr size_t SIZE = ROWS;
        static constexpr i64 SSIZE = ROWS;

    public:
        row_type row[ROWS];

    public: // Static factory functions
        [[nodiscard]] NOA_HD static constexpr Mat from_value(nt::scalar auto s) noexcept {
            Mat mat{};
            for (size_t r{}; r < ROWS; ++r)
                for (size_t c{}; c < COLS; ++c)
                    if (r == c)
                        mat[r][c] = static_cast<value_type>(s);
            return mat;
        }

        [[nodiscard]] NOA_HD static constexpr Mat from_diagonal(nt::scalar auto s) noexcept {
            return from_value(s);
        }

        template<typename U, size_t A>
        [[nodiscard]] NOA_HD static constexpr Mat from_diagonal(const Vec<U, DIAG, A>& diagonal) noexcept {
            Mat mat{};
            for (size_t r{}; r < ROWS; ++r)
                for (size_t c{}; c < COLS; ++c)
                    if (r == c)
                        mat[r][c] = static_cast<value_type>(diagonal[r]);
            return mat;
        }

        [[nodiscard]] NOA_HD static constexpr Mat eye(nt::scalar auto s) noexcept {
            return from_diagonal(s);
        }

        template<typename U, size_t A>
        [[nodiscard]] NOA_HD static constexpr Mat eye(const Vec<U, DIAG, A>& diagonal) noexcept {
            return from_diagonal(diagonal);
        }

        template<typename U>
        [[nodiscard]] NOA_HD static constexpr Mat from_matrix(const Mat<U, ROWS, COLS>& m) noexcept {
            Mat mat;
            for (size_t r{}; r < ROWS; ++r)
                mat[r] = m[r].template as<value_type>();
            return mat;
        }

        template<nt::scalar... U> requires (sizeof...(U) == ROWS * COLS)
        [[nodiscard]] NOA_HD static constexpr Mat from_values(U... values) noexcept {
            Mat mat;
            auto op = [&mat, v = forward_as_tuple(values...)]<size_t I, size_t... J>(){
                ((mat[I][J] = static_cast<value_type>(v[Tag<I * COLS + J>{}])), ...);
            };
            [&op]<size_t... I, size_t... J>(std::index_sequence<I...>, std::index_sequence<J...>) {
                (op.template operator()<I, J...>(), ...);
            }(std::make_index_sequence<ROWS>{}, std::make_index_sequence<COLS>{});
            return mat;
        }

        [[nodiscard]] NOA_HD static constexpr Mat from_pointer(nt::scalar auto* ptr) noexcept {
            return [&ptr]<size_t...I>(std::index_sequence<I...>) {
                return Mat::from_values(ptr[I]...);
            }(std::make_index_sequence<ROWS * COLS>{});
        }

        template<nt::vec_scalar_size<COLS>... V> requires (sizeof...(V) == ROWS)
        [[nodiscard]] NOA_HD static constexpr Mat from_rows(const V&... r) noexcept {
            return {r.template as<value_type>()...};
        }

        template<nt::vec_scalar_size<ROWS>... V> requires (sizeof...(V) == COLS)
        [[nodiscard]] NOA_HD static constexpr Mat from_columns(const V&... c) noexcept {
            Mat mat;
            auto op = [&mat, v = forward_as_tuple(c...)]<size_t I, size_t... J>(){
                ((mat[I][J] = static_cast<value_type>(v[Tag<J>{}][I])), ...);
            };
            [&op]<size_t...I, size_t...J>(std::index_sequence<I...>, std::index_sequence<J...>) {
                (op.template operator()<I, J...>(), ...);
            }(std::make_index_sequence<ROWS>{}, std::make_index_sequence<COLS>{});
            return mat;
        }

    public:
        // Allow explicit conversion constructor (while still being an aggregate)
        // and add support for static_cast<Mat<U>>(Mat<T>{}).
        template<typename U>
        [[nodiscard]] NOA_HD constexpr explicit operator Mat<U, ROWS, COLS>() const noexcept {
            return Mat<U, ROWS, COLS>::from_matrix(*this);
        }

    public: // Component accesses
        NOA_HD constexpr auto operator[](nt::integer auto i) noexcept -> row_type& {
            NOA_ASSERT(static_cast<size_t>(i) < ROWS);
            return row[i];
        }

        NOA_HD constexpr auto operator[](nt::integer auto i) const noexcept -> const row_type& {
            NOA_ASSERT(static_cast<size_t>(i) < ROWS);
            return row[i];
        }

        // Structure binding support.
        template<int I> [[nodiscard]] NOA_HD constexpr const row_type& get() const noexcept { return (*this)[I]; }
        template<int I> [[nodiscard]] NOA_HD constexpr row_type& get() noexcept { return (*this)[I]; }

        [[nodiscard]] NOA_HD constexpr const row_type* data() const noexcept { return row; }
        [[nodiscard]] NOA_HD constexpr row_type* data() noexcept { return row; }
        [[nodiscard]] NOA_HD static constexpr size_t size() noexcept { return SIZE; };
        [[nodiscard]] NOA_HD static constexpr i64 ssize() noexcept { return SSIZE; };

    public: // Iterators -- support for range loops
        [[nodiscard]] NOA_HD constexpr row_type* begin() noexcept { return data(); }
        [[nodiscard]] NOA_HD constexpr const row_type* begin() const noexcept { return data(); }
        [[nodiscard]] NOA_HD constexpr const row_type* cbegin() const noexcept { return data(); }
        [[nodiscard]] NOA_HD constexpr row_type* end() noexcept { return data() + SIZE; }
        [[nodiscard]] NOA_HD constexpr const row_type* end() const noexcept { return data() + SIZE; }
        [[nodiscard]] NOA_HD constexpr const row_type* cend() const noexcept { return data() + SIZE; }

    public: // Assignment operators
        NOA_HD constexpr Mat& operator+=(const Mat& m) noexcept {
            for (size_t i{}; i < ROWS; ++i)
                row[i] += m[i];
            return *this;
        }

        NOA_HD constexpr Mat& operator-=(const Mat& m) noexcept {
            for (size_t i{}; i < ROWS; ++i)
                row[i] -= m[i];
            return *this;
        }

        NOA_HD constexpr Mat& operator*=(const Mat& m) noexcept requires (ROWS == COLS) {
            *this = noa::matmul(*this, m);
            return *this;
        }

        NOA_HD constexpr Mat& operator/=(const Mat& m) noexcept requires (ROWS == COLS) {
            *this *= noa::inverse(m);
            return *this;
        }

        NOA_HD constexpr Mat& operator+=(value_type s) noexcept {
            for (auto& r: row)
                r += s;
            return *this;
        }

        NOA_HD constexpr Mat& operator-=(value_type s) noexcept {
            for (auto& r: row)
                r -= s;
            return *this;
        }

        NOA_HD constexpr Mat& operator*=(value_type s) noexcept {
            for (auto& r: row)
                r *= s;
            return *this;
        }

        NOA_HD constexpr Mat& operator/=(value_type s) noexcept {
            for (auto& r: row)
                r /= s;
            return *this;
        }

    public: // Non-member functions
        // -- Unary operators --
        [[nodiscard]] friend NOA_HD constexpr Mat operator+(const Mat& m) noexcept {
            return m;
        }

        [[nodiscard]] friend NOA_HD constexpr Mat operator-(const Mat& m) noexcept {
            return [&m]<size_t... I>(std::index_sequence<I...>) {
                return Mat{-m[I]...};
            }(std::make_index_sequence<ROWS>{});
        }

        // -- Binary arithmetic operators --
        [[nodiscard]] friend NOA_HD constexpr Mat operator+(const Mat& m1, const Mat& m2) noexcept {
            return [&]<size_t... I>(std::index_sequence<I...>) {
                return Mat{m1[I] + m2[I]...};
            }(std::make_index_sequence<ROWS>{});
        }

        [[nodiscard]] friend NOA_HD constexpr Mat operator+(value_type s, const Mat& m) noexcept {
            return [&]<size_t... I>(std::index_sequence<I...>) {
                return Mat{s + m[I]...};
            }(std::make_index_sequence<ROWS>{});
        }

        [[nodiscard]] friend NOA_HD constexpr Mat operator+(const Mat& m, value_type s) noexcept {
            return [&]<size_t... I>(std::index_sequence<I...>) {
                return Mat{m[I] + s...};
            }(std::make_index_sequence<ROWS>{});
        }

        [[nodiscard]] friend NOA_HD constexpr Mat operator-(const Mat& m1, const Mat& m2) noexcept {
            return [&]<size_t... I>(std::index_sequence<I...>) {
                return Mat{m1[I] - m2[I]...};
            }(std::make_index_sequence<ROWS>{});
        }

        [[nodiscard]] friend NOA_HD constexpr Mat operator-(value_type s, const Mat& m) noexcept {
            return [&]<size_t... I>(std::index_sequence<I...>) {
                return Mat{s - m[I]...};
            }(std::make_index_sequence<ROWS>{});
        }

        [[nodiscard]] friend NOA_HD constexpr Mat operator-(const Mat& m, value_type s) noexcept {
            return [&]<size_t... I>(std::index_sequence<I...>) {
                return Mat{m[I] - s...};
            }(std::make_index_sequence<ROWS>{});
        }

        template<size_t C1>
        [[nodiscard]] friend NOA_HD constexpr auto operator*(const Mat& m1, const Mat<value_type, COLS, C1>& m2) noexcept {
            return matmul(m1, m2);
        }

        [[nodiscard]] friend NOA_HD constexpr Mat operator*(value_type s, const Mat& m) noexcept {
            return [&]<size_t... I>(std::index_sequence<I...>) {
                return Mat{s * m[I]...};
            }(std::make_index_sequence<ROWS>{});
        }

        [[nodiscard]] friend NOA_HD constexpr Mat operator*(const Mat& m, value_type s) noexcept {
            return [&]<size_t... I>(std::index_sequence<I...>) {
                return Mat{m[I] * s...};
            }(std::make_index_sequence<ROWS>{});
        }

        template<size_t A>
        [[nodiscard]] friend NOA_HD constexpr auto operator*(const Mat& m, const Vec<value_type, C, A>& c) noexcept {
            return [&]<size_t... I>(std::index_sequence<I...>) {
                return column_type{dot(m[I], c)...};
            }(std::make_index_sequence<ROWS>{});
        }

        template<size_t A>
        [[nodiscard]] friend NOA_HD constexpr auto operator*(const Vec<value_type, R, A>& r, const Mat& m) noexcept {
            row_type out{};
            for (size_t i{}; i < ROWS; ++i)
                out += r[i] * m[i];
            return out;
        }

        [[nodiscard]] friend NOA_HD constexpr Mat operator/(Mat m1, const Mat& m2) noexcept requires (ROWS == COLS) {
            m1 /= m2;
            return m1;
        }

        [[nodiscard]] friend NOA_HD constexpr Mat operator/(value_type s, const Mat& m) noexcept {
            return [&]<size_t... I>(std::index_sequence<I...>) {
                return Mat{s / m[I]...};
            }(std::make_index_sequence<ROWS>{});
        }

        [[nodiscard]] friend NOA_HD constexpr Mat operator/(const Mat& m, value_type s) noexcept {
            return [&]<size_t... I>(std::index_sequence<I...>) {
                return Mat{m[I] / s...};
            }(std::make_index_sequence<ROWS>{});
        }

        template<size_t A> requires (ROWS == COLS)
        [[nodiscard]] friend NOA_HD constexpr column_type operator/(const Mat& m, const Vec<value_type, C, A>& c) noexcept {
            return noa::inverse(m) * c;
        }

        template<size_t A> requires (ROWS == COLS)
        [[nodiscard]] friend NOA_HD constexpr row_type operator/(const Vec<value_type, R, A>& r, const Mat& m) noexcept {
            return r * noa::inverse(m);
        }

        [[nodiscard]] friend NOA_HD constexpr bool operator==(const Mat& m1, const Mat& m2) noexcept {
            for (size_t r{}; r < ROWS; ++r)
                if (not vall(Equal{}, m1[r], m2[r]))
                    return false;
            return true;
        }

        [[nodiscard]] friend NOA_HD constexpr bool operator!=(const Mat& m1, const Mat& m2) noexcept {
            for (size_t r{}; r < ROWS; ++r)
                if (vany(NotEqual{}, m1[r], m2[r]))
                    return true;
            return false;
        }

    public:
        template<nt::real U>
        [[nodiscard]] NOA_HD constexpr auto as() const noexcept {
            return Mat<U, ROWS, COLS>::from_matrix(*this);
        }

        [[nodiscard]] NOA_IHD constexpr Mat transpose() const noexcept {
            return noa::transpose(*this);
        }

        [[nodiscard]] NOA_IHD constexpr Mat inverse() const noexcept {
            return noa::inverse(*this);
        }
    };

    template<typename T> using Mat22 = Mat<T, 2, 2>;
    template<typename T> using Mat23 = Mat<T, 2, 3>;
    template<typename T> using Mat33 = Mat<T, 3, 3>;
    template<typename T> using Mat34 = Mat<T, 3, 4>;
    template<typename T> using Mat44 = Mat<T, 4, 4>;
}

namespace std {
    template<typename T, size_t R, size_t C>
    struct tuple_size<noa::Mat<T, R, C>> : std::integral_constant<size_t, R> {};

    template<typename T, size_t R, size_t C>
    struct tuple_size<const noa::Mat<T, R, C>> : std::integral_constant<size_t, R> {};

    template<size_t I, size_t R, size_t C, typename T>
    struct tuple_element<I, noa::Mat<T, R, C>> { using type = T; };

    template<size_t I, size_t R, size_t C, typename T>
    struct tuple_element<I, const noa::Mat<T, R, C>> { using type = const T; };
}

namespace noa::traits {
    template<typename T, size_t R, size_t C>
    struct proclaim_is_mat<noa::Mat<T, R, C>> : std::true_type {};

    template<typename T, size_t R0, size_t C0, size_t R, size_t C>
    struct proclaim_is_mat_of_shape<noa::Mat<T, R0, C0>, R, C> : std::bool_constant<R0 == R and C0 == C> {};
}

#ifdef NOA_IS_OFFLINE
namespace noa::string {
    template<typename T, size_t R, size_t C>
    struct Stringify<Mat<T, R, C>> {
        static auto get() -> std::string {
            return fmt::format("Mat<{},{},{}>", stringify<T>(), R, C);
        }
    };
}
#endif
