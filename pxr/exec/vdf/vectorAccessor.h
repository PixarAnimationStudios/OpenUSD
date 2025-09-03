//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_VECTOR_ACCESSOR_H
#define PXR_EXEC_VDF_VECTOR_ACCESSOR_H

#include "pxr/pxr.h"

#include "pxr/exec/vdf/boxedContainer.h"
#include "pxr/exec/vdf/compressedIndexMapping.h"
#include "pxr/exec/vdf/vectorData.h"

#include "pxr/base/arch/demangle.h"

#include "pxr/base/tf/safeTypeCompare.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Accessor class. This is used to provide fast access with making sure
/// that the type checks are done no matter what.
///
template<typename T>
class Vdf_VectorAccessor
{
public:
    static_assert(
        !Vdf_IsBoxedContainer<T>,
        "Vdf_VectorAccessor does not provide access to boxed containers");

    /// Default constructor.
    ///
    Vdf_VectorAccessor() :
        _numValues(0),
        _boxed(false)
    {}

    /// Constructor.
    ///
    Vdf_VectorAccessor(
        Vdf_VectorData *data,
        const Vdf_VectorData::Info &info)
        : _boxed(info.layout == Vdf_VectorData::Info::Layout::Boxed) {

        const std::type_info &haveType = data->GetTypeInfo();

        if (ARCH_UNLIKELY(!TfSafeTypeCompare(haveType, typeid(T)))) {
            TF_FATAL_ERROR(
                "Invalid type.  Vector is holding %s, tried to use as %s",
                ArchGetDemangled(haveType).c_str(),
                ArchGetDemangled(typeid(T)).c_str());
        }

        // Setup the compressed index mapping, if any.
        _indexMapping = nullptr;
        if (ARCH_UNLIKELY(info.compressedIndexMapping)) {
            _numValues = info.size;
            _data = (T *)info.data;
            _indexMapping = info.compressedIndexMapping;
            _indexMappingHint = 0;
        }

        // Access for vector data that is not boxed.
        else if (!_boxed)  {
            _numValues = info.size;
            _data = (T *)info.data - info.first;
        }

        // Access for boxed vector data.
        else {
            // We expect exactly a single data element in this case.
            TF_DEV_AXIOM(
                !info.compressedIndexMapping &&
                info.size == 1 && info.first == 0 && info.last == 0);

            using BoxedVectorType = Vdf_BoxedContainer<T>;
            BoxedVectorType *boxedVector = (BoxedVectorType *)info.data;
            _numValues = boxedVector->size();
            _data = boxedVector->data();
        }
    }

    /// Returns true if vector is empty.
    ///
    bool IsEmpty() const { return _numValues == 0; }

    /// Returns size of the vector, ie. the number of values it holds.
    ///
    size_t GetNumValues() const { return _numValues; }

    /// Returns \c true if this accessor is providing element-wise access into
    /// a boxed container.
    ///
    bool IsBoxed() const { return _boxed; }

    /// Returns a reference to an element.
    ///
    T &operator[](size_t idx) const {
        if (ARCH_UNLIKELY(_indexMapping)) {
            idx = _indexMapping->FindDataIndex(idx, &_indexMappingHint);
        }
        return _data[idx];
    }

private:
    size_t                      _numValues;
    T                          *_data;
    Vdf_CompressedIndexMapping *_indexMapping;
    mutable size_t              _indexMappingHint;
    bool                        _boxed;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
