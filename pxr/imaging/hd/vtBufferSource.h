//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_VT_BUFFER_SOURCE_H
#define PXR_IMAGING_HD_VT_BUFFER_SOURCE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/vt/value.h"

#include <vector>

#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE


/// \class HdVtBufferSource
///
/// An implementation of HdBufferSource where the source data value is a
/// VtValue.
///
class HdVtBufferSource final : public HdBufferSource
{
public:
    /// Constructs a new buffer from a VtValue.
    ///
    /// \param arraySize indicates how many values are provided per element.
    /// \param allowDoubles indicates if double types can be used, or if they
    ///        must be converted to floats.
    HD_API
    HdVtBufferSource(TfToken const &name, VtValue const& value,
                     int arraySize=1, bool allowDoubles=true);

    /// Constructs a new buffer from a matrix.
    /// The data is convert to the default type (see GetDefaultMatrixType()).
    ///
    /// Note that if we use above VtValue taking constructor, we can use
    /// either float or double matrix regardless the default type.
    ///
    /// \param allowDoubles indicates if double types can be used, or if they
    ///        must be converted to floats regardless of the default type.
    HD_API
    HdVtBufferSource(TfToken const &name, GfMatrix4d const &matrix,
                     bool allowDoubles=true);

    /// Constructs a new buffer from a matrix.
    /// The data is convert to the default type (see GetDefaultMatrixType()).
    ///
    /// Note that if we use above VtValue taking constructor, we can use
    /// either float or double matrix regardless the default type.
    ///
    /// \param arraySize indicates how many values are provided per element.
    /// \param allowDoubles indicates if double types can be used, or if they
    ///        must be converted to floats regardless of the default type.
    HD_API
    HdVtBufferSource(TfToken const &name, VtArray<GfMatrix4d> const &matrices,
                     int arraySize=1, bool allowDoubles=true);

    /// Returns the default matrix type.
    /// The default is HdTypeFloatMat4, but if HD_ENABLE_DOUBLEMATRIX is true,
    /// then HdTypeDoubleMat4 is used instead.
    HD_API
    static HdType GetDefaultMatrixType();

    /// Destructor deletes the internal storage.
    HD_API
    ~HdVtBufferSource() override;

    /// Truncate the buffer to the given number of elements.
    /// If the VtValue contains too much data, this is a way to only forward
    /// part of the data to the hydra buffer system. numElements must be less
    /// than or equal to the current result of GetNumElements().
    HD_API
    void Truncate(size_t numElements);

    /// Return the name of this buffer source.
    TfToken const &GetName() const override {
        return _name;
    }

    /// Returns the raw pointer to the underlying data.
    void const* GetData() const override {
        return HdGetValueData(_value);
    }

    /// Returns the data type and count of this buffer source.
    HdTupleType GetTupleType() const override {
        return _tupleType;
    }

    /// Returns the number of elements (e.g. VtVec3dArray().GetLength()) from
    /// the source array.
    HD_API
    size_t GetNumElements() const override;

    /// Add the buffer spec for this buffer source into given bufferspec vector.
    void GetBufferSpecs(HdBufferSpecVector *specs) const override {
        specs->push_back(HdBufferSpec(_name, _tupleType));
    }

    /// Prepare the access of GetData().
    bool Resolve() override {
        if (!_TryLock()) return false;

        // nothing. just marks as resolved, and returns _data in GetData()
        _SetResolved();
        return true;
    }

protected:
    HD_API
    bool _CheckValid() const override;

private:
    // Constructor helper.
    void _SetValue(const VtValue &v, int arraySize, bool allowDoubles);

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
    size_t _numElements;
};

/// Diagnostic output.
HD_API
std::ostream &operator <<(std::ostream &out, const HdVtBufferSource& self);

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_VT_BUFFER_SOURCE_H
