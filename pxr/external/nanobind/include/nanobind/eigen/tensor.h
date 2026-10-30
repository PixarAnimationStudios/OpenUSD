/*
    nanobind/eigen/tensor.h: type casters for Eigen tensors

    Copyright (c) 2026 INRIA

    Author(s): Wilson Jallet

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include <nanobind/ndarray.h>
#include <unsupported/Eigen/CXX11/Tensor>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

/// As of April 2026, Eigen::Tensor types support 16-byte alignment or no alignment.
inline bool is_tensor_aligned(const void *data, std::size_t align = Eigen::Aligned) {
    return (reinterpret_cast<std::size_t>(data) % align) == 0;
}

/// Type trait for inheriting from Eigen::TensorBase.
/// All TensorBase specializations inherit from TensorBase<T, ReadOnlyAccessors>.
template<typename T> constexpr bool is_eigen_tensor_v = 
    std::is_base_of_v<Eigen::TensorBase<T, Eigen::ReadOnlyAccessors>, T>;

template<typename T> constexpr bool eigen_tensor_is_row_major_v = T::Layout == Eigen::RowMajor;
template<typename T> constexpr bool eigen_tensor_is_col_major_v = T::Layout == Eigen::ColMajor;

template<typename T>
constexpr bool is_eigen_tensor_map_v = false;

// Covers const case
template<typename T, int Options, template<class> class MakePointer>
constexpr bool is_eigen_tensor_map_v<Eigen::TensorMap<T, Options, MakePointer>> = true;

template<typename T>
constexpr bool is_eigen_tensor_ref_v = false;

// Covers const case
template<typename T>
constexpr bool is_eigen_tensor_ref_v<Eigen::TensorRef<T>> = true;

template<typename T>
constexpr bool is_eigen_tensor_plain_v = false;

template<typename Scalar, int NumIndices, int Options, typename IndexType>
constexpr bool is_eigen_tensor_plain_v<Eigen::Tensor<Scalar, NumIndices, Options, IndexType>> = true;

template<typename Scalar, std::ptrdiff_t... Indices, int Options, typename IndexType>
constexpr bool is_eigen_tensor_plain_v<Eigen::TensorFixedSize<Scalar, Eigen::Sizes<Indices...>, Options, IndexType>> = true;

template<typename T>
constexpr bool is_eigen_tensor_xpr_v = 
    is_eigen_tensor_v<T> &&
    !is_eigen_tensor_plain_v<T> &&
    !is_eigen_tensor_map_v<T> &&
    !is_eigen_tensor_ref_v<T>;

template<typename T, typename Scalar = typename T::Scalar>
using ndarray_for_eigen_tensor_t = ndarray<
    Scalar,
    numpy,
    ndim<T::NumDimensions>, 
    std::conditional_t<
        eigen_tensor_is_row_major_v<T>,
        c_contig,
        f_contig>>;

/** \brief Type caster for ``Eigen::TensorMap<T>``
 */
template<typename T, int MapOptions, template<class> class MakePointer>
struct type_caster<
    Eigen::TensorMap<T, MapOptions, MakePointer>,
    enable_if_t<is_ndarray_scalar_v<typename T::Scalar>>> {

    using Scalar = typename T::Scalar;
    using IndexType = typename T::Index;
    static constexpr int NumIndices = T::NumIndices;
    static constexpr int Options = T::Options;
    using PlainTensor = Eigen::Tensor<Scalar, NumIndices, Options, IndexType>;
    using Dimensions = typename T::Dimensions;
    using MapType = Eigen::TensorMap<T, MapOptions, MakePointer>;
    static constexpr bool IsAligned = MapType::IsAligned;

    // Only partial specification. Dimensions not known at compile time...
    using NDArray =
        ndarray_for_eigen_tensor_t<T, std::conditional_t<std::is_const_v<T>,
                                                         const Scalar,
                                                         Scalar>>;
    using NDArrayCaster = make_caster<NDArray>;
    static constexpr auto Name = NDArrayCaster::Name;
    template<typename T_> using Cast = MapType;
    template<typename T_> static constexpr bool can_cast() { return true; };

    NDArrayCaster caster;

    bool from_python(handle src, uint32_t flags, cleanup_list *cleanup) noexcept {
        // Disable implicit conversions
        flags &= ~cast_flags::convert;
        // Do not accept None
        flags &= ~cast_flags::accepts_none;

        if (!caster.from_python(src, flags, cleanup))
            return false;
        if(IsAligned && !is_tensor_aligned(caster.value.data()))
            return false;

        return true;
    }

    static handle from_cpp(const MapType &v, rv_policy policy, cleanup_list *cleanup) noexcept {
        size_t shape[NumIndices];
        for (size_t i = 0 ; i < NumIndices; i++) {
            shape[i] = (size_t) v.dimension((Eigen::Index) i);
        }

        void* ptr = (void *)v.data();
        if (policy == rv_policy::automatic || policy == rv_policy::automatic_reference)
            policy = rv_policy::reference;
        return NDArrayCaster::from_cpp(
            NDArray {ptr, NumIndices, shape, handle()},
            policy,
            cleanup);
    }

    operator MapType() {
        NDArray &t = caster.value;
        std::array<long, NumIndices> shape;
        for (size_t i = 0 ; i < NumIndices; i++) {
            shape[i] = (long) t.shape(i);
        }
        return MapType(t.data(), shape);
    }
};


