#pragma once

#include "noa/core/Config.hpp"
#include "noa/core/Traits.hpp"
#include "noa/core/types/Vec.hpp"
#include "noa/core/types/Shape.hpp"
#include "noa/core/indexing/Offset.hpp"
#include "noa/core/utils/Misc.hpp"

namespace noa::guts {
    template<typename A, size_t... Is, typename Pointer, typename... Indexes>
    [[nodiscard]] NOA_FHD constexpr auto offset_pointer(
            A&& accessor, Pointer pointer,
            std::index_sequence<Is...>,
            Indexes&&... indexes
    ) noexcept {
        ((pointer += offset_at(indexes, accessor.template stride<Is>())), ...);
        return pointer;
    }

    template<typename A, size_t... Is, typename Pointer, typename Integer>
    [[nodiscard]] NOA_FHD constexpr auto offset_pointer(
            A&& accessor, Pointer pointer,
            std::index_sequence<Is...>,
            const Vec<Integer, sizeof...(Is)>& indexes
    ) noexcept {
        ((pointer += offset_at(indexes[Is], accessor.template stride<Is>())), ...);
        return pointer;
    }
}

namespace noa {
    enum class PointerTraits { DEFAULT, RESTRICT }; // TODO ATOMIC?
    enum class StridesTraits { STRIDED, CONTIGUOUS }; // TODO EMPTY?

    template<typename T, size_t N, typename I,
             PointerTraits PointerTrait,
             StridesTraits StridesTrait>
    class AccessorReference;

    /// Multidimensional accessor.
    ///
    /// \details Wraps a pointer and nd-strides, and provides nd-indexing.
    ///          Accessors are mostly intended for internal use, so we aim for overall good performance.
    ///          As such:
    ///          - The size of the dimensions are not stored, so accessors cannot bound-check the indexes against
    ///            their dimension size. In a lot of cases, the input/output arrays have the same size/shape and
    ///            the size/shape is often not needed by the compute kernels, leading to storing useless data.
    ///            If the extents of the region are required, use (md)spans (which are intended for all use-cases).
    ///
    ///          - \b Pointer-traits. By default, the pointers are not marked with any attributes, but the "restrict"
    ///            traits can be added. This is useful to guarantee that pointers don't alias, which helps generating
    ///            better code.
    ///
    ///          - \b Strides-traits. Strides are fully dynamic (one dynamic stride per dimension) by default,
    ///            but the rightmost dimension can be marked contiguous. Accessors (and the library internals) uses
    ///            the rightmost convention, so that the innermost dimension is the rightmost dimension.
    ///            As such, StridesTraits::CONTIGUOUS implies C-contiguous. F-contiguous layouts are not supported
    ///            by the accessors, as these layouts should be reordered to C-contiguous before creating the
    ///            contiguous accessor.
    ///            With StridesTraits::CONTIGUOUS, the innermost/rightmost stride is fixed to 1 and is not stored,
    ///            resulting in the strides being truncated by 1 (Strides<I,N-1>). In case of a 1d contiguous
    ///            accessor, this mean that the strides are empty (Strides<I,0>) and the indexing is equivalent
    ///            to pointer/array indexing.
    template<typename T, size_t N, typename I,
             PointerTraits PointerTrait = PointerTraits::DEFAULT,
             StridesTraits StridesTrait = StridesTraits::STRIDED>
    class Accessor {
    public:
        static_assert(!std::is_pointer_v<T>);
        static_assert(!std::is_reference_v<T>);
        static_assert(std::is_integral_v<I>);
        static_assert(N <= 4);

        static constexpr bool IS_RESTRICT = PointerTrait == PointerTraits::RESTRICT;
        static constexpr bool IS_CONTIGUOUS = StridesTrait == StridesTraits::CONTIGUOUS;
        static constexpr size_t SIZE = N;
        static constexpr int64_t SSIZE = N;

