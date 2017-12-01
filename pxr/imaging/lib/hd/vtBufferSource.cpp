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
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/hd/conversions.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/vtExtractor.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/vt/types.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"

#include "pxr/base/tf/getenv.h"
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE


// ------------------------------------------------------------------------- //
// HdVtBufferSource Implementation
// ------------------------------------------------------------------------- //

HdVtBufferSource::HdVtBufferSource(TfToken const& name, VtValue const& value,
                                   bool staticArray)
    : _name(name), _value(value), _staticArray(staticArray)
{
    HD_TRACE_FUNCTION();

    // The GPU has alignment requirements for bools (each one is 32-bits)
    // However, VtBufferSource and the Memory Managers make certain assumptions
    // that the raw buffer in the VtValue matches the requirements of the
    // the render delegate.
    //
    // XXX: For now, scalar bool types are manually converted to
    // ints for processing.  This follows the same pattern as the matrix cases
    // below.
    // Array and componented bools are not currently supported.
    //
    if (_value.IsHolding<bool>()) {
        int intValue = _value.UncheckedGet<bool>() ? 1 : 0;

        _value                = VtValue(intValue);
        _glComponentDataType  = GL_BOOL;
        _glElementDataType    = GL_BOOL;
        _size                 = 4;
        _numComponents        = 1;
        _data                 = &_value.UncheckedGet<int>();
        return;
    }


    // Extract element size from value.
    // This also guarantees the VtValue is an accepted type.
    //
    // note: make sure to extract the pointer from [_value], not [value],
    // since if the VtValue holds a tiny type whose size is less than
    // sizeof(void*), it uses LocalStorage and doesn't allocate a COW storage.
    // In that case the pointer to the held type of [value] is on stack,
    // would be invalid.
    Hd_VtExtractor arrayInfo;

    arrayInfo.Extract(_value);

    // Copy the values out of the extractor.
    _glComponentDataType = arrayInfo.GetGLCompontentType();
    _glElementDataType = arrayInfo.GetGLElementType();
    _size = arrayInfo.GetSize();
    _numComponents = arrayInfo.GetNumComponents();
    _data = arrayInfo.GetData();
}

HdVtBufferSource::HdVtBufferSource(TfToken const &name, GfMatrix4d const &matrix)
    : _name(name), _staticArray(false)
{
    if (GetDefaultMatrixType() == GL_DOUBLE) {
        _value = VtValue(matrix);
        _glComponentDataType = GL_DOUBLE;
        _glElementDataType = GL_DOUBLE_MAT4;
        _size = sizeof(matrix);
        _numComponents = 16;
        _data = _value.UncheckedGet<GfMatrix4d>().GetArray();
    } else {
        GfMatrix4f fmatrix(
            matrix[0][0], matrix[0][1], matrix[0][2], matrix[0][3],
            matrix[1][0], matrix[1][1], matrix[1][2], matrix[1][3],
            matrix[2][0], matrix[2][1], matrix[2][2], matrix[2][3],
            matrix[3][0], matrix[3][1], matrix[3][2], matrix[3][3]);
        _value = VtValue(fmatrix);
        _glComponentDataType = GL_FLOAT;
        _glElementDataType = GL_FLOAT_MAT4;
        _size = sizeof(fmatrix);
        _numComponents = 16;
        _data = _value.UncheckedGet<GfMatrix4f>().GetArray();
    }
}

HdVtBufferSource::HdVtBufferSource(TfToken const &name,
                                   VtArray<GfMatrix4d> const &matrices,
                                   bool staticArray)
    : _name(name), _staticArray(staticArray)
{
    if (GetDefaultMatrixType() == GL_DOUBLE) {
        _value = VtValue(matrices);
        _glComponentDataType = GL_DOUBLE;
        _glElementDataType = GL_DOUBLE_MAT4;
        _size = sizeof(GfMatrix4d) * matrices.size();
        _numComponents = 16;
        // Hold a pointer to the internal storage of the VtArray (_value).
        if (matrices.size() > 0) _data = matrices.cdata();
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
        _value = VtValue(fmatrices);
        _glComponentDataType = GL_FLOAT;
        _glElementDataType = GL_FLOAT_MAT4;
        _size = sizeof(GfMatrix4f) * fmatrices.size();
        _numComponents = 16;
        // Hold a pointer to the internal storage of the VtArray (_value).
        if (fmatrices.size() > 0) _data = fmatrices.cdata();
    }
}

HdVtBufferSource::~HdVtBufferSource()
{
}

/*static*/
GLenum
HdVtBufferSource::GetDefaultMatrixType()
{
    static GLenum matrixType =
        TfGetenvBool("HD_ENABLE_DOUBLE_MATRIX", false) ? GL_DOUBLE : GL_FLOAT;
    return matrixType;
}

/*virtual*/
int
HdVtBufferSource::GetNumElements() const
{
    return _size / (_numComponents *
                    HdConversions::GetComponentSize(_glComponentDataType));
}

bool
HdVtBufferSource::_CheckValid() const
{
    // This is using the same check as _ExtractArrayData()
    // to check for validity.
    //
    // Note: Can't do _size check as an empty buffer is valid
    return ((_numComponents > 0) &&
            (_glComponentDataType > 0) &&
            (_glElementDataType > 0));
}

HD_API
std::ostream &operator <<(std::ostream &out,
                                 const HdVtBufferSource& self) {
    out << "Buffer Source:\n";
    out << "    Size: "                  << self._size << "\n";
    out << "    GL Component DataType: " << self._glComponentDataType << "\n";
    out << "    GL Element DataType: "   << self._glElementDataType << "\n";
    out << "    Num Elements: "          << self.GetNumElements() << "\n";
    out << "    Element Size: "          << self.GetElementSize() << "\n";
    out << "    Num Components: "        << self._numComponents << "\n";
    out << "    Component Size: "        << self.GetComponentSize() << "\n";
    return out;
}

PXR_NAMESPACE_CLOSE_SCOPE