/** \brief Type caster for plain ``Eigen::Tensor<T>`` types.
 */
template<typename Scalar, int NumIndices, int Options, typename IndexType>
struct type_caster<
        Eigen::Tensor<Scalar, NumIndices, Options, IndexType>,
        enable_if_t<is_ndarray_scalar_v<Scalar>>> {

    using PlainTensor = Eigen::Tensor<Scalar, NumIndices, Options, IndexType>;
    using Dimensions = typename PlainTensor::Dimensions;
    using Coeffs = typename PlainTensor::CoeffReturnType;
    static constexpr bool IsRowMajor = bool(Options & Eigen::RowMajorBit);
    using NDArray = ndarray_for_eigen_tensor_t<PlainTensor>;
    using NDArrayCaster = make_caster<NDArray>;

    // PlainTensor value;
    NB_TYPE_CASTER(PlainTensor, NDArrayCaster::Name);

    bool from_python(handle src, uint32_t flags, cleanup_list *cleanup) noexcept {
        using NDArrayConst = ndarray_for_eigen_tensor_t<PlainTensor, const Scalar>;
        make_caster<NDArrayConst> caster;
        // Do not accept None
        if (!caster.from_python(src, flags & ~cast_flags::accepts_none, cleanup))
            return false;

        const NDArrayConst &array = caster.value;
        // copy tensor dims
        std::array<long, NumIndices> out_dims;
        for(size_t i = 0; i < NumIndices; i++) {
            out_dims[i] = (long) array.shape(i);
        }
        value.resize(out_dims);

        memcpy(value.data(), array.data(), array.size() * sizeof(Scalar));

        return true;
    }

    template<typename T2>
    static handle from_cpp(T2 &&v, rv_policy policy, cleanup_list *cleanup) noexcept {
        policy = infer_policy<T2>(policy);
        if constexpr (std::is_pointer_v<T2>)
            return from_cpp_internal((const PlainTensor &) *v, policy, cleanup);
        else
            return from_cpp_internal((const PlainTensor &) v, policy, cleanup);
    }

    static handle from_cpp_internal(const PlainTensor &v, rv_policy policy, cleanup_list *cleanup) noexcept {
        size_t shape[NumIndices];

        for (size_t i = 0 ; i < NumIndices; i++) {
            shape[i] = (size_t) v.dimension(i);
        }

        void *ptr = (void *)v.data();

        object owner;
        if (policy == rv_policy::move) {
            PlainTensor *tmp = new PlainTensor((PlainTensor&&)v);
            owner = capsule(tmp, [](void* p) noexcept {
                delete (PlainTensor*) p;
            });
            ptr = tmp->data();
            policy = rv_policy::reference;
        } else if (policy == rv_policy::reference_internal) {
            // reference_internal needs a self pointer; give up if unavailable
            if (!cleanup || !cleanup->self())
                return handle();
            owner = borrow(cleanup->self());
            policy = rv_policy::reference;
        }
        return NDArrayCaster::from_cpp(
            NDArray {ptr, NumIndices, shape, owner},
            policy, cleanup);
    }
};

/** \brief Type caster for Tensor expressions. From-cpp conversion just converts the expression to a plain Tensor object.
 */
