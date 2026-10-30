/*
    nanobind/eigen/sparse.h: type casters for sparse Eigen matrices

    Copyright (c) 2023 Henri Menke and Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include <nanobind/ndarray.h>
#include <Eigen/SparseCore>

#include <memory>
#include <type_traits>
#include <utility>

NAMESPACE_BEGIN(NB_NAMESPACE)

NAMESPACE_BEGIN(detail)

inline import_cache scipy_csr { "scipy.sparse", "csr_matrix" },
                    scipy_csc { "scipy.sparse", "csc_matrix" };

/// Detect Eigen::SparseMatrix
template <typename T> constexpr bool is_eigen_sparse_matrix_v =
    is_base_of_template_v<T, Eigen::SparseMatrixBase> &&
    !std::is_base_of_v<Eigen::SparseMapBase<T, Eigen::ReadOnlyAccessors>, T>;


/// Caster for Eigen::SparseMatrix
template <typename T> struct type_caster<T, enable_if_t<is_eigen_sparse_matrix_v<T>>> {
    using Scalar = typename T::Scalar;
    using StorageIndex = typename T::StorageIndex;
    using Index = typename T::Index;
    using SparseMap = Eigen::Map<T>;

    static_assert(std::is_same_v<T, Eigen::SparseMatrix<Scalar, T::Options, StorageIndex>>,
                  "nanobind: Eigen sparse caster only implemented for matrices");

    static constexpr bool RowMajor = T::IsRowMajor;

    using ScalarNDArray = ndarray<numpy, Scalar, shape<-1>>;
    using StorageIndexNDArray = ndarray<numpy, StorageIndex, shape<-1>>;

    using ScalarCaster = make_caster<ScalarNDArray>;
    using StorageIndexCaster = make_caster<StorageIndexNDArray>;

    NB_TYPE_CASTER(T, const_name<RowMajor>("scipy.sparse.csr_matrix[",
                                           "scipy.sparse.csc_matrix[")
                   + make_caster<Scalar>::Name + const_name("]"))

    ScalarCaster data_caster;
    StorageIndexCaster indices_caster, indptr_caster;

    bool from_python(handle src, uint32_t flags, cleanup_list *cleanup) noexcept {
        object obj = borrow(src);

        try {
            handle matrix_type = (RowMajor ? scipy_csr : scipy_csc).get();
            if (!obj.type().is(matrix_type)) {
                if (!(flags & cast_flags::convert))
                    return false;
                obj = matrix_type(obj);
            }

            if (!cast<bool>(obj.attr("has_sorted_indices")))
                obj.attr("sort_indices")();

            if (object data_o = obj.attr("data");
                !data_caster.from_python(data_o, flags, cleanup))
                return false;

            if (object indices_o = obj.attr("indices");
                !indices_caster.from_python(indices_o, flags, cleanup))
                return false;

            if (object indptr_o = obj.attr("indptr");
                !indptr_caster.from_python(indptr_o, flags, cleanup))
                return false;

            object shape_o = obj.attr("shape");
            if (len(shape_o) != 2)
                return false;

            Index rows = cast<Index>(shape_o[0]),
                  cols = cast<Index>(shape_o[1]),
                  nnz = cast<Index>(obj.attr("nnz"));

            value = SparseMap(rows, cols, nnz,
                              indptr_caster.value.data(),
                              indices_caster.value.data(),
                              data_caster.value.data());
            return true;
        } catch (const std::exception &) {
            return false;
        }
    }

    static handle from_cpp(T &&v, rv_policy policy, cleanup_list *cleanup) noexcept {
        if (policy == rv_policy::automatic ||
            policy == rv_policy::automatic_reference)
            policy = rv_policy::move;

        return from_cpp((const T &) v, policy, cleanup);
    }

    template <typename T2>
    static handle from_cpp(T2 &&v, rv_policy policy, cleanup_list *cleanup) noexcept {
        policy = infer_policy<T2>(policy);
        if constexpr (std::is_pointer_v<T2>)
            return from_cpp_internal((const T &) *v, policy, cleanup);
        else
            return from_cpp_internal((const T &) v, policy, cleanup);
    }

    static handle from_cpp_internal(const T &v, rv_policy policy, cleanup_list *) noexcept {
        if (!v.isCompressed()) {
            PyErr_SetString(PyExc_ValueError,
                            "nanobind: unable to return an Eigen sparse matrix that is not in a compressed format. "
                            "Please call `.makeCompressed()` before returning the value on the C++ end.");
            return handle();
        }

        handle matrix_type;
        try {
            matrix_type = (RowMajor ? scipy_csr : scipy_csc).get();
        } catch (python_error &e) {
            e.restore();
            return handle();
        }

        const Index rows = v.rows(), cols = v.cols();
        const size_t data_shape[] = { (size_t) v.nonZeros() };
        const size_t outer_indices_shape[] = { (size_t) ((RowMajor ? rows : cols) + 1) };

        T *src = std::addressof(const_cast<T &>(v));
        object owner;
        if (policy == rv_policy::move) {
            // Eigen sparse matrices lack a move constructor; copy-constructing
            // from a matrix flagged as an rvalue performs a cheap pointer swap.
            const_cast<T &>(v).markAsRValue();
            src = new T(v);
            owner = capsule(src, [](void *p) noexcept { delete (T *) p; });
        }

        ScalarNDArray data(src->valuePtr(), 1, data_shape, owner);
        StorageIndexNDArray outer_indices(src->outerIndexPtr(), 1, outer_indices_shape, owner);
        StorageIndexNDArray inner_indices(src->innerIndexPtr(), 1, data_shape, owner);

        try {
            return matrix_type(nanobind::make_tuple(
                                   std::move(data), std::move(inner_indices), std::move(outer_indices)),
                               nanobind::make_tuple(rows, cols))
                .release();
        } catch (python_error &e) {
            e.restore();
            return handle();
        }
    }
};


/// Caster for Eigen::Map<Eigen::SparseMatrix>, still needs to be implemented.
template <typename T>
struct type_caster<Eigen::Map<T>, enable_if_t<is_eigen_sparse_matrix_v<T>>> {
    using Scalar = typename T::Scalar;
    using StorageIndex = typename T::StorageIndex;
    using Index = typename T::Index;
    using SparseMap = Eigen::Map<T>;
    using Map = Eigen::Map<T>;
    using SparseMatrixCaster = type_caster<T>;
    static constexpr bool RowMajor = T::IsRowMajor;

    using ScalarNDArray = ndarray<numpy, Scalar, shape<-1>>;
    using StorageIndexNDArray = ndarray<numpy, StorageIndex, shape<-1>>;

    using ScalarCaster = make_caster<ScalarNDArray>;
    using StorageIndexCaster = make_caster<StorageIndexNDArray>;

    static constexpr auto Name = const_name<RowMajor>("scipy.sparse.csr_matrix[",
                                           "scipy.sparse.csc_matrix[")
                   + make_caster<Scalar>::Name + const_name("]");

    template <typename T_> using Cast = Map;
    template <typename T_> static constexpr bool can_cast() { return true; }

    ScalarCaster data_caster;
    StorageIndexCaster indices_caster, indptr_caster;
    Index rows, cols, nnz;

    bool from_python(handle src, uint32_t flags, cleanup_list *cleanup) noexcept {
        flags &= ~cast_flags::convert;

        try {
            handle matrix_type = (RowMajor ? scipy_csr : scipy_csc).get();
            if (!src.type().is(matrix_type))
                return false;

            if (!cast<bool>(src.attr("has_sorted_indices")))
                src.attr("sort_indices")();

            if (object data_o = src.attr("data");
                !data_caster.from_python(data_o, flags, cleanup))
                return false;

            if (object indices_o = src.attr("indices");
                !indices_caster.from_python(indices_o, flags, cleanup))
                return false;

            if (object indptr_o = src.attr("indptr");
                !indptr_caster.from_python(indptr_o, flags, cleanup))
                return false;

            object shape_o = src.attr("shape");
            if (len(shape_o) != 2)
                return false;

            rows = cast<Index>(shape_o[0]);
            cols = cast<Index>(shape_o[1]);
            nnz = cast<Index>(src.attr("nnz"));
        } catch (const std::exception &) {
            return false;
        }

        return true;
    }

    static handle from_cpp(const Map &v, rv_policy, cleanup_list *) noexcept {
        if (!v.isCompressed()) {
            PyErr_SetString(
                PyExc_ValueError,
                "nanobind: unable to return an Eigen sparse matrix that is not "
                "in a compressed format. Please call `.makeCompressed()` "
                "before returning the value on the C++ end.");
            return handle();
        }

        handle matrix_type;
        try {
            matrix_type = (RowMajor ? scipy_csr : scipy_csc).get();

            const Index rows = v.rows(), cols = v.cols();
            const size_t data_shape[] = { (size_t) v.nonZeros() };
            const size_t outer_indices_shape[] = {
                (size_t) ((RowMajor ? rows : cols) + 1)
            };

            ScalarNDArray data((void *) v.valuePtr(), 1, data_shape);
            StorageIndexNDArray
                outer_indices((void *) v.outerIndexPtr(), 1, outer_indices_shape),
                inner_indices((void *) v.innerIndexPtr(), 1, data_shape);

            return matrix_type(nanobind::make_tuple(
                                   cast(data, rv_policy::reference),
                                   cast(inner_indices, rv_policy::reference),
                                   cast(outer_indices, rv_policy::reference)),
                               nanobind::make_tuple(rows, cols))
                .release();
        } catch (python_error &e) {
            e.restore();
            return handle();
        }
    };

    operator Map() {
        return SparseMap(rows, cols, nnz,
                         indptr_caster.value.data(),
                         indices_caster.value.data(),
                         data_caster.value.data());
    }
};


/// Caster for Eigen::Ref<Eigen::SparseMatrix>, still needs to be implemented
template <typename T, int Options>
struct type_caster<Eigen::Ref<T, Options>, enable_if_t<is_eigen_sparse_matrix_v<T>>> {
    using Ref = Eigen::Ref<T, Options>;
    using Map = Eigen::Map<T, Options>;
    using MapCaster = make_caster<Map>;
    static constexpr auto Name = MapCaster::Name;
    template <typename T_> using Cast = Ref;
    template <typename T_> static constexpr bool can_cast() { return true; }

    bool from_python(handle src, uint32_t flags, cleanup_list *cleanup) noexcept = delete;

    static handle from_cpp(const Ref &v, rv_policy policy, cleanup_list *cleanup) noexcept = delete;
};

NAMESPACE_END(detail)

NAMESPACE_END(NB_NAMESPACE)