        #if defined(__CUDACC__)
        using pointer_type = std::conditional_t<IS_RESTRICT, T* __restrict__, T*>;
        #else
        using pointer_type = std::conditional_t<IS_RESTRICT, T* __restrict, T*>;
        #endif
        using value_type = T;
        using mutable_value_type = std::remove_const_t<T>;
        using index_type = I;
        using strides_type = std::conditional_t<!IS_CONTIGUOUS, Strides<index_type, N>, Strides<index_type, N - 1>>;
        using accessor_reference_type = AccessorReference<value_type, SIZE, index_type, PointerTrait, StridesTrait>;

    public: // Constructors
        NOA_HD constexpr Accessor() = default;

        /// Creates a strided or contiguous accessor.
        NOA_HD constexpr Accessor(pointer_type pointer, const Strides<index_type, SIZE>& strides) noexcept
                : m_ptr{pointer},
                  m_strides{strides_type::from_pointer(strides.data())} {}

        /// Creates an accessor from an accessor reference.
        NOA_HD constexpr explicit Accessor(accessor_reference_type accessor_reference) noexcept
                : m_ptr{accessor_reference.get()},
                  m_strides{strides_type::from_pointer(accessor_reference.strides())} {}

        /// Creates a contiguous 1d accessor, assuming the stride is 1.
        template<typename Void = void, typename = std::enable_if_t<(SIZE == 1) && IS_CONTIGUOUS && std::is_void_v<Void>>>
        NOA_HD constexpr explicit Accessor(pointer_type pointer) noexcept
                : m_ptr{pointer}, m_strides{/*empty*/} {}

        /// Creates a const accessor from an existing non-const accessor.
        template<typename U, nt::enable_if_bool_t<nt::is_mutable_value_type_v<U, value_type>> = true>
        NOA_HD constexpr /* implicit */ Accessor(const Accessor<U, N, I, PointerTrait, StridesTrait>& accessor)
                : m_ptr{accessor.get()}, m_strides{accessor.strides()} {}

    public: // Accessing strides
        template<size_t INDEX>
        [[nodiscard]] NOA_HD constexpr auto stride() const noexcept {
            static_assert(INDEX < N);
            if constexpr (IS_CONTIGUOUS && INDEX == SIZE - 1)
                return index_type{1};
            return m_strides[INDEX];
        }

        [[nodiscard]] NOA_HD constexpr strides_type& strides() noexcept { return m_strides; }
        [[nodiscard]] NOA_HD constexpr const strides_type& strides() const noexcept { return m_strides; }

    public:
        [[nodiscard]] NOA_HD constexpr pointer_type get() const noexcept { return m_ptr; }
        [[nodiscard]] NOA_HD constexpr pointer_type data() const noexcept { return m_ptr; }
        [[nodiscard]] NOA_HD constexpr bool is_empty() const noexcept { return m_ptr == nullptr; }
        [[nodiscard]] NOA_HD constexpr explicit operator bool() const noexcept { return !is_empty(); }

        [[nodiscard]] NOA_HD constexpr accessor_reference_type to_accessor_reference() const noexcept {
            return accessor_reference_type(*this);
        }

        /// Swap the dimensions (i.e. the strides), in-place.
        template<typename Int0, typename Int1,
                 typename = std::enable_if_t<StridesTraits::STRIDED == StridesTrait && nt::are_int_v<Int0, Int1>>>
        NOA_HD constexpr Accessor& swap_dimensions(Int0 d0, Int1 d1) noexcept {
            noa::guts::swap(strides()[d0], strides()[d1]);
            return *this;
        }

    public:
        /// Offsets the pointer at dimension 0, in-place.
        /// This is often used to offset the starting batch when working in chunks.
        template<typename Int, typename = std::enable_if_t<std::is_integral_v<Int>>>
        NOA_HD constexpr Accessor& offset_accessor(Int index) noexcept {
            NOA_ASSERT(!is_empty());
            m_ptr += offset_at(index, stride<0>());
            return *this;
        }

        /// C-style indexing operator, decrementing the dimensionality of the accessor by 1.
        template<typename Int, nt::enable_if_bool_t<(SIZE > 1) && std::is_integral_v<Int>> = true>
        [[nodiscard]] NOA_HD auto operator[](Int index) const noexcept {
            NOA_ASSERT(!is_empty());
            using output_type = AccessorReference<value_type, (SIZE - 1), index_type, PointerTrait, StridesTrait>;
            return output_type(m_ptr + offset_at(index, stride<0>()), strides().data() + 1);
        }

