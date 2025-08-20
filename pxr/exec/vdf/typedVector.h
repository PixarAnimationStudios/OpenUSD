//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_TYPED_VECTOR_H
#define PXR_EXEC_VDF_TYPED_VECTOR_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/vector.h"

PXR_NAMESPACE_OPEN_SCOPE

/// A VdfTypedVector implements a VdfVector with a specific type. 
///
template<typename TYPE>
class VdfTypedVector : public VdfVector
{
public:
    // Note the sole purpose of this type is to allow construction of
    // VdfVectors holding TYPE.  VdfVector is not polymorphic, this type
    // should not hold any state at all.  We rely that objects of this type
    // can be sliced into VdfVectors with no adverse effect.

    /// Constructs an empty vector.
    VdfTypedVector() {
        static_assert(sizeof(VdfTypedVector) == sizeof(VdfVector),
                      "VdfTypedVector must have same size as VdfVector");
        _data.New<Vdf_VectorImplEmpty<TYPE>>(0);
    }

    /// Constructs a new vector and initializes it with a specific value.
    VdfTypedVector(const TYPE &value) {
        static_assert(sizeof(VdfTypedVector) == sizeof(VdfVector),
                      "VdfTypedVector must have same size as VdfVector");
        _data.New<Vdf_VectorImplSingle<TYPE>>(value);
    }

    /// Constructs a new vector with the specified size.
    static VdfTypedVector CreateWithSize(size_t size) {
        return VdfTypedVector(size, _WithSize);
    }

private:
    enum _WithSizeTag { _WithSize };
    VdfTypedVector(size_t size, _WithSizeTag) {
        static_assert(sizeof(VdfTypedVector) == sizeof(VdfVector),
                      "VdfTypedVector must have same size as VdfVector");
        switch (size) {
        case 0:
            _data.New<Vdf_VectorImplEmpty<TYPE>>(0);
            break;
        case 1:
            _data.New<Vdf_VectorImplSingle<TYPE>>();
            break;
        default:
            _data.New<Vdf_VectorImplContiguous<TYPE>>(size);
            break;
        }
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif

