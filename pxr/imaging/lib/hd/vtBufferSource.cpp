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
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/patchIndex.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/vt/types.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"

#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/stackTrace.h"
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE


// ------------------------------------------------------------------------- //
// HdVtBufferSource Implementation
// ------------------------------------------------------------------------- //

void
HdVtBufferSource::_SetValue(const VtValue &v, size_t arraySize)
{
    _value = v;
    _tupleType = HdGetValueTupleType(_value);

    // XXX: The following is a bit weird and could use reconsideration.
    // The GL backend has specific alignment requirement for bools.
    // Currently that is implemented by having HdVtBufferSource promote
    // bool into int32 values while still reporting the value type as
    // HdTypeBool (so that shader codegen can emit the right typenames).
    // It would be better for this kind of concern to be handled closer
    // to the specific backend.
    // Array and componented bools are not currently supported.
    if (_value.IsHolding<bool>()) {
        int intValue = _value.UncheckedGet<bool>() ? 1 : 0;
        _value = VtValue(intValue);
        // Intentionally leave _tupleType as HdTypeBool; see comment above.
    }

    // For the common case of a default value that is an empty
    // VtArray<T>, interpret it as one T per element rather than
    // a zero-sized tuple.
    if (_value.IsArrayValued() && _value.GetArraySize() == 0) {
        _tupleType.count = 1;
        _numElements = 0;
        return;
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
                                   size_t arraySize)
    : _name(name)
{
    _SetValue(value, arraySize);
}

HdVtBufferSource::HdVtBufferSource(TfToken const &name,
                                   GfMatrix4d const &matrix)
    : _name(name)
{
    if (GetDefaultMatrixType() == HdTypeDoubleMat4) {
        _SetValue( VtValue(matrix), 1 );
    } else {
        GfMatrix4f fmatrix(
            matrix[0][0], matrix[0][1], matrix[0][2], matrix[0][3],
            matrix[1][0], matrix[1][1], matrix[1][2], matrix[1][3],
            matrix[2][0], matrix[2][1], matrix[2][2], matrix[2][3],
            matrix[3][0], matrix[3][1], matrix[3][2], matrix[3][3]);
        _SetValue( VtValue(fmatrix), 1 );
    }
}

HdVtBufferSource::HdVtBufferSource(TfToken const &name,
                                   VtArray<GfMatrix4d> const &matrices,
                                   size_t arraySize)
    : _name(name)
{
    if (GetDefaultMatrixType() == HdTypeDoubleMat4) {
        _SetValue( VtValue(matrices), arraySize );
    } else {
        VtArray<GfMatrix4f> fmatrices(matrices.size());
        for (size_t i = 0; i < matrices.size(); ++i) {
            GfMatrix4d const &matrix = matrices[i];
            GfMatrix4f fmatrix(
                matrix[0][0], matrix[0][1], matrix[0][2], matrix[0][3],
                matrix[1][0], matrix[1][1], matrix[1][2], matrix[1][3],
                matrix[2][0], matrix[2][1], matrix[2][2], matrix[2][3],
                matrix[3][0], matrix[3][1], matrix[3][2], matrix[3][3]);
            fmatrices[i] = fmatrix;
        }
        _SetValue( VtValue(fmatrices), arraySize );
    }
}

HdVtBufferSource::~HdVtBufferSource()
{
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
int
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