        /// C-style indexing operator, decrementing the dimensionality of the accessor by 1.
        /// When done on a 1d accessor, this acts as a pointer/array indexing and dereferences the data.
        template<typename Int, nt::enable_if_bool_t<(SIZE == 1) && std::is_integral_v<Int>> = true>
        [[nodiscard]] NOA_HD value_type& operator[](Int index) const noexcept {
            NOA_ASSERT(!is_empty());
            return m_ptr[offset_at(index, stride<0>())];
        }

    public:
        template<typename Pointer, typename... Indexes,
                 nt::enable_if_bool_t<SIZE == sizeof...(Indexes) && nt::are_int_v<Indexes...>> = true>
        [[nodiscard]] NOA_HD constexpr auto offset_pointer(
                Pointer pointer, Indexes&&... indexes
        ) const noexcept {
            return guts::offset_pointer(
                    *this, pointer, std::make_index_sequence<SIZE>{},
                    std::forward<Indexes>(indexes)...);
        }

        template<typename... Indexes,
                 nt::enable_if_bool_t<SIZE == sizeof...(Indexes) && nt::are_int_v<Indexes...>> = true>
        [[nodiscard]] NOA_HD constexpr value_type& operator()(Indexes&&... indexes) const noexcept {
            NOA_ASSERT(!is_empty());
            return *offset_pointer(m_ptr, std::forward<Indexes>(indexes)...);
        }

        template<size_t N0, typename Integer, nt::enable_if_bool_t<SIZE == N0 && nt::is_int_v<Integer>> = true>
        [[nodiscard]] NOA_HD constexpr value_type& operator()(const Vec<Integer, N0>& indexes) const noexcept {
            NOA_ASSERT(!is_empty());
            return *guts::offset_pointer(*this, m_ptr, std::make_index_sequence<N0>{}, indexes);
        }

    private:
        pointer_type m_ptr{};
        NOA_NO_UNIQUE_ADDRESS strides_type m_strides{};
    };

    /// Reference to Accessor.
    ///
    /// \details This is similar to Accessor, except that this type does not store the strides, it simply points to
    ///          existing ones. Usually AccessorReference is not constructed explicitly, and is instead the result of
    ///          Accessor::operator[] used to decrement the dimensionality of the Accessor, emulating C-style multi-
    ///          dimensional indexing, e.g. a[b][d][h][w] = value.
    template<typename T, size_t N, typename I,
             PointerTraits PointerTrait = PointerTraits::DEFAULT,
             StridesTraits StridesTrait = StridesTraits::STRIDED>
    class AccessorReference {
    public:
        static constexpr bool IS_RESTRICT = PointerTrait == PointerTraits::RESTRICT;
        static constexpr bool IS_CONTIGUOUS = StridesTrait == StridesTraits::CONTIGUOUS;
        static constexpr size_t SIZE = N;
        static constexpr int64_t SSIZE = N;

        using accessor_type = Accessor<T, N, I, PointerTrait, StridesTrait>;
        using value_type = typename accessor_type::value_type;
        using mutable_value_type = typename accessor_type::mutable_value_type;
        using index_type = typename accessor_type::index_type;
        using pointer_type = typename accessor_type::pointer_type;
        using strides_type = const index_type*;

    public:
        // Creates an empty view.
        NOA_HD constexpr AccessorReference() = default;

        /// Creates a reference to an accessor.
        /// For the contiguous case, the rightmost stride is ignored and never read from
        /// the \p strides strides pointer (so \p strides[N-1] is never accessed).
        /// As such, in the 1d contiguous case, a nullptr can be passed.
        NOA_HD constexpr AccessorReference(pointer_type pointer, strides_type strides) noexcept
                : m_ptr(pointer), m_strides(strides) {}

        NOA_HD constexpr explicit AccessorReference(accessor_type accessor) noexcept
                : AccessorReference(accessor.ptr, accessor.strides().data()) {}

        // Creates a const accessor from an existing non-const accessor.
        template<typename U, typename = std::enable_if_t<nt::is_mutable_value_type_v<U, value_type>>>
        NOA_HD constexpr /* implicit */ AccessorReference(
                const AccessorReference<U, N, I, PointerTrait, StridesTrait>& accessor
        ) : m_ptr(accessor.get()), m_strides(accessor.strides()) {}

    public: // Accessing strides
        template<size_t INDEX>
        [[nodiscard]] NOA_HD constexpr auto stride() const noexcept {
            static_assert(INDEX < N);
            if constexpr (IS_CONTIGUOUS && INDEX == SIZE - 1)
                return index_type{1};
            return m_strides[INDEX];
        }

        [[nodiscard]] NOA_HD constexpr strides_type strides() noexcept { return m_strides; }

    public:
        [[nodiscard]] NOA_HD constexpr pointer_type get() const noexcept { return m_ptr; }
        [[nodiscard]] NOA_HD constexpr pointer_type data() const noexcept { return m_ptr; }
        [[nodiscard]] NOA_HD constexpr bool is_empty() const noexcept { return m_ptr == nullptr; }
        [[nodiscard]] NOA_HD constexpr explicit operator bool() const noexcept { return !is_empty(); }

        [[nodiscard]] NOA_HD constexpr accessor_type to_accessor() const noexcept {
            return accessor_type(*this);
        }

    public:
        template<typename Int, typename = std::enable_if_t<std::is_integral_v<Int>>>
        NOA_HD AccessorReference& offset_accessor(Int index) noexcept {
            NOA_ASSERT(!is_empty());
            m_ptr += noa::offset_at(index, stride<0>());
            return *this;
        }

        // Indexing operator, on 1D accessor. 1D -> ref
        template<typename Int, std::enable_if_t<SIZE == 1 && std::is_integral_v<Int>, bool> = true>
        [[nodiscard]] NOA_HD value_type& operator[](Int index) const noexcept {
            NOA_ASSERT(!is_empty());
            return m_ptr[noa::offset_at(index, stride<0>())];
        }

        // Indexing operator, multidimensional accessor. ND -> ND-1
        template<typename Int, std::enable_if_t<(SIZE > 1) && std::is_integral_v<Int>, bool> = true>
        [[nodiscard]] NOA_HD auto operator[](Int index) const noexcept {
            NOA_ASSERT(!is_empty());
            using output_type = AccessorReference<value_type, SIZE - 1, index_type, PointerTrait, StridesTrait>;
            return output_type(m_ptr + noa::offset_at(index, stride<0>()), m_strides + 1);
        }

    public:
        template<typename Pointer, typename... Indexes,
                 nt::enable_if_bool_t<SIZE == sizeof...(Indexes) && nt::are_int_v<Indexes...>> = true>
        [[nodiscard]] NOA_HD constexpr auto offset_pointer(
                Pointer pointer, Indexes&&... indexes
        ) const noexcept {
            return guts::offset_pointer(
                    *this, pointer,
                    std::make_index_sequence<SIZE>{},
                    std::forward<Indexes>(indexes)...);
        }

        template<typename... Indexes,
                 nt::enable_if_bool_t<SIZE == sizeof...(Indexes) && nt::are_int_v<Indexes...>> = true>
        [[nodiscard]] NOA_HD constexpr value_type& operator()(Indexes&&... indexes) const noexcept {
            NOA_ASSERT(!is_empty());
            return *offset_pointer(m_ptr, std::forward<Indexes>(indexes)...);
        }

        template<size_t N0, typename Int> // nt::enable_if_bool_t<(SIZE >= 1) && std::is_integral_v<Int>> = true>
        [[nodiscard]] NOA_HD constexpr value_type& operator()(const Vec<Int, N0>& indexes) const noexcept {
            NOA_ASSERT(!is_empty());
            return *guts::offset_pointer(*this, m_ptr, std::make_index_sequence<N0>{}, indexes);
        }