template<typename T>
struct type_caster<T, enable_if_t<is_eigen_tensor_xpr_v<T> && is_ndarray_scalar_v<typename T::Scalar>>> {
    static constexpr int NumDimensions = T::NumDimensions;
    static constexpr int Options = T::Options;
    using IndexType = typename T::Index;
    using XprTraits = typename Eigen::internal::traits<T>;
    static constexpr int Layout = XprTraits::Layout;
    using PlainTensor = Eigen::Tensor<typename T::Scalar, NumDimensions, Layout, IndexType>;
    using Caster = make_caster<PlainTensor>;
    static constexpr auto Name = Caster::Name;
    template<typename T_> using Cast = T;
    template<typename T_> static constexpr bool can_cast() { return true; }

    /// Generating an expression template from a Python object is impossible.
    bool from_python(handle src, uint32_t flags, cleanup_list *cleanup) noexcept = delete;

    template <typename T2>
    static handle from_cpp(T2 &&v, rv_policy policy, cleanup_list *cleanup) noexcept {
        return Caster::from_cpp(std::forward<T2>(v), policy, cleanup);
    }
};

/** \brief Type caster for ``Eigen::TensorRef<T>``
 */
template<typename T>
struct type_caster<
    Eigen::TensorRef<T>,
    enable_if_t<is_ndarray_scalar_v<typename T::Scalar>>> {

    using Scalar = typename T::Scalar;
    using IndexType = typename T::Index;
    static constexpr int NumIndices = T::NumIndices;
    static constexpr int Options = T::Options;
    using PlainTensor = Eigen::Tensor<Scalar, NumIndices, Options, IndexType>;
    using Dimensions = typename T::Dimensions;

    // Only partial specification. Dimensions not known at compile time...
    using NDArray =
        ndarray_for_eigen_tensor_t<T, std::conditional_t<std::is_const_v<T>,
                                                         const Scalar,
                                                         Scalar>>;
    using NDArrayCaster = make_caster<NDArray>;

    using MapType = Eigen::TensorMap<T>;
    using MapCaster = make_caster<MapType>;

    using RefType = Eigen::TensorRef<T>;


    static constexpr bool MaybeConvert = std::is_const_v<T>;
    using PlainCaster = make_caster<PlainTensor>;

    static constexpr auto Name = const_name<MaybeConvert>(PlainCaster::Name, MapCaster::Name);

    template<typename T_> using Cast = RefType;
    template<typename T_> static constexpr bool can_cast() { return true; };

    MapCaster caster;
    struct Empty {};
    std::conditional_t<MaybeConvert, PlainCaster, Empty> plain_caster;

    bool from_python(handle src, uint32_t flags, cleanup_list *cleanup) noexcept {
        // no conversion for mutable Ref
        if constexpr (!std::is_const_v<T>)
            flags &= ~cast_flags::convert;

        // Try direct cast
        if (caster.from_python(src, flags, cleanup))
            return true;

        // if const T, attempt leveraging PlainTensor conversion
        if constexpr (MaybeConvert) {
            // we create a new temporary tensor object, and
            // its lifetime is that of plain_caster.
            // for manual conversion, disable conversion.
            if ((flags & cast_flags::manual))
                flags &= ~cast_flags::convert;
            if (plain_caster.from_python(src, flags, cleanup))
                return true;
        }

        return false;
    }

    static handle from_cpp(const RefType &v, rv_policy policy, cleanup_list *cleanup) noexcept {
        size_t shape[NumIndices];

        for (size_t i = 0; i < NumIndices; i++) {
            shape[i] = (size_t) v.dimension((Eigen::Index) i);
        }
    
        return NDArrayCaster::from_cpp(
            NDArray((void *) v.data(), NumIndices, shape, handle()),
            (policy == rv_policy::automatic ||
             policy == rv_policy::automatic_reference)
                ? rv_policy::reference
                : policy,
            cleanup);
    }

    operator RefType() {
        if constexpr (MaybeConvert) {
            // if there's a value, return it
            if (plain_caster.caster.value.is_valid())
                return RefType(plain_caster.operator PlainTensor&());
        }
        return RefType(caster.operator MapType());
    }
};


NAMESPACE_END(detail)

NAMESPACE_END(NB_NAMESPACE)
