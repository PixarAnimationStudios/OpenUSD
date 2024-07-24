//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/vt/types.h"

#include "pxr/base/gf/matrix2d.h"
#include "pxr/base/gf/matrix2f.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"

#include "pxr/base/tf/getenv.h"
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE


// ------------------------------------------------------------------------- //
// HdVtBufferSource Implementation
// ------------------------------------------------------------------------- //

namespace {

template <typename DBL, typename FLT>
void
_ConvertDoubleToFloat(VtValue * value, HdTupleType * tupleType)
{
    *value = VtValue(FLT(value->UncheckedGet<DBL>()));
    *tupleType = HdGetValueTupleType(*value);
}

template <typename DBL, typename FLT>
void
_ConvertDoubleToFloatArray(VtValue * value, HdTupleType * tupleType)
{
    VtArray<DBL> const & dblArray = value->UncheckedGet<VtArray<DBL>>();
    VtArray<FLT> fltArray(dblArray.size());
    for (size_t i = 0; i < dblArray.size(); ++i) {
        fltArray[i] = FLT(dblArray[i]);
    }
    *value = VtValue(fltArray);
    *tupleType = HdGetValueTupleType(*value);
}

}

void
HdVtBufferSource::_SetValue(const VtValue &v, int arraySize, bool allowDoubles)
{
    _value = v;
    _tupleType = HdGetValueTupleType(_value);

    // For the common case of a default value that is an empty
    // VtArray<T>, interpret it as one T per element rather than
    // a zero-sized tuple.
    if (_value.IsArrayValued() && _value.GetArraySize() == 0) {
        _tupleType.count = 1;
        _numElements = 0;
        return;
    }

    // XXX: The following is a bit weird and could use reconsideration.
    // The GL backend has specific alignment requirement for bools.
    // Currently that is implemented by having HdVtBufferSource promote
    // bool into int32 values while still reporting the value type as
    // HdTypeBool (so that shader codegen can emit the right typenames).
    // It would be better for this kind of concern to be handled closer
    // to the specific backend.
    // Componented bools are not currently supported.
    if (_value.IsHolding<bool>()) {
        int intValue = _value.UncheckedGet<bool>() ? 1 : 0;
        _value = VtValue(intValue);
        // Intentionally leave _tupleType as HdTypeBool; see comment above.
    } else if (_value.IsHolding<VtBoolArray>()) {
        VtBoolArray boolValues = _value.UncheckedGet<VtBoolArray>();
        VtIntArray intValues(_value.GetArraySize());
        for (size_t i = 0; i < _value.GetArraySize(); i++) {
            intValues[i] = boolValues[i] ? 1 : 0;
        }
        _value = VtValue(intValues);
    } else if (!allowDoubles) {
        // Any doubles must be converted to floats.
        if (_value.IsArrayValued()) {
            if (_value.IsHolding<VtDoubleArray>()) {
                _ConvertDoubleToFloatArray<double, float>(&_value, &_tupleType);
            } else if (_value.IsHolding<VtVec2dArray>()) {
                _ConvertDoubleToFloatArray<GfVec2d, GfVec2f>(&_value,
                    &_tupleType);
            } else if (_value.IsHolding<VtVec3dArray>()) {
                _ConvertDoubleToFloatArray<GfVec3d, GfVec3f>(&_value,
                    &_tupleType);
            } else if (_value.IsHolding<VtVec4dArray>()) {
                _ConvertDoubleToFloatArray<GfVec4d, GfVec4f>(&_value,
                    &_tupleType);
            } else if (_value.IsHolding<VtMatrix2dArray>()) {
                _ConvertDoubleToFloatArray<GfMatrix2d, GfMatrix2f>(&_value,
                    &_tupleType);
            } else if (_value.IsHolding<VtMatrix3dArray>()) {
                _ConvertDoubleToFloatArray<GfMatrix3d, GfMatrix3f>(&_value,
                    &_tupleType);
            } else if (_value.IsHolding<VtMatrix4dArray>()) {
                _ConvertDoubleToFloatArray<GfMatrix4d, GfMatrix4f>(&_value,
                    &_tupleType);
            }
        }
        else {
            if (_value.IsHolding<double>()) {
                _ConvertDoubleToFloat<double, float>(&_value, &_tupleType);
            } else if (_value.IsHolding<GfVec2d>()) {
                _ConvertDoubleToFloat<GfVec2d, GfVec2f>(&_value, &_tupleType);
            } else if (_value.IsHolding<GfVec3d>()) {
                _ConvertDoubleToFloat<GfVec3d, GfVec3f>(&_value, &_tupleType);
            } else if (_value.IsHolding<GfVec4d>()) {
                _ConvertDoubleToFloat<GfVec4d, GfVec4f>(&_value, &_tupleType);
            } else if (_value.IsHolding<GfMatrix2d>()) {
                _ConvertDoubleToFloat<GfMatrix2d, GfMatrix2f>(&_value,
                    &_tupleType);
            } else if (_value.IsHolding<GfMatrix3d>()) {
                _ConvertDoubleToFloat<GfMatrix3d, GfMatrix3f>(&_value,
                    &_tupleType);
            } else if (_value.IsHolding<GfMatrix4d>()) {
                _ConvertDoubleToFloat<GfMatrix4d, GfMatrix4f>(&_value,
                    &_tupleType);
            }
        }
    }

    // Factor the VtArray length into numElements and tuple count.
    // VtArray is a 1D array and does not have multidimensional shape,
    // therefore it cannot distinguish the case of N values for M elements
    // from the case of 1 value for NM elements.  This is why
    // HdVtBufferSource requires the caller to provide this context
    // via the arraySize argument, so it can apply that shape here.
    _numElements = _tupleType.count / arraySize;
    _tupleType.count = arraySize;
}