    private:
        pointer_type m_ptr{};
        NOA_NO_UNIQUE_ADDRESS strides_type m_strides{};
    };

    /// Stores a value and provide an nd-accessor interface of that value.
    ///
    /// \details This is poorly named because as opposed to Accessor(Reference), this type is the owner of the
    ///          thing being accessed. The original goal is to provide the accessor interface so we can index
    ///          a value as if it was a nd-array (something like Scalar would have been have better choice),
    ///          but we want to emphasize that the goal here is to support the accessor indexing, while referring
    ///          to a single _local_ single value (we don't want to refer to a value on the heap for example).
    ///
    /// \note As opposed to the Accessor, the const-ness is also enforced by the accessor.
    ///       With AccessorValue<const f64>, the value cannot be mutated.
    ///       With AccessorValue<f64>, the value can be mutated, iif the accessor itself is not const.
    ///       This is because the AccessorValue stores the value, so const-ness is transferred to the member variable.
    ///
    /// \note This can be treated as an Accessor of any dimension. So a(2) or a(2,3,4,5) are equivalent and
    ///       are both returning a reference of the value. The only constraint is on operator[], which always
    ///       returns a reference of the value (like array/pointer indexing).
    ///
    /// \note The pointer_type (e.g. returned from .data()) is always marked restrict because the value is owned
    ///       by the accessor. This shouldn't be relevant since the compiler can see that, but it's just to keep
    ///       things coherent.
    ///
    /// \tparam T Any value. A value of that type is stored in the accessor.
    /// \tparam I Index type. This defines the type for the strides (which are empty).
    template<typename T, typename I>
    class AccessorValue {
    public:
        static_assert(!std::is_pointer_v<T>);
        static_assert(!std::is_reference_v<T>);

        static constexpr bool IS_RESTRICT = true;
        static constexpr bool IS_CONTIGUOUS = true;
        static constexpr size_t SIZE = 1;
        static constexpr int64_t SSIZE = 1;

        using value_type = T;
        using index_type = I;
        using mutable_value_type = std::remove_const_t<T>;
        using strides_type = Strides<index_type, 0>;
        #if defined(__CUDACC__)
        using pointer_type = T* __restrict__;
        using const_pointer_type = const mutable_value_type* __restrict__;
        #else
        using pointer_type = T* __restrict;
        using const_pointer_type = const mutable_value_type* __restrict;
        #endif

    public: // Constructors
        NOA_HD constexpr AccessorValue() = default;

        NOA_HD constexpr explicit AccessorValue(mutable_value_type&& value) noexcept
                : m_value{std::move(value)} {}

        NOA_HD constexpr explicit AccessorValue(const mutable_value_type& value) noexcept
                : m_value{value} {}

    public: // Accessing strides
        template<size_t>
        [[nodiscard]] NOA_HD constexpr index_type stride() const noexcept { return 0; }
        [[nodiscard]] NOA_HD constexpr strides_type strides() const noexcept { return {}; } // TODO C++20

    public:
        // Note: restrict is a type qualifier and is ignored in the return type? -Wignored-qualifiers
        [[nodiscard]] NOA_HD constexpr value_type* get() noexcept { return &m_value; }
        [[nodiscard]] NOA_HD constexpr value_type* data() noexcept { return &m_value; }
        [[nodiscard]] NOA_HD constexpr const value_type* get() const noexcept { return &m_value; }
        [[nodiscard]] NOA_HD constexpr const value_type* data() const noexcept { return &m_value; }
        [[nodiscard]] NOA_HD constexpr bool is_empty() const noexcept { return false; }
        [[nodiscard]] NOA_HD constexpr explicit operator bool() const noexcept { return !is_empty(); }

        template<typename Int0, typename Int1, typename = std::enable_if_t<nt::are_int_v<Int0, Int1>>>
        NOA_HD constexpr AccessorValue& swap_dimensions(Int0, Int1) noexcept {
            return *this;
        }

    public:
        template<typename Int, typename = std::enable_if_t<std::is_integral_v<Int>>>
        NOA_HD constexpr AccessorValue& offset_accessor(Int) noexcept {
            return *this;
        }

