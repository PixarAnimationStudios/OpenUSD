/*
    nanobind/ndarray.h: functionality to exchange n-dimensional arrays with
    other array programming frameworks (NumPy, PyTorch, etc.)

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.

    The API below is based on the DLPack project
    (https://github.com/dmlc/dlpack/blob/main/include/dlpack/dlpack.h)
*/

#pragma once

#include <nanobind/nanobind.h>
#include <initializer_list>

NAMESPACE_BEGIN(NB_NAMESPACE)

/// DLPack API/ABI data structures are part of a separate namespace.
NAMESPACE_BEGIN(dlpack)

// The version of DLPack that is supported by libnanobind
static constexpr uint32_t major_version = 1;
static constexpr uint32_t minor_version = 1;

enum class dtype_code : uint8_t {
    Int = 0, UInt = 1, Float = 2, Bfloat = 4, Complex = 5, Bool = 6,
    Float8_E3M4 = 7, Float8_E4M3 = 8, Float8_E4M3B11FNUZ = 9,
    Float8_E4M3FN = 10, Float8_E4M3FNUZ = 11, Float8_E5M2 = 12,
    Float8_E5M2FNUZ = 13, Float8_E8M0FNU = 14,
    Float6_E2M3FN = 15, Float6_E3M2FN = 16,
    Float4_E2M1FN = 17,
    Bcomplex = 18
};

struct device {
    int32_t device_type = 0;
    int32_t device_id = 0;
};

struct dtype {
    uint8_t code = 0;
    uint8_t bits = 0;
    uint16_t lanes = 0;

    constexpr bool operator==(const dtype &o) const {
        return code == o.code && bits == o.bits && lanes == o.lanes;
    }

    constexpr bool operator!=(const dtype &o) const { return !operator==(o); }
};

struct dltensor {
    void *data = nullptr;
    nanobind::dlpack::device device;
    int32_t ndim = 0;
    nanobind::dlpack::dtype dtype;
    int64_t *shape = nullptr;
    int64_t *strides = nullptr;
    uint64_t byte_offset = 0;
};

NAMESPACE_END(dlpack)

#define NB_FRAMEWORK(Name, Value, label)                                       \
    struct Name {                                                              \
        static constexpr auto name = detail::const_name(label);                \
        static constexpr int value = Value;                                    \
        static constexpr bool is_framework = true;                             \
    }

