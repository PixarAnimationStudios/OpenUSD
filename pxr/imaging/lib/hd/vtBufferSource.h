//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef HD_VT_BUFFER_SOURCE_H
#define HD_VT_BUFFER_SOURCE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/vt/value.h"

#include <vector>
#include <boost/shared_ptr.hpp>

#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE


/// \class HdVtBufferSource
///
/// An implementation of HdBufferSource where the source data value is a
/// VtValue.
///
class HdVtBufferSource : public HdBufferSource {
public:
    /// Constructs a new buffer from a VtValue.
    ///
    /// \param arraySize indicates how many values are provided per element.
    HD_API
    HdVtBufferSource(TfToken const &name, VtValue const& value,
                     int arraySize=1);

    /// Constructs a new buffer from a matrix.
    /// The data is convert to the default type (see GetDefaultMatrixType()).
    ///
    /// note that if we use above VtValue taking constructor, we can use
    /// either float or double matrix regardless the default type.
    HD_API
    HdVtBufferSource(TfToken const &name, GfMatrix4d const &matrix);

    /// Constructs a new buffer from a matrix.
    /// The data is convert to the default type (see GetDefaultMatrixType()).
    ///
    /// note that if we use above VtValue taking constructor, we can use
    /// either float or double matrix regardless the default type.
    ///
    /// \param arraySize indicates how many values are provided per element.
    HD_API
    HdVtBufferSource(TfToken const &name, VtArray<GfMatrix4d> const &matrices,
                     int arraySize=1);

    /// Returns the default matrix type.
    /// The default is HdTypeFloatMat4, but if HD_ENABLE_DOUBLEMATRIX is true,
    /// then HdTypeDoubleMat4 is used instead.
    HD_API
    static HdType GetDefaultMatrixType();

    /// Destructor deletes the internal storage.
    HD_API
    ~HdVtBufferSource();

    /// Truncate the buffer to the given number of elements.
    /// If the VtValue contains too much data, this is a way to only forward
    /// part of the data to the hydra buffer system. numElements must be less
    /// than or equal to the current result of GetNumElements().
    HD_API
    void Truncate(int numElements);

    /// Return the name of this buffer source.
    virtual TfToken const &GetName() const override {
        return _name;
    }

    /// Returns the raw pointer to the underlying data.
    virtual void const* GetData() const override {
        return HdGetValueData(_value);
    }

    /// Returns the data type and count of this buffer source.
    virtual HdTupleType GetTupleType() const override {
        return _tupleType;
    }

    /// Returns the number of elements (e.g. VtVec3dArray().GetLength()) from
    /// the source array.
    HD_API
    virtual int GetNumElements() const override;

    /// Add the buffer spec for this buffer source into given bufferspec vector.
    virtual void GetBufferSpecs(HdBufferSpecVector *specs) const override {
        specs->push_back(HdBufferSpec(_name, _tupleType));
    }

    /// Prepare the access of GetData().
    virtual bool Resolve() override {
        if (!_TryLock()) return false;

        // nothing. just marks as resolved, and returns _data in GetData()
        _SetResolved();
        return true;
    }

protected:
    HD_API
    virtual bool _CheckValid() const;

private:
    // Constructor helper.
    void _SetValue(const VtValue &v, int arraySize);

    TfToken _name;

    // We hold the source value to avoid making unnecessary copies of the data: if
    // we immediately copy the source into a temporary buffer, we may need to
    // copy it again into an aggregate buffer later. 
    //
    // We can elide this member easily with only a few internal changes, it
    // should never surface in the public API and for the same reason, this
    // class should remain noncopyable.
    VtValue _value;
    HdTupleType _tupleType;
    int _numElements;
};

/// Diagnostic output.
HD_API
std::ostream &operator <<(std::ostream &out, const HdVtBufferSource& self);

PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_VT_BUFFER_SOURCE_H