        template<typename Int, nt::enable_if_bool_t<std::is_integral_v<Int>> = true>
        [[nodiscard]] NOA_HD value_type& operator[](Int) const noexcept {
            return *m_value;
        }

        template<typename Int, nt::enable_if_bool_t<std::is_integral_v<Int>> = true>
        [[nodiscard]] NOA_HD value_type& operator[](Int) noexcept {
            return *m_value;
        }

    public:
        template<typename Pointer>
        [[nodiscard]] NOA_HD constexpr auto offset_pointer(Pointer pointer) const noexcept {
            return pointer;
        }

        template<typename... Indexes, nt::enable_if_bool_t<nt::are_int_v<Indexes...>> = true>
        [[nodiscard]] NOA_HD constexpr value_type& operator()(Indexes&&...) const noexcept {
            return *m_value;
        }
        template<typename... Indexes, nt::enable_if_bool_t<nt::are_int_v<Indexes...>> = true>
        [[nodiscard]] NOA_HD constexpr value_type& operator()(Indexes&&...) noexcept {
            return *m_value;
        }

        template<size_t N0, typename Integer, nt::enable_if_bool_t<nt::is_int_v<Integer>> = true>
        [[nodiscard]] NOA_HD constexpr value_type& operator()(const Vec<Integer, N0>&) const noexcept {
            return *m_value;
        }
        template<size_t N0, typename Integer, nt::enable_if_bool_t<nt::is_int_v<Integer>> = true>
        [[nodiscard]] NOA_HD constexpr value_type& operator()(const Vec<Integer, N0>&) noexcept {
            return *m_value;
        }

    private:
        mutable_value_type m_value;
        // TODO C++20 Add strides_type (which is empty) here with [[no_unique_address]]
    };

    template<typename T, size_t N>
    using AccessorI64 = Accessor<T, N, int64_t>;
    template<typename T, size_t N>
    using AccessorI32 = Accessor<T, N, int32_t>;
    template<typename T, size_t N>
    using AccessorU64 = Accessor<T, N, uint64_t>;
    template<typename T, size_t N>
    using AccessorU32 = Accessor<T, N, uint32_t>;

    template<typename T, size_t N, typename I, StridesTraits StridesTrait = StridesTraits::STRIDED>
    using AccessorRestrict = Accessor<T, N, I, PointerTraits::RESTRICT, StridesTrait>;
    template<typename T, size_t N, StridesTraits StridesTrait = StridesTraits::STRIDED>
    using AccessorRestrictI64 = AccessorRestrict<T, N, int64_t, StridesTrait>;
    template<typename T, size_t N, StridesTraits StridesTrait = StridesTraits::STRIDED>
    using AccessorRestrictI32 = AccessorRestrict<T, N, int32_t, StridesTrait>;
    template<typename T, size_t N, StridesTraits StridesTrait = StridesTraits::STRIDED>
    using AccessorRestrictU64 = AccessorRestrict<T, N, uint64_t, StridesTrait>;
    template<typename T, size_t N, StridesTraits StridesTrait = StridesTraits::STRIDED>
    using AccessorRestrictU32 = AccessorRestrict<T, N, uint32_t, StridesTrait>;

    template<typename T, size_t N, typename I, PointerTraits PointerTrait = PointerTraits::DEFAULT>
    using AccessorContiguous = Accessor<T, N, I, PointerTrait, StridesTraits::CONTIGUOUS>;
    template<typename T, size_t N, PointerTraits PointerTrait = PointerTraits::DEFAULT>
    using AccessorContiguousI64 = AccessorContiguous<T, N, int64_t, PointerTrait>;
    template<typename T, size_t N, PointerTraits PointerTrait = PointerTraits::DEFAULT>
    using AccessorContiguousI32 = AccessorContiguous<T, N, int32_t, PointerTrait>;
    template<typename T, size_t N, PointerTraits PointerTrait = PointerTraits::DEFAULT>
    using AccessorContiguousU64 = AccessorContiguous<T, N, uint64_t, PointerTrait>;
    template<typename T, size_t N, PointerTraits PointerTrait = PointerTraits::DEFAULT>
    using AccessorContiguousU32 = AccessorContiguous<T, N, uint32_t, PointerTrait>;

