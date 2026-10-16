/*
    nanobind/eigen/dense.h: type casters for dense Eigen
    vectors and matrices

    Copyright (c) 2023 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include <nanobind/ndarray.h>
#include <Eigen/Core>

static_assert(EIGEN_VERSION_AT_LEAST(3, 3, 1),
              "Eigen matrix support in nanobind requires Eigen >= 3.3.1");

NAMESPACE_BEGIN(NB_NAMESPACE)

/// Function argument types that are compatible with various array flavors
using DStride = Eigen::Stride<Eigen::Dynamic, Eigen::Dynamic>;
template <typename T> using DRef = Eigen::Ref<T, 0, DStride>;
template <typename T> using DMap = Eigen::Map<T, 0, DStride>;

/// Variants with a fixed unit inner stride
using DStride1 = Eigen::Stride<Eigen::Dynamic, 1>;
template <typename T> using DRef1 = Eigen::Ref<T, 0, DStride1>;
template <typename T> using DMap1 = Eigen::Map<T, 0, DStride1>;

NAMESPACE_BEGIN(detail)

/// Determine the number of dimensions of the given Eigen type
template <typename T>
constexpr int ndim_v = bool(T::IsVectorAtCompileTime) ? 1 : 2;

/// Extract the compile-time strides of the given Eigen type
template <typename T> struct stride {
    using type = Eigen::Stride<0, 0>;
};

template <typename T, int Options, typename StrideType> struct stride<Eigen::Map<T, Options, StrideType>> {
    using type = StrideType;
};

template <typename T, int Options, typename StrideType> struct stride<Eigen::Ref<T, Options, StrideType>> {
    using type = StrideType;
};

template <typename T> using stride_t = typename stride<T>::type;

/** \brief Identify types with a contiguous memory representation.
 *
 * This includes all specializations of ``Eigen::Matrix``/``Eigen::Array`` and
 * certain specializations of ``Eigen::Map`` and ``Eigen::Ref``. Note: Eigen
 * interprets a compile-time stride of 0 as contiguous.
 */
template <typename T>
constexpr bool is_contiguous_v =
    (stride_t<T>::InnerStrideAtCompileTime == 0 ||
     stride_t<T>::InnerStrideAtCompileTime == 1) &&
    (ndim_v<T> == 1 || stride_t<T>::OuterStrideAtCompileTime == 0 ||
     (stride_t<T>::OuterStrideAtCompileTime != Eigen::Dynamic &&
      int(stride_t<T>::OuterStrideAtCompileTime) == int(T::InnerSizeAtCompileTime)));

/// Identify types with a static or dynamic layout that support contiguous storage
template <typename T>
constexpr bool can_map_contiguous_memory_v =
    (stride_t<T>::InnerStrideAtCompileTime == 0 ||
     stride_t<T>::InnerStrideAtCompileTime == 1 ||
     stride_t<T>::InnerStrideAtCompileTime == Eigen::Dynamic) &&
    (ndim_v<T> == 1 || stride_t<T>::OuterStrideAtCompileTime == 0 ||
     stride_t<T>::OuterStrideAtCompileTime == Eigen::Dynamic ||
     int(stride_t<T>::OuterStrideAtCompileTime) == int(T::InnerSizeAtCompileTime));

/* This type alias builds the most suitable 'ndarray' for the given Eigen type.
   In particular, it

  - matches the underlying scalar type
  - matches the number of dimensions (i.e. whether the type is a vector/matrix)
  - matches the shape (if the row/column count is known at compile time)
  - matches the in-memory ordering when the Eigen type is contiguous.

  This is helpful because type_caster<ndarray<..>> will then perform the
  necessary conversion steps (if given incompatible input) to enable data
  exchange with Eigen.

  A limitation of this approach is that ndarray does not support compile-time
  strides besides c_contig and f_contig. If an Eigen type requires
  non-contiguous strides (at compile-time) and we are given an ndarray with
  unsuitable strides (at run-time), type casting will fail. Note, however, that
  this is rather unusual, since the default stride type of Eigen::Map requires
  contiguous memory, and the one of Eigen::Ref requires a contiguous inner
  stride, while handling any outer stride.
*/

