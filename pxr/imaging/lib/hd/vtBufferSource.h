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

#include "pxr/base/tf/token.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/glUtils.h"
#include "pxr/imaging/hd/patchIndex.h"
#include "pxr/base/vt/value.h"

#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/mpl/vector/vector40.hpp>

#include <iosfwd>

/// \class HdVtBufferSource
///
/// A transient buffer of data that has not yet been committed to the GPU.
///
/// This class is primarily used in the interaction between HdRprim and the
/// HdSceneDelegate. The buffer source holds raw data that is either 
/// topological or a shader input (PrimVar data), so it gets attached to either
/// an HdTopologySubset or an HdPrimVarLayout. The buffer source will be 
/// inserted into these objects at the offset specified or appended to the end.
/// 
/// The public interface provided is intended to be convenient for OpenGL API
/// calls.
///
class HdVtBufferSource : public HdBufferSource {
public:

    // TODO: Add support for scalar types (float, int, bool)

    // TODO: Make this internal or perhaps use a TfHashMap/dispatch table.

    // This struct is required to avoid instantiation of the actual types during
    // type dispatch.
    template <typename T> struct THolder {
    typedef T Type;
    };

    // The valid types a HdBufferSource can be constructed from.
    typedef boost::mpl::vector31<
            THolder<int>,
            THolder<float>,
            THolder<double>,
            THolder<size_t>,
            THolder<VtIntArray>,
            THolder<VtFloatArray>,
            THolder<VtDoubleArray>,
            THolder<VtVec2fArray>,
            THolder<VtVec3fArray>,
            THolder<VtVec4fArray>,
            THolder<VtVec2dArray>,
            THolder<VtVec3dArray>,
            THolder<VtVec4dArray>,
            THolder<VtVec2iArray>,
            THolder<VtVec3iArray>,
            THolder<VtVec4iArray>,
            THolder<GfMatrix4d>,
            THolder<GfMatrix4f>,
            THolder<GfVec2f>,
            THolder<GfVec3f>,
            THolder<GfVec4f>,
            THolder<GfVec2d>,
            THolder<GfVec3d>,
            THolder<GfVec4d>,
            THolder<GfVec2i>,
            THolder<GfVec3i>,
            THolder<GfVec4i>,
            THolder<VtArray<HdVec4f_2_10_10_10_REV> >,
            THolder<VtArray<GfMatrix4f> >,
            THolder<VtArray<GfMatrix4d> >,
            THolder<VtArray<Hd_BSplinePatchIndex> >
            > AcceptedTypes;

    /// Constructs a new buffer from an existing VtValue, the data is fully
    /// copied into a new internal buffer.
    ///
    /// We may be able to map this to GPU memory in the glorious future.
    HdVtBufferSource(TfToken const &name, VtValue const& value,
                     bool staticArray=false);

    /// Constructs a new buffer from a matrix, the data is copied using default
    /// matrix type.
    /// (GL_FLOAT by default, GL_DOUBLE when HD_ENABLE_DOUBLE_MATRIX=1)
    /// note that if we use above VtValue taking constructor, we can use
    /// either float or double matrix regardless the default type.
    HdVtBufferSource(TfToken const &name, GfMatrix4d const &matrix);

    /// Constructs a new buffer from matrix array. The data is copied
    /// using default matrix type.
    /// (GL_FLOAT by default, GL_DOUBLE when HD_ENABLE_DOUBLE_MATRIX=1)
    /// note that if we use above VtValue taking constructor, we can use
    /// either float or double matrix regardless the default type.
    HdVtBufferSource(TfToken const &name, VtArray<GfMatrix4d> const &matrices,
                     bool staticArray=false);

    /// Returns the default matrix type (GL_FLOAT or GL_DOUBLE)
    static GLenum GetDefaultMatrixType();

    /// Destructor deletes the internal storage.
    ~HdVtBufferSource();

    /// Return the name of this buffer source.
    virtual TfToken const &GetName() const {return _name;}

    /// Returns the raw pointer to the underlying data.
    virtual void const* GetData() const {return _data;}

    /// OpenGL data type; GL_UNSIGNED_INT, etc
    virtual int GetGLComponentDataType() const {return _glComponentDataType;}

    /// OpenGL data type; GL_FLOAT_VEC3, etc
    virtual int GetGLElementDataType() const {return _glElementDataType;}

    /// Returns the flat array size in bytes.
    virtual size_t GetSize() const {return _size;}

    /// Returns the number of elements (e.g. VtVec3dArray().GetLength()) from
    /// the source array.
    virtual int GetNumElements() const;

    /// Returns the number of components in a single element.
    ///
    /// For example, for a BufferSource created from a VtIntArray, this method
    /// would return 1, but for a VtVec3dArray this method would return 3.
    ///
    /// This value is always in the range [1,4] or 16 (GfMatrix4d).
    virtual short GetNumComponents() const {return _numComponents;}

    /// Add the buffer spec for this buffer source into given bufferspec vector.
    virtual void AddBufferSpecs(HdBufferSpecVector *specs) const {
        specs->push_back(
            HdBufferSpec(_name, _glComponentDataType, _numComponents,
                         _staticArray ? GetNumElements() : 1));
    }

    /// Prepare the access of GetData().
    virtual bool Resolve() {
        if (not _TryLock()) return false;

        // nothing. just marks as resolved, and returns _data in GetData()
        _SetResolved();
        return true;
    }

    friend std::ostream &operator <<(std::ostream &out,
                                     const HdVtBufferSource& self);

protected:
    virtual bool _CheckValid() const;

private:
    TfToken _name;

    // We hold the source value to avoid making unnecessary copies of the data: if
    // we immediately copy the source into a temporary buffer, we may need to
    // copy it again into an aggregate buffer later. 
    //
    // We can elide this member easily with only a few internal changes, it
    // should never surface in the public API and for the same reason, this
    // class should remain noncopyable.
    VtValue _value;

    void const* _data;
    int _glComponentDataType;
    int _glElementDataType;
    size_t _size;
    short _numComponents;
    bool _staticArray;
};

#endif //HD_VT_BUFFER_SOURCE_H