    template<typename T, size_t N, typename I>
    using AccessorRestrictContiguous = Accessor<T, N, I, PointerTraits::RESTRICT, StridesTraits::CONTIGUOUS>;
    template<typename T, size_t N>
    using AccessorRestrictContiguousI64 = AccessorRestrictContiguous<T, N, int64_t>;
    template<typename T, size_t N>
    using AccessorRestrictContiguousI32 = AccessorRestrictContiguous<T, N, int32_t>;
    template<typename T, size_t N>
    using AccessorRestrictContiguousU64 = AccessorRestrictContiguous<T, N, uint64_t>;
    template<typename T, size_t N>
    using AccessorRestrictContiguousU32 = AccessorRestrictContiguous<T, N, uint32_t>;

    template<typename T>
    using AccessorValueI64 = AccessorValue<T, int64_t>;
    template<typename T>
    using AccessorValueI32 = AccessorValue<T, int32_t>;
    template<typename T>
    using AccessorValueU64 = AccessorValue<T, uint64_t>;
    template<typename T>
    using AccessorValueU32 = AccessorValue<T, uint32_t>;
}

// Accessor, AccessorReference and AccessorValue are all accessors.
// AccessorReference is accessor_reference and AccessorValue is accessor_value.
// AccessorValue is accessor_restrict, accessor_contiguous and accessor_nd for any N.
namespace noa::traits {
    template<typename T> struct proclaim_is_accessor : std::false_type {};
    template<typename T> using is_accessor = std::bool_constant<proclaim_is_accessor<T>::value>;
    template<typename T> constexpr bool is_accessor_v = is_accessor<std::decay_t<T>>::value;
    template<typename... Ts> constexpr bool are_accessor_v = bool_and<is_accessor_v<Ts>...>::value;

    template<typename T> struct proclaim_is_accessor_restrict : std::false_type {};
    template<typename T> using is_accessor_restrict = std::bool_constant<proclaim_is_accessor_restrict<T>::value>;
    template<typename T> constexpr bool is_accessor_restrict_v = is_accessor_restrict<std::decay_t<T>>::value;
    template<typename... Ts> constexpr bool are_accessor_restrict_v = bool_and<is_accessor_restrict_v<Ts>...>::value;

    template<typename T> struct proclaim_is_accessor_contiguous : std::false_type {};
    template<typename T> using is_accessor_contiguous = std::bool_constant<proclaim_is_accessor_contiguous<T>::value>;
    template<typename T> constexpr bool is_accessor_contiguous_v = is_accessor_contiguous<std::decay_t<T>>::value;
    template<typename... Ts> constexpr bool are_accessor_contiguous_v = bool_and<is_accessor_contiguous_v<Ts>...>::value;

    template<typename T, size_t N> struct proclaim_is_accessor_nd : std::false_type {};
    template<typename T, size_t N> using is_accessor_nd = std::bool_constant<proclaim_is_accessor_nd<T, N>::value>;
    template<typename T, size_t N> constexpr bool is_accessor_nd_v = is_accessor_nd<std::decay_t<T>, N>::value;
    template<typename... Ts> constexpr bool are_accessor_nd_v = bool_and<is_accessor_contiguous_v<Ts>...>::value;

    template<typename T> constexpr bool is_accessor_1d_v = is_accessor_nd_v<T, 1>;
    template<typename T> constexpr bool is_accessor_2d_v = is_accessor_nd_v<T, 2>;
    template<typename T> constexpr bool is_accessor_3d_v = is_accessor_nd_v<T, 3>;
    template<typename T> constexpr bool is_accessor_4d_v = is_accessor_nd_v<T, 4>;