template <typename T, typename Scalar = typename T::Scalar>
using array_for_eigen_t = ndarray<
    Scalar,
    numpy,
    std::conditional_t<
        ndim_v<T> == 1,
        shape<T::SizeAtCompileTime>,
        shape<T::RowsAtCompileTime,
              T::ColsAtCompileTime>>,
    std::conditional_t<
        is_contiguous_v<T>,
        std::conditional_t<
            ndim_v<T> == 1 || T::IsRowMajor,
            c_contig,
            f_contig>,
        unused>>;

/// Any kind of Eigen class
template <typename T> constexpr bool is_eigen_v = is_base_of_template_v<T, Eigen::EigenBase>;

/// Detects Eigen::Array, Eigen::Matrix, etc.
template <typename T> constexpr bool is_eigen_plain_v = is_base_of_template_v<T, Eigen::PlainObjectBase>;

/// Detect Eigen::SparseMatrix
template <typename T> constexpr bool is_eigen_sparse_v = is_base_of_template_v<T, Eigen::SparseMatrixBase>;

/// Detects expression templates
template <typename T> constexpr bool is_eigen_xpr_v =
    is_eigen_v<T> && !is_eigen_plain_v<T> && !is_eigen_sparse_v<T> &&
    !std::is_base_of_v<Eigen::MapBase<T, Eigen::ReadOnlyAccessors>, T>;

template <typename T>
struct type_caster<T, enable_if_t<is_eigen_plain_v<T> &&
                                  is_ndarray_scalar_v<typename T::Scalar>>> {
    using Scalar = typename T::Scalar;
    using NDArray = array_for_eigen_t<T>;
    using NDArrayCaster = make_caster<NDArray>;

    NB_TYPE_CASTER(T, NDArrayCaster::Name)

    bool from_python(handle src, uint32_t flags, cleanup_list *cleanup) noexcept {
        // We're in any case making a copy, so non-writable inputs area also okay
        using NDArrayConst = array_for_eigen_t<T, const typename T::Scalar>;
        make_caster<NDArrayConst> caster;
        if (!caster.from_python(src, flags & ~cast_flags::accepts_none, cleanup))
            return false;

        const NDArrayConst &array = caster.value;
        if constexpr (ndim_v<T> == 1)
            value.resize((Eigen::Index) array.shape(0));
        else
            value.resize((Eigen::Index) array.shape(0),
                         (Eigen::Index) array.shape(1));

        // The layout is contiguous & compatible thanks to array_for_eigen_t<T>
        memcpy(value.data(), array.data(), array.size() * sizeof(Scalar));

        return true;
    }

    template <typename T2>
    static handle from_cpp(T2 &&v, rv_policy policy, cleanup_list *cleanup) noexcept {
        policy = infer_policy<T2>(policy);
        if constexpr (std::is_pointer_v<T2>)
            return from_cpp_internal((const T &) *v, policy, cleanup);
        else
            return from_cpp_internal((const T &) v, policy, cleanup);
    }

    static handle from_cpp_internal(const T &v, rv_policy policy, cleanup_list *cleanup) noexcept {
        size_t shape[ndim_v<T>];
        int64_t strides[ndim_v<T>];

        if constexpr (ndim_v<T> == 1) {
            shape[0] = (size_t) v.size();
            strides[0] = v.innerStride();
        } else {
            shape[0] = (size_t) v.rows();
            shape[1] = (size_t) v.cols();
            strides[0] = v.rowStride();
            strides[1] = v.colStride();
        }

        void *ptr = (void *) v.data();

        if (policy == rv_policy::move) {
            // Don't bother moving when the data is static or occupies <1KB
            if ((T::SizeAtCompileTime != Eigen::Dynamic ||
                 (size_t) v.size() < (1024 / sizeof(Scalar))))
                policy = rv_policy::copy;
        }

        object owner;
        if (policy == rv_policy::move) {
            T *temp = new T((T&&) v);
            owner = capsule(temp, [](void *p) noexcept { delete (T *) p; });
            ptr = temp->data();
            policy = rv_policy::reference;
        } else if (policy == rv_policy::reference_internal) {
            // reference_internal needs a self pointer; give up if unavailable
            if (!cleanup || !cleanup->self())
                return handle();
            owner = borrow(cleanup->self());
            policy = rv_policy::reference;
        }

        object o = steal(NDArrayCaster::from_cpp(
            NDArray(ptr, ndim_v<T>, shape, owner, strides),
            policy, cleanup));

        return o.release();
    }
};