#define NB_DEVICE(Name, Value)                                                 \
    struct Name {                                                              \
        static constexpr auto name = detail::const_name("device='" #Name "'"); \
        static constexpr int value = Value;                                    \
        static constexpr bool is_device_type = true;                           \
    }

#define NB_ORDER(Name, Value)                                                  \
    struct Name {                                                              \
        static constexpr auto name = detail::const_name("order='" Value "'");  \
        static constexpr char value = Value[0];                                \
        static constexpr bool is_order = true;                                 \
    }

NB_ORDER(c_contig, "C");
NB_ORDER(f_contig, "F");
NB_ORDER(any_contig, "A");

NB_FRAMEWORK(no_framework, 0, "ndarray");
NB_FRAMEWORK(numpy, 1, "numpy.ndarray");
NB_FRAMEWORK(pytorch, 2, "torch.Tensor");
NB_FRAMEWORK(tensorflow, 3, "tensorflow.python.framework.ops.EagerTensor");
NB_FRAMEWORK(jax, 4, "jaxlib._jax.ArrayImpl");
NB_FRAMEWORK(cupy, 5, "cupy.ndarray");
NB_FRAMEWORK(memview, 6, "memoryview");
NB_FRAMEWORK(array_api, 7, "ArrayLike");
NB_FRAMEWORK(mlx, 8, "mlx.core.array");

NAMESPACE_BEGIN(device)
NB_DEVICE(none, 0); NB_DEVICE(cpu, 1); NB_DEVICE(cuda, 2);
NB_DEVICE(cuda_host, 3); NB_DEVICE(opencl, 4); NB_DEVICE(vulkan, 7);
NB_DEVICE(metal, 8); NB_DEVICE(rocm, 10); NB_DEVICE(rocm_host, 11);
NB_DEVICE(cuda_managed, 13); NB_DEVICE(oneapi, 14);
NAMESPACE_END(device)

#undef NB_FRAMEWORK
#undef NB_DEVICE
#undef NB_ORDER

NAMESPACE_BEGIN(detail)

template <typename T, typename /* SFINAE */ = int> struct dtype_traits {
    static constexpr bool is_complex = is_complex_v<T>;
    static constexpr bool is_float   = std::is_floating_point_v<T>;
    static constexpr bool is_bool    = std::is_same_v<std::remove_cv_t<T>, bool>;
    static constexpr bool is_int     = std::is_integral_v<T> && !is_bool;
    static constexpr bool is_signed  = std::is_signed_v<T>;

    static constexpr int matches = is_bool + is_complex + is_float + is_int;
    static_assert(matches <= 1, "dtype matches multiple type categories!");

    static constexpr dlpack::dtype value{
        (uint8_t) ((is_bool ? (int) dlpack::dtype_code::Bool : 0) +
                   (is_complex ? (int) dlpack::dtype_code::Complex : 0) +
                   (is_float ? (int) dlpack::dtype_code::Float : 0) +
                   (is_int &&  is_signed ? (int) dlpack::dtype_code::Int : 0) +
                   (is_int && !is_signed ? (int) dlpack::dtype_code::UInt : 0)),
        (uint8_t) matches ? sizeof(T) * 8 : 0,
        matches ? 1 : 0
    };

    static constexpr auto name =
        const_name<is_complex>("complex", "") +
        const_name<is_int &&  is_signed>("int", "") +
        const_name<is_int && !is_signed>("uint", "") +
        const_name<is_float>("float", "") +
        const_name<is_bool>(const_name("bool"), const_name<sizeof(T) * 8>());
};

template <> struct dtype_traits<void> {
    static constexpr dlpack::dtype value{ 0, 0, 0 };
    static constexpr auto name = descr<0>();
};

template <typename T> struct dtype_traits<const T> {
    static constexpr dlpack::dtype value = dtype_traits<T>::value;
    static constexpr auto name = dtype_traits<T>::name;
};

template <ssize_t... Is> struct shape {
    static constexpr auto name =
        const_name("shape=(") +
        concat(const_name<Is == -1>(const_name("*"),
                                    const_name<(size_t) Is>())...) + const_name(")");
    static_assert(
        ((Is >= 0 || Is == -1) && ...),
        "The arguments to nanobind::shape must either be positive or equal to -1"
    );

    static void put(int64_t *out) {
        size_t ctr = 0;
        ((out[ctr++] = Is), ...);
    }

    static void put(size_t *out) {
        if constexpr (((Is == -1) || ...)) {
            fprintf(stderr, "Critical nanobind error: negative ndarray "
                            "sizes are not allowed here!\n");
            abort();
        } else {
            size_t ctr = 0;
            ((out[ctr++] = (size_t) Is), ...);
        }
    }
};

template <typename T>
constexpr bool is_ndarray_scalar_v = dtype_traits<T>::value.bits != 0;

template <typename> struct ndim_shape;
template <size_t... S> struct ndim_shape<std::index_sequence<S...>> {
    using type = shape<((void) S, -1)...>;
};

NAMESPACE_END(detail)

using detail::shape;

struct ro { };

template <size_t N>
using ndim = typename detail::ndim_shape<std::make_index_sequence<N>>::type;

template <typename T> constexpr dlpack::dtype dtype() {
    return detail::dtype_traits<T>::value;
}

NAMESPACE_BEGIN(detail)

/// Sentinel type to initialize ndarray_config_t<>
struct unused {
    using type = void;
    static constexpr int value = 0;
    static constexpr auto name = descr<0>();
};

/**
 * \brief ndarray_config describes a requested array configuration, which the
 * backend reads through \ref ndarray_import().
 *
 * The layout of this data structure is part of the backend ABI contract.
 */
struct ndarray_config {
    /// Optional constraints (none yet) and the ABI tag in the top 8 bits
    uint32_t flags;

    /// Requested DLPack device type (0: any)
    int32_t device_type;

    /// Requested dimension count (-1: any)
    int32_t ndim;

    /// Requested element type (a zero 'bits' field means any)
    dlpack::dtype dtype;

    /// Requested memory order ('C', 'F', 'A', or '\0' for any)
    char order;

    /// Does the binding access the array through a const pointer?
    bool ro;

    /// Unused, claimable by a later ABI minor
    uint16_t unused_0;
    uint32_t unused_1;

    /// Requested extents ('ndim' entries, -1 marks a free dimension)
    const int64_t *shape;

    template <typename T> ndarray_config(T)
        : flags(NB_ABI_MINOR_TAG),
          device_type(T::DeviceType::value), ndim(T::N),
          dtype(nanobind::dtype<typename T::Scalar>()),
          order((char) T::Order::value),
          ro(std::is_const_v<typename T::Scalar>), shape(nullptr) { }
};

/// ndarray_create_args describes an array that a binding hands to Python,
/// which the backend reads through ndarray_create()
struct ndarray_create_args {
    /// Pointer to the array contents
    void *data;

    /// Extents of the array ('ndim' entries)
    const size_t *shape;

    /// Strides in units of elements ('ndim' entries). When null, they are
    /// derived from 'shape' and 'order'.
    const int64_t *strides;

    /// Python object owning the data, if any
    PyObject *owner;

    /// Offset of the first element in bytes
    uint64_t byte_offset;

    /// Number of dimensions
    uint32_t ndim;

    /// Optional fields (none yet) and the ABI tag in the top 8 bits
    uint32_t flags;

    /// DLPack device type (0: the CPU device)
    int32_t device_type;

    /// Index of the device within its device type
    int32_t device_id;

    /// Element type
    dlpack::dtype dtype;

    /// Memory order used to derive missing strides ('C', 'F', 'A', or '\0')
    char order;

    /// Is the array read-only?
    bool ro;

    /// Unused, claimable by a later ABI minor
    uint16_t unused;
};

/// ndarray_config_t collects nd-array template parameters in a structured way.
/// Its "storage" is purely based on types members
template <typename /* SFINAE */ = int, typename...> struct ndarray_config_t;

template <> struct ndarray_config_t<int> {
    using Framework = no_framework;
    using Scalar = void;
    using Shape = unused;
    using Order = unused;
    using DeviceType = unused;
    static constexpr int32_t N = -1;
};

// Template infrastructure to collect ndarray annotations and fail if duplicates are found
template <typename... Args> struct ndarray_config_t<int, ro, Args...> : ndarray_config_t<int, Args...> {
    using Scalar = std::add_const_t<typename ndarray_config_t<int, Args...>::Scalar>;
};

template <typename... Args> struct ndarray_config_t<int, unused, Args...> : ndarray_config_t<int, Args...> { };

template <typename Arg, typename... Args> struct ndarray_config_t<enable_if_t<is_ndarray_scalar_v<Arg>>, Arg, Args...> : ndarray_config_t<int, Args...> {
    using Scalar = std::conditional_t<
        std::is_const_v<typename ndarray_config_t<int, Args...>::Scalar>,
        std::add_const_t<Arg>, Arg>;
};

template <typename Arg, typename... Args> struct ndarray_config_t<enable_if_t<Arg::is_device_type>, Arg, Args...> : ndarray_config_t<int, Args...> {
    using DeviceType = Arg;
};

template <typename Arg, typename... Args> struct ndarray_config_t<enable_if_t<Arg::is_framework>, Arg, Args...> : ndarray_config_t<int, Args...> {
    using Framework = Arg;
};

template <typename Arg, typename... Args> struct ndarray_config_t<enable_if_t<Arg::is_order>, Arg, Args...> : ndarray_config_t<int, Args...> {
    using Order = Arg;
};

template <ssize_t... Is, typename... Args> struct ndarray_config_t<int, shape<Is...>, Args...> : ndarray_config_t<int, Args...> {
    using Shape = shape<Is...>;
    static constexpr int32_t N = sizeof...(Is);
};

NAMESPACE_END(detail)

template <typename Scalar, size_t Dim, char Order> struct ndarray_view {
    ndarray_view() = default;
    ndarray_view(const ndarray_view &) = default;
    ndarray_view(ndarray_view &&) = default;
    ndarray_view &operator=(const ndarray_view &) = default;
    ndarray_view &operator=(ndarray_view &&) noexcept = default;
    ~ndarray_view() noexcept = default;

    template <typename... Args> NB_INLINE Scalar &operator()(Args... indices) const {
        static_assert(
            sizeof...(Args) == Dim,
            "ndarray_view::operator(): invalid number of arguments");

        const int64_t indices_i64[] { (int64_t) indices... };
        int64_t offset = 0;
        for (size_t i = 0; i < Dim; ++i)
            offset += indices_i64[i] * m_strides[i];

        return *(m_data + offset);
    }

    size_t ndim() const { return Dim; }
    size_t shape(size_t i) const { return (size_t) m_shape[i]; }
    int64_t stride(size_t i) const { return m_strides[i]; }
    Scalar *data() const { return m_data; }

private:
    template <typename...> friend class ndarray;

    template <size_t... I1, ssize_t... I2>
    ndarray_view(Scalar *data, const int64_t *shape, const int64_t *strides,
                 std::index_sequence<I1...>, nanobind::shape<I2...>)
        : m_data(data) {

        // Initialize shape/strides with compile-time knowledge if
        // available (to permit vectorization, loop unrolling, etc.)
        ((m_shape[I1] = (I2 == -1) ? shape[I1] : (int64_t) I2), ...);
        ((m_strides[I1] = strides[I1]), ...);

        if constexpr (Order == 'F') {
            m_strides[0] = 1;
            for (size_t i = 1; i < Dim; ++i)
                m_strides[i] = m_strides[i - 1] * m_shape[i - 1];
        } else if constexpr (Order == 'C') {
            m_strides[Dim - 1] = 1;
            for (Py_ssize_t i = (Py_ssize_t) Dim - 2; i >= 0; --i)
                m_strides[i] = m_strides[i + 1] * m_shape[i + 1];
        }
    }

    Scalar *m_data = nullptr;
    int64_t m_shape[Dim] { };
    int64_t m_strides[Dim] { };
};


template <typename... Args> class ndarray {
public:
    template <typename...> friend class ndarray;
    template <typename, typename> friend struct detail::type_caster;

    using Config = detail::ndarray_config_t<int, Args...>;
    using Scalar = typename Config::Scalar;
    static constexpr bool ReadOnly = std::is_const_v<Scalar>;
    static constexpr char Order = Config::Order::value;
    static constexpr int DeviceType = Config::DeviceType::value;
    using VoidPtr = std::conditional_t<ReadOnly, const void *, void *>;

    ndarray() = default;

    explicit ndarray(detail::ndarray_handle *handle) : m_handle(handle) {
        if (handle)
            m_dltensor = *NB_CALL(ndarray_inc_ref)(handle);
    }

    template <typename... Args2>
    explicit ndarray(const ndarray<Args2...> &other) : ndarray(other.m_handle) { }

    ndarray(VoidPtr data,
            size_t ndim,
            const size_t *shape,
            handle owner = { },
            const int64_t *strides = nullptr,
            dlpack::dtype dtype = nanobind::dtype<Scalar>(),
            int device_type = DeviceType,
            int device_id = 0,
            char order = Order,
            uint64_t byte_offset = 0) {

        detail::ndarray_create_args args;
        args.flags = NB_ABI_MINOR_TAG;
        args.data = (void *) data;
        args.shape = shape;
        args.strides = strides;
        args.owner = owner.ptr();
        args.ndim = (uint32_t) ndim;
        args.byte_offset = byte_offset;
        args.device_type = device_type;
        args.device_id = device_id;
        args.dtype = dtype;
        args.order = order;
        args.ro = ReadOnly;

        m_handle = NB_CALL(ndarray_create)(NB_CTX, &args);
        m_dltensor = *NB_CALL(ndarray_inc_ref)(m_handle);
    }

    ndarray(VoidPtr data,
            std::initializer_list<size_t> shape = { },
            handle owner = { },
            std::initializer_list<int64_t> strides = { },
            dlpack::dtype dtype = nanobind::dtype<Scalar>(),
            int device_type = DeviceType,
            int device_id = 0,
            char order = Order,
            uint64_t byte_offset = 0) {

        size_t shape_size = shape.size();

        if (strides.size() != 0 && strides.size() != shape_size) {
            fprintf(stderr, "Critical nanobind error: ndarray(): shape and "
                            "strides have incompatible size!\n");
            abort();
        }

        size_t shape_buf[Config::N <= 0 ? 1 : Config::N];
        const size_t *shape_ptr = shape.begin();

        if constexpr (Config::N > 0) {
            if (!shape_size) {
                Config::Shape::put(shape_buf);
                shape_size = Config::N;
                shape_ptr = shape_buf;
            }
        } else {
            (void) shape_buf;
        }

        detail::ndarray_create_args args;
        args.flags = NB_ABI_MINOR_TAG;
        args.data = (void *) data;
        args.shape = shape_ptr;
        args.strides = (strides.size() == 0) ? nullptr : strides.begin();
        args.owner = owner.ptr();
        args.ndim = (uint32_t) shape_size;
        args.byte_offset = byte_offset;
        args.device_type = device_type;
        args.device_id = device_id;
        args.dtype = dtype;
        args.order = order;
        args.ro = ReadOnly;

        m_handle = NB_CALL(ndarray_create)(NB_CTX, &args);
        m_dltensor = *NB_CALL(ndarray_inc_ref)(m_handle);
    }

    ~ndarray() {
        NB_CALL(ndarray_dec_ref)(m_handle);
    }

    ndarray(const ndarray &t) : m_handle(t.m_handle), m_dltensor(t.m_dltensor) {
        NB_CALL(ndarray_inc_ref)(m_handle);
    }

    ndarray(ndarray &&t) noexcept : m_handle(t.m_handle), m_dltensor(t.m_dltensor) {
        // Only reset m_handle, it's safe to leave m_dltensor as-is
        t.m_handle = nullptr;
    }

    ndarray &operator=(ndarray &&t) noexcept {
        NB_CALL(ndarray_dec_ref)(m_handle);
        m_handle = t.m_handle;
        m_dltensor = t.m_dltensor;
        // Only reset t.m_handle, it's safe to leave t.m_dltensor as-is
        t.m_handle = nullptr;
        return *this;
    }

    ndarray &operator=(const ndarray &t) {
        NB_CALL(ndarray_inc_ref)(t.m_handle);
        NB_CALL(ndarray_dec_ref)(m_handle);
        m_handle = t.m_handle;
        m_dltensor = t.m_dltensor;
        return *this;
    }

    dlpack::dtype dtype() const { return m_dltensor.dtype; }
    size_t ndim() const { return (size_t) m_dltensor.ndim; }
    size_t shape(size_t i) const { return (size_t) m_dltensor.shape[i]; }
    int64_t stride(size_t i) const { return m_dltensor.strides[i]; }
    const int64_t* shape_ptr() const { return m_dltensor.shape; }
    const int64_t* stride_ptr() const { return m_dltensor.strides; }
    bool is_valid() const { return m_handle != nullptr; }
    int device_type() const { return (int) m_dltensor.device.device_type; }
    int device_id() const { return (int) m_dltensor.device.device_id; }
    void *data_handle() const { return m_dltensor.data; }
    uint64_t byte_offset() const { return m_dltensor.byte_offset; }
    detail::ndarray_handle *handle() const { return m_handle; }

    size_t size() const {
        size_t ret = is_valid();
        for (size_t i = 0; i < ndim(); ++i)
            ret *= shape(i);
        return ret;
    }

    size_t itemsize() const { return ((size_t) dtype().bits + 7) / 8; }
    size_t nbytes() const { return ((size_t) dtype().bits * size() + 7) / 8; }

    Scalar *data() const {
        return (Scalar *) ((uint8_t *) m_dltensor.data +
                           m_dltensor.byte_offset);
    }

    template <typename... Args2>
    NB_INLINE auto& operator()(Args2... indices) const {
        return *(Scalar *) ((uint8_t *) m_dltensor.data +
                            compute_byte_offset(indices...));
    }

    template <typename... Args2> NB_INLINE auto view() const {
        using namespace detail;
        using Config2 = detail::ndarray_config_t<int, Args2..., Args...>;
        using Scalar2 = typename Config2::Scalar;
        constexpr size_t N = Config2::N >= 0 ? Config2::N : 0;

        constexpr bool has_scalar = !std::is_void_v<Scalar2>,
                       has_shape  = Config2::N >= 0;

        static_assert(has_scalar,
            "To use the ndarray::view<..>() method, you must add a scalar type "
            "annotation (e.g. 'float') to the template parameters of the parent "
            "ndarray, or to the call to .view<..>()");

        static_assert(has_shape,
            "To use the ndarray::view<..>() method, you must add a shape<..> "
            "or ndim<..> annotation to the template parameters of the parent "
            "ndarray, or to the call to .view<..>()");

        if constexpr (has_scalar && has_shape) {
            using Result = ndarray_view<Scalar2, N, Config2::Order::value>;
            return Result((Scalar2 *) data(), shape_ptr(), stride_ptr(),
                          std::make_index_sequence<N>(),
                          typename Config2::Shape());
        } else {
            return nullptr;
        }
    }

    auto cast(rv_policy rvp = rv_policy::automatic, class handle parent = {});

private:
    template <typename... Args2>
    NB_INLINE int64_t compute_byte_offset(Args2... indices) const {
        constexpr bool has_scalar = !std::is_void_v<Scalar>,
                       has_shape  = Config::N != -1;

        static_assert(has_scalar,
            "To use ndarray::operator(), you must add a scalar type "
            "annotation (e.g. 'float') to the ndarray template parameters.");

        static_assert(has_shape,
            "To use ndarray::operator(), you must add a shape<> or "
            "ndim<> annotation to the ndarray template parameters.");

        if constexpr (has_scalar && has_shape) {
            static_assert(sizeof...(Args2) == (size_t) Config::N,
                          "ndarray::operator(): invalid number of arguments");

            size_t counter = 0;
            int64_t index = 0;
            ((index += int64_t(indices) * m_dltensor.strides[counter++]), ...);

            return (int64_t) m_dltensor.byte_offset + index * (int64_t) sizeof(Scalar);
        } else {
            return 0;
        }
    }

    detail::ndarray_handle *m_handle = nullptr;
    dlpack::dltensor m_dltensor;
};

inline bool ndarray_check(handle h) { return NB_CALL(ndarray_check)(NB_CTX, h.ptr()); }

NAMESPACE_BEGIN(detail)

template <typename T> struct dtype_name {
    static constexpr auto name = detail::const_name("dtype=") + dtype_traits<T>::name;
};

template <> struct dtype_name<void> : unused { };
template <> struct dtype_name<const void> : unused { };

template <typename T> struct dtype_const_name {
    static constexpr auto name = const_name<std::is_const_v<T>>("writable=False", "");
};

template <typename... Args> struct type_caster<ndarray<Args...>> {
    using Config = detail::ndarray_config_t<int, Args...>;
    using Scalar = typename Config::Scalar;

    NB_TYPE_CASTER(ndarray<Args...>,
                   Config::Framework::name +
                   const_name("[") +
                       concat_maybe(dtype_name<Scalar>::name,
                                    Config::Shape::name,
                                    Config::Order::name,
                                    Config::DeviceType::name,
                                    dtype_const_name<Scalar>::name) +
                   const_name("]"))

    bool from_python(handle src, uint32_t flags, cleanup_list *cleanup) noexcept {
        if (src.is_none() && flags & cast_flags::accepts_none) {
            value = ndarray<Args...>();
            return true;
        }

        int64_t shape_buf[Config::N <= 0 ? 1 : Config::N];
        ndarray_config config{Config()};

        if constexpr (Config::N > 0) {
            Config::Shape::put(shape_buf);
            config.shape = shape_buf;
        } else {
            (void) shape_buf;
        }

        detail::ndarray_handle *h = NB_CALL(ndarray_import)(NB_CTX_C(cleanup),
            src.ptr(), &config, flags & cast_flags::convert,
            cleanup);

        if (NB_UNLIKELY(value.m_handle))
            NB_CALL(ndarray_dec_ref)(value.m_handle);
        if (NB_LIKELY(h))
            value.m_dltensor = *NB_CALL(ndarray_inc_ref)(h);
        value.m_handle = h;
        return h != nullptr;
    }

    static handle from_cpp(const ndarray<Args...> &tensor, rv_policy policy,
                           cleanup_list *cleanup) noexcept {
        return NB_CALL(ndarray_export)(NB_CTX_C(cleanup), tensor.handle(),
                                       Config::Framework::value, policy,
                                       cleanup);
    }
};

template <typename... Args>
class ndarray_object : public object {
public:
    using object::object;
    using object::operator=;
    static constexpr auto Name = type_caster<ndarray<Args...>>::Name;
};

NAMESPACE_END(detail)

template <typename... Args>
auto ndarray<Args...>::cast(rv_policy rvp, class handle parent) {
    return borrow<detail::ndarray_object<Args...>>(
        nanobind::cast(*this, rvp, parent));
}

NAMESPACE_END(NB_NAMESPACE)