    template<typename T> struct proclaim_is_accessor_reference : std::false_type {};
    template<typename T> using is_accessor_reference = std::bool_constant<proclaim_is_accessor_reference<T>::value>;
    template<typename T> constexpr bool is_accessor_reference_v = is_accessor_reference<std::decay_t<T>>::value;
    template<typename... Ts> constexpr bool are_accessor_reference_v = bool_and<is_accessor_reference_v<Ts>...>::value;

    template<typename T> struct proclaim_is_accessor_value : std::false_type {};
    template<typename T> using is_accessor_value = std::bool_constant<proclaim_is_accessor_value<T>::value>;
    template<typename T> constexpr bool is_accessor_value_v = is_accessor_value<std::decay_t<T>>::value;
    template<typename... Ts> constexpr bool are_accessor_value_v = bool_and<is_accessor_value_v<Ts>...>::value;

    template<typename T> constexpr bool is_accessor_1d_restrict_contiguous_v =
            is_accessor_nd_v<T, 1> && is_accessor_restrict_v<T> && is_accessor_contiguous_v<T>;

    // ---

    template<typename T, size_t N, typename I, PointerTraits PointerTrait, StridesTraits StridesTrait>
    struct proclaim_is_accessor<Accessor<T, N, I, PointerTrait, StridesTrait>> : std::true_type {};

    template<typename T, size_t N, typename I, PointerTraits PointerTrait, StridesTraits StridesTrait>
    struct proclaim_is_accessor_restrict<Accessor<T, N, I, PointerTrait, StridesTrait>> : std::bool_constant<PointerTrait == PointerTraits::RESTRICT> {};

    template<typename T, size_t N, typename I, PointerTraits PointerTrait, StridesTraits StridesTrait>
    struct proclaim_is_accessor_contiguous<Accessor<T, N, I, PointerTrait, StridesTrait>> : std::bool_constant<StridesTrait == StridesTraits::CONTIGUOUS> {};

    template<typename T, size_t N1, typename I, PointerTraits PointerTrait, StridesTraits StridesTrait, size_t N2>
    struct proclaim_is_accessor_nd<Accessor<T, N1, I, PointerTrait, StridesTrait>, N2> : std::bool_constant<N1 == N2> {};

    template<typename T, size_t N, typename I, PointerTraits PointerTrait, StridesTraits StridesTrait>
    struct proclaim_is_accessor<AccessorReference<T, N, I, PointerTrait, StridesTrait>> : std::true_type {};

    template<typename T, size_t N, typename I, PointerTraits PointerTrait, StridesTraits StridesTrait>
    struct proclaim_is_accessor_restrict<AccessorReference<T, N, I, PointerTrait, StridesTrait>> : std::bool_constant<PointerTrait == PointerTraits::RESTRICT> {};

    template<typename T, size_t N, typename I, PointerTraits PointerTrait, StridesTraits StridesTrait>
    struct proclaim_is_accessor_contiguous<AccessorReference<T, N, I, PointerTrait, StridesTrait>> : std::bool_constant<StridesTrait == StridesTraits::CONTIGUOUS> {};

    template<typename T, size_t N1, typename I, PointerTraits PointerTrait, StridesTraits StridesTrait, size_t N2>
    struct proclaim_is_accessor_nd<AccessorReference<T, N1, I, PointerTrait, StridesTrait>, N2> : std::bool_constant<N1 == N2> {};

    template<typename T, size_t N, typename I, PointerTraits PointerTrait, StridesTraits StridesTrait>
    struct proclaim_is_accessor_reference<AccessorReference<T, N, I, PointerTrait, StridesTrait>> : std::true_type {};

    template<typename T, typename I>
    struct proclaim_is_accessor<AccessorValue<T, I>> : std::true_type {};

    template<typename T, typename I>
    struct proclaim_is_accessor_restrict<AccessorValue<T, I>> : std::true_type {};

    template<typename T, typename I>
    struct proclaim_is_accessor_contiguous<AccessorValue<T, I>> : std::true_type {};

    template<typename T, typename I, size_t N>
    struct proclaim_is_accessor_nd<AccessorValue<T, I>, N> : std::true_type {};

    template<typename T, typename I>
    struct proclaim_is_accessor_value<AccessorValue<T, I>> : std::true_type {};
}