/// Caster for Eigen expression templates
template <typename T>
struct type_caster<T, enable_if_t<is_eigen_xpr_v<T> &&
                                  is_ndarray_scalar_v<typename T::Scalar>>> {
    using Array = Eigen::Array<typename T::Scalar, T::RowsAtCompileTime,
                               T::ColsAtCompileTime>;
    using Caster = make_caster<Array>;
    static constexpr auto Name = Caster::Name;
    template <typename T_> using Cast = T;
    template <typename T_> static constexpr bool can_cast() { return true; }

    /// Generating an expression template from a Python object is, of course, not possible
    bool from_python(handle src, uint32_t flags, cleanup_list *cleanup) noexcept = delete;

    template <typename T2>
    static handle from_cpp(T2 &&v, rv_policy policy, cleanup_list *cleanup) noexcept {
        return Caster::from_cpp(std::forward<T2>(v), policy, cleanup);
    }
};

/** \brief Type caster for ``Eigen::Map<T>``

  The ``Eigen::Map<..>`` type exists to efficiently access memory provided by a
  caller. Given that, the nanobind type caster refuses to turn incompatible
  inputs into a ``Eigen::Map<T>`` when this would require an implicit
  conversion.
*/

template <typename T, int Options, typename StrideType>
struct type_caster<Eigen::Map<T, Options, StrideType>,
                   enable_if_t<is_eigen_plain_v<T> &&
                               is_ndarray_scalar_v<typename T::Scalar>>> {
    using Map = Eigen::Map<T, Options, StrideType>;
    using NDArray =
        array_for_eigen_t<Map, std::conditional_t<std::is_const_v<T>,
                                                  const typename Map::Scalar,
                                                  typename Map::Scalar>>;
    using NDArrayCaster = type_caster<NDArray>;
    static constexpr auto Name = NDArrayCaster::Name;
    template <typename T_> using Cast = Map;
    template <typename T_> static constexpr bool can_cast() { return true; }

    NDArrayCaster caster;

    bool from_python(handle src, uint32_t flags, cleanup_list *cleanup) noexcept {
        // Disable implicit conversions
        return from_python_(src, flags & ~cast_flags::convert, cleanup);
    }

    bool from_python_(handle src, uint32_t flags, cleanup_list* cleanup) noexcept {
        if (!caster.from_python(src, flags & ~cast_flags::accepts_none, cleanup))
            return false;

        // The `caster` already validated the shape, and for is_contiguous_v it
        // also validated the memory layout, but if not then we need to validate
        // against StrideType. Dynamic strides are compatible with any input, so
        // only compile-time strides need checking (and can be ignored when the
        // size in their dimension is <= 1).
        if constexpr (!is_contiguous_v<Map>) {
            constexpr size_t inner_dim = (ndim_v<T> == 2 && T::IsRowMajor) ? 1 : 0;
            constexpr size_t outer_dim = 1 - inner_dim;
            constexpr int64_t IS = StrideType::InnerStrideAtCompileTime;
            if constexpr (IS != Eigen::Dynamic) {
                int64_t actual_stride = caster.value.stride(inner_dim);
                // A compile-time stride of 0 implies "contiguous".
                constexpr int64_t expected_stride = IS == 0 ? 1 : IS;
                if (actual_stride != expected_stride &&
                        caster.value.shape(inner_dim) > 1)
                    return false;
            }
            constexpr int64_t OS = StrideType::OuterStrideAtCompileTime;
            if constexpr (ndim_v<T> == 2 && OS != Eigen::Dynamic) {
                int64_t actual_stride = caster.value.stride(outer_dim);
                // A compile-time stride of 0 implies "contiguous".
                int64_t expected_stride = OS == 0
                                            ? (int64_t) caster.value.shape(inner_dim)
                                            : OS;
                if (actual_stride != expected_stride &&
                        caster.value.shape(outer_dim) > 1)
                    return false;
            }
        }
        return true;
    }

    static handle from_cpp(const Map &v, rv_policy policy, cleanup_list *cleanup) noexcept {
        size_t shape[ndim_v<T>];
        int64_t strides[ndim_v<T>];

        if constexpr (ndim_v<T> == 1) {
            shape[0] = (size_t) v.size();
            strides[0] = v.innerStride();
        } else {
            shape[0] = (size_t) v.rows();
            shape[1] = (size_t) v.cols();
            strides[0] = v.rowStride();
            strides[1] = v.colStride();
        }

        return NDArrayCaster::from_cpp(
            NDArray((void *) v.data(), ndim_v<T>, shape, handle(), strides),
            (policy == rv_policy::automatic ||
             policy == rv_policy::automatic_reference)
                ? rv_policy::reference
                : policy,
            cleanup);
    }

    StrideType strides() const {
        constexpr int IS = StrideType::InnerStrideAtCompileTime,
                      OS = StrideType::OuterStrideAtCompileTime;
        if constexpr (std::is_same_v<StrideType, Eigen::InnerStride<IS>>)
            return StrideType(inner_stride());
        else if constexpr (std::is_same_v<StrideType, Eigen::OuterStride<OS>>)
            return StrideType(outer_stride());
        else
            return StrideType(outer_stride(), inner_stride());
    }

    operator Map() {
        NDArray &t = caster.value;
        if constexpr (ndim_v<T> == 1)
            return Map(t.data(), (Eigen::Index) t.shape(0), strides());
        else
            return Map(t.data(), (Eigen::Index) t.shape(0),
                       (Eigen::Index) t.shape(1), strides());
    }

private:
    Eigen::Index inner_stride() const {
        if constexpr (StrideType::InnerStrideAtCompileTime == Eigen::Dynamic)
            if constexpr (ndim_v<T> == 1)
                return (Eigen::Index) caster.value.stride(0);
            else if constexpr (T::IsRowMajor)
                return (Eigen::Index) caster.value.stride(1);
            else
                return (Eigen::Index) caster.value.stride(0);
        else
            return StrideType::InnerStrideAtCompileTime;
    }

    Eigen::Index outer_stride() const {
        if constexpr (StrideType::OuterStrideAtCompileTime == Eigen::Dynamic)
            if constexpr (ndim_v<T> == 1)
                return (Eigen::Index) caster.value.shape(0);
            else if constexpr (T::IsRowMajor)
                return (Eigen::Index) caster.value.stride(0);
            else
                return (Eigen::Index) caster.value.stride(1);
        else
            return StrideType::OuterStrideAtCompileTime;
    }
};