HdVtBufferSource::HdVtBufferSource(TfToken const& name, VtValue const& value,
                                   int arraySize, bool allowDoubles)
    : _name(name)
{
    _SetValue(value, arraySize, allowDoubles);
}

HdVtBufferSource::HdVtBufferSource(TfToken const &name,
                                   GfMatrix4d const &matrix,
                                   bool allowDoubles)
    : _name(name)
{
    bool const resolvedAllowDoubles = allowDoubles &&
        GetDefaultMatrixType() == HdTypeDoubleMat4;
    _SetValue(VtValue(matrix), 1, resolvedAllowDoubles);
}

HdVtBufferSource::HdVtBufferSource(TfToken const &name,
                                   VtArray<GfMatrix4d> const &matrices,
                                   int arraySize,
                                   bool allowDoubles)
    : _name(name)
{
    bool const resolvedAllowDoubles = allowDoubles &&
        GetDefaultMatrixType() == HdTypeDoubleMat4;
    _SetValue(VtValue(matrices), arraySize, resolvedAllowDoubles);
}

HdVtBufferSource::~HdVtBufferSource()
{
}

void
HdVtBufferSource::Truncate(size_t numElements)
{
    if (numElements > _numElements) {
        TF_CODING_ERROR(
            "Buffer '%s', cannot truncate from length %zu to length %zu",
            _name.GetText(), _numElements, numElements);
        return;
    }

    _numElements = numElements;
}

/*static*/
HdType
HdVtBufferSource::GetDefaultMatrixType()
{
    static HdType matrixType =
        TfGetenvBool("HD_ENABLE_DOUBLE_MATRIX", false)
        ? HdTypeDoubleMat4 : HdTypeFloatMat4;
    return matrixType;
}

/*virtual*/
size_t
HdVtBufferSource::GetNumElements() const
{
    return _numElements;
}

bool
HdVtBufferSource::_CheckValid() const
{
    return _tupleType.type != HdTypeInvalid;
}

HD_API
std::ostream &operator <<(std::ostream &out, const HdVtBufferSource& self) {
    const HdTupleType t = self.GetTupleType();

    out << "Buffer Source:\n";
    out << "    Name:      " << self.GetName() << "\n";
    out << "    Size:      " << HdDataSizeOfTupleType(t) << "\n";
    out << "    Type:      " << TfEnum::GetName(t.type) << "\n";
    out << "    Count:     " << t.count << "\n";
    out << "    Num elems: " << self.GetNumElements() << "\n";
    return out;
}

PXR_NAMESPACE_CLOSE_SCOPE