/** \brief Caster for Eigen::Ref<T>

  Compared to the ``Eigen::Map<T>`` type caster above, the reference caster
  accepts a wider set of inputs when it is used in *constant reference* mode
  (i.e., ``Eigen::Ref<const T>``). In this case, it performs stride conversions
  (except for unusual non-contiguous strides) as well as conversions of the
  underlying scalar type (if implicit conversions are enabled).

  For non-constant references, the caster matches that of ``Eigen::Map<T>`` and
  requires an input with the expected layout (so that changes can propagate to
  the caller).
*/
template <typename T, int Options, typename StrideType>
struct type_caster<Eigen::Ref<T, Options, StrideType>,
                   enable_if_t<is_eigen_plain_v<T> &&
                               is_ndarray_scalar_v<typename T::Scalar>>> {
    using Ref = Eigen::Ref<T, Options, StrideType>;

    /// Potentially convert strides/dtype when casting constant references
    static constexpr bool MaybeConvert =
        std::is_const_v<T> &&
        // Restrict to contiguous 'T' (limitation in Eigen, see PR #215)
        can_map_contiguous_memory_v<Ref>;

    using NDArray =
        array_for_eigen_t<Ref, std::conditional_t<std::is_const_v<T>,
                                                  const typename Ref::Scalar,
                                                  typename Ref::Scalar>>;
    using NDArrayCaster = type_caster<NDArray>;

    /// Eigen::Map<T> caster with fixed strides
    using Map = Eigen::Map<T, Options, StrideType>;
    using MapCaster = make_caster<Map>;

    // Extended version taking arbitrary strides
    using DMap = Eigen::Map<const T, Options, DStride>;
    using DMapCaster = make_caster<DMap>;

    /**
     * The constructor of ``Ref<const T>`` uses one of two strategies
     * depending on the input. It may either
     *
     * 1. Create a copy ``Ref<const T>::m_object`` (owned by Ref), or
     * 2. Reference the existing input (non-owned).
     *
     * When the value below is ``true``, then it is guaranteed that
     * ``Ref(<DMap instance>)`` owns the underlying data.
     */
    static constexpr bool DMapConstructorOwnsData =
        !Eigen::internal::traits<Ref>::template match<DMap>::type::value;

    static constexpr auto Name =
        const_name<MaybeConvert>(DMapCaster::Name, MapCaster::Name);

    template <typename T_> using Cast = Ref;
    template <typename T_> static constexpr bool can_cast() { return true; }

    MapCaster caster;
    struct Empty { };
    std::conditional_t<MaybeConvert, DMapCaster, Empty> dcaster;

    bool from_python(handle src, uint32_t flags, cleanup_list *cleanup) noexcept {
        // Try a direct cast without implicit conversion first
        if (caster.from_python(src, flags, cleanup))
            return true;

        // Potentially convert strides/dtype when casting constant references
        if constexpr (MaybeConvert) {
            // Generating an implicit copy requires some object to assume
            // ownership. During a function call, ``dcaster`` can serve that
            // role (this case is detected by checking whether ``flags`` has
            // the ``manual`` flag set). When used in other situations (e.g.
            // ``nb::cast()``), the created ``Eigen::Ref<..>`` must take
            // ownership of the copy. This is only guranteed to work if
            // DMapConstructorOwnsData.
            //
            // If neither of these is possible, we disable implicit
            // conversions.

            if ((flags & cast_flags::manual) &&
                !DMapConstructorOwnsData)
                flags &= ~cast_flags::convert;

            if (dcaster.from_python_(src, flags, cleanup))
                return true;
        }

        return false;
    }

    static handle from_cpp(const Ref &v, rv_policy policy, cleanup_list *cleanup) noexcept {
        // Copied from the Eigen::Map caster

        size_t shape[ndim_v<T>];
        int64_t strides[ndim_v<T>];

        if constexpr (ndim_v<T> == 1) {
            shape[0] = (size_t) v.size();
            strides[0] = v.innerStride();
        } else {
            shape[0] = (size_t) v.rows();
            shape[1] = (size_t) v.cols();
            strides[0] = v.rowStride();
            strides[1] = v.colStride();
        }

        return NDArrayCaster::from_cpp(
            NDArray((void *) v.data(), ndim_v<T>, shape, handle(), strides),
            (policy == rv_policy::automatic ||
             policy == rv_policy::automatic_reference)
                ? rv_policy::reference
                : policy,
            cleanup);
    }

    operator Ref() {
        if constexpr (MaybeConvert) {
            if (dcaster.caster.value.is_valid())
                return Ref(dcaster.operator DMap());
        }

        return Ref(caster.operator Map());
    }
};

NAMESPACE_END(detail)

NAMESPACE_END(NB_NAMESPACE)
