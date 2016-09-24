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

#include "pxr/base/vt/array.h"
#include "pxr/base/vt/types.h"

#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2i.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3i.h"

#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4i.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"

#include "pxr/base/tf/getenv.h"

#include <boost/type_traits.hpp>

#include <boost/mpl/assert.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/has_xxx.hpp>

#include <boost/utility/enable_if.hpp>

#include <iostream>

// ------------------------------------------------------------------------- //
// Generic helpers for extracting data from VtValue into char[]
// ------------------------------------------------------------------------- //
// What's happening here:
//
//   * HdVtBufferSource gets initialized with a VtValue and calls
//     _ExtractArrayData to convert it to a raw byte array.
//
//   * _ExtractArrayData does a type dispatch and checks if the VtValue
//     provided is holding one of the HdVtBufferSource::AcceptedTypes
//
//   * When it determines the actual held type, the _Extractor is initialized
//     with the held T or VtArray<T> value
//
//   * For VtArray<T>, the appropriate _GetNumComponents method is then
//     selected based on the element type of the VtArray
//
//   * The OpenGL data type enumeration is selected using _GetGLType<T>, where
//     T is VtArray<T>::ElementType
//
// This code is self contained and could be split off into a Vt conversion
// helper module.
// ------------------------------------------------------------------------- //

// Convert runtime element type into GL component and element type enums.
struct _GLDataType {
    _GLDataType(int componentType, int elementType)
        : componentType(componentType)
        , elementType(elementType) { }
    int componentType;
    int elementType;
};
template<typename T> _GLDataType _GetGLType();
template<> _GLDataType _GetGLType<char>()
        { return _GLDataType(GL_BYTE, GL_BYTE); }
template<> _GLDataType _GetGLType<short>()
        { return _GLDataType(GL_SHORT, GL_SHORT); }
template<> _GLDataType _GetGLType<unsigned short>()
        { return _GLDataType(GL_UNSIGNED_SHORT, GL_UNSIGNED_SHORT); }
template<> _GLDataType _GetGLType<int>()
        { return _GLDataType(GL_INT, GL_INT); }
template<> _GLDataType _GetGLType<size_t>()
        { return _GLDataType(GL_UNSIGNED_INT64_NV, GL_UNSIGNED_INT64_NV); }
template<> _GLDataType _GetGLType<GfVec2i>()
        { return _GLDataType(GL_INT, GL_INT_VEC2); }
template<> _GLDataType _GetGLType<GfVec3i>()
        { return _GLDataType(GL_INT, GL_INT_VEC3); }
template<> _GLDataType _GetGLType<GfVec4i>()
        { return _GLDataType(GL_INT, GL_INT_VEC4); }
template<> _GLDataType _GetGLType<unsigned int>()
        { return _GLDataType(GL_UNSIGNED_INT, GL_UNSIGNED_INT); }
template<> _GLDataType _GetGLType<float>()
        { return _GLDataType(GL_FLOAT, GL_FLOAT); }
template<> _GLDataType _GetGLType<GfVec2f>()
        { return _GLDataType(GL_FLOAT, GL_FLOAT_VEC2); }
template<> _GLDataType _GetGLType<GfVec3f>()
        { return _GLDataType(GL_FLOAT, GL_FLOAT_VEC3); }
template<> _GLDataType _GetGLType<GfVec4f>()
        { return _GLDataType(GL_FLOAT, GL_FLOAT_VEC4); }
template<> _GLDataType _GetGLType<double>()
        { return _GLDataType(GL_DOUBLE, GL_DOUBLE); }
template<> _GLDataType _GetGLType<GfVec2d>()
        { return _GLDataType(GL_DOUBLE, GL_DOUBLE_VEC2); }
template<> _GLDataType _GetGLType<GfVec3d>()
        { return _GLDataType(GL_DOUBLE, GL_DOUBLE_VEC3); }
template<> _GLDataType _GetGLType<GfVec4d>()
        { return _GLDataType(GL_DOUBLE, GL_DOUBLE_VEC4); }
template<> _GLDataType _GetGLType<GfMatrix4f>()
        { return _GLDataType(GL_FLOAT, GL_FLOAT_MAT4); }
template<> _GLDataType _GetGLType<GfMatrix4d>()
        { return _GLDataType(GL_DOUBLE, GL_DOUBLE_MAT4); }
template<> _GLDataType _GetGLType<Hd_BSplinePatchIndex>()
        { return _GLDataType(GL_INT, GL_INT); }
template<> _GLDataType _GetGLType<HdVec4f_2_10_10_10_REV>()
        { return _GLDataType(GL_INT_2_10_10_10_REV, GL_INT_2_10_10_10_REV); }


// XXX: we don't have BOOST_TTI_HAS_MEMBER_DATA yet. (requires boost 1.54)
// http://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Member_Detector
#define HAS_MEMBER_DATA(name)                                           \
    template <typename T, bool Enabled=boost::is_class<T>::value>       \
    struct has_##name {   /* if T is int, float, double etc */          \
        static bool const value = false;                                \
    };                                                                  \
    template <typename T>                                               \
    struct has_##name<T, true> {  /* if T is class */                   \
        struct B { int name; };                                         \
        struct D : T, B { };                                            \
        template<typename C, C> struct ChT;                             \
        template<typename C> static char (&f(ChT<int B::*, &C::name>*))[1]; \
        template<typename C> static char (&f(...))[2];                  \
        static bool const value = sizeof(f<D>(0)) == 2;                 \
    };

// mpl helpers to check an existence of data member
HAS_MEMBER_DATA(dimension);
HAS_MEMBER_DATA(numRows);

// ------------------------------------------------------------------------- //
// Determine the number of components in each element, because the ElementTypes
// held by VtArray do not have compatible interfaces, the following methods are
// specialized for each category of element type: integral/float, GfVec*,
// GfMatrix* etc.

// Integral/Float types.
template <typename T>
short
_GetNumComponents(T const& array,
                  typename boost::enable_if<
                            boost::mpl::or_<
                                boost::is_integral<typename T::ElementType>,
                                boost::is_float<typename T::ElementType>
                                >
                            , T>::type* = 0) {
    return 1;
}

// GfVec* types. 
template <typename T>
short 
_GetNumComponents(T const& array, typename boost::enable_if<
                  has_dimension<typename T::ElementType> >::type* = 0) {
    typedef typename T::ElementType ET;
    return ET::dimension;
}

// GfMatrix* types.
template <typename T>
short 
_GetNumComponents(T const& array, typename boost::enable_if<
                  has_numRows<typename T::ElementType> >::type* = 0) {
    typedef typename T::ElementType ET;
    return ET::numRows * ET::numColumns;
}

// ------------------------------------------------------------------------- //
// Helper class for extracting the data from a VtValue<T> or
// VtValue< VtArray<T> >.
// 
class _Extractor {
public:
    _Extractor()
        : _glDataType(0, 0)
        , _size(0)
        , _numComponents(0)
        , _data(NULL)
    { }

    // mpl helpers to check an existence of nested type
    BOOST_MPL_HAS_XXX_TRAIT_DEF(ElementType);

    // single values (float, double, int)
    template <typename T>
    void
    Extract(T const& value,
            typename boost::enable_if<
              boost::mpl::or_<
                boost::is_integral<T>, boost::is_float<T> > >::type* = 0){
        // This method is only called once, with the actual value in the
        // VtValue
        TF_VERIFY(_data == NULL);
        TF_VERIFY(_numComponents == 0);

        // size of single value in interleaved struct rounds up to
        // sizeof(GLint) according to GL spec.
        _size = std::max(sizeof(T), sizeof(GLint));
        _numComponents = 1;
        _glDataType = _GetGLType<T>();

        // Hold a pointer to the heldtype
        _data = &value;
    }

    // vector
    template <typename T>
    void
    Extract(T const& value,
            typename boost::enable_if<has_dimension<T> >::type* = 0) {
        // This method is only called once, with the actual vector in the
        // VtValue
        TF_VERIFY(_data == NULL);
        TF_VERIFY(_numComponents == 0);

        // Calculate the size.
        _size = sizeof(T);
        _numComponents = T::dimension;
        _glDataType = _GetGLType<typename T::ScalarType>();

        // Hold a pointer to the GfVec
        _data = value.GetArray();
    }

    // matrix
    template <typename T>
    void
    Extract(T const& value,
            typename boost::enable_if<has_numRows<T> >::type* = 0) {
        // This method is only called once, with the actual matrix held in the
        // VtValue.
        TF_VERIFY(_data == NULL);
        TF_VERIFY(_numComponents == 0);

        // Calculate the size.
        _size = sizeof(T);
        _numComponents = T::numRows * T::numColumns;
        _glDataType = _GetGLType<typename T::ScalarType>();

        // Hold a pointer to the GfMatrix
        _data = value.GetArray();
    }

    // array
    template <typename T>
    void
    Extract(T const& array,
            typename boost::enable_if<has_ElementType<T> >::type* = 0) {
        typedef typename T::ElementType ElementType;

        // This method is only called once, with the actual array held in the
        // VtValue.
        TF_VERIFY(_data == NULL);
        TF_VERIFY(_numComponents == 0);

        // Calculate the size.
        _size = array.size() * sizeof(ElementType);

        // Extract data that requires additional dispatch based on the element
        // type.
        _numComponents = _GetNumComponents(array);
        _glDataType = _GetGLType<ElementType>();

        // Hold a pointer to the internal storage of the VtArray. A new buffer
        // could be allocated at this point, if holding the VtValue has
        // negative side effects.
        if (array.size() > 0) _data = array.cdata();
    }

    _GLDataType GetGLDataType() {return _glDataType;}
    size_t GetSize() {return _size;}
    short GetNumComponents() {return _numComponents;}
    void const* GetData() {return _data;}

private:

    _GLDataType _glDataType;
    size_t _size;
    short _numComponents;
    void const* _data;
};

// ------------------------------------------------------------------------- //
// Works in conjunction with _ExtractArrayData to detect the held type of a
// VtValue.
template<typename FUNCTOR>
struct _TypeChecker {
    _TypeChecker(VtValue const& value, FUNCTOR& extractor)
        : _value(value)
        , _extractor(extractor)
    {
        /*Nothing*/
    }

    template <typename T>
    void operator()(T) {
        // Check each type T, if the VtValue is holding the current T, then
        // bind the value to the extractor.
        if (_value.IsHolding<typename T::Type>())
            _extractor.Extract(_value.UncheckedGet<typename T::Type>());
    }

private:
    VtValue const& _value;
    FUNCTOR& _extractor;
};

static
_Extractor
_ExtractArrayData(VtValue const& value) {
    // Type dispatch to _Extractor.
    //
    // Iterate over each acceptable type T, calling typeChecker<T>(), which
    // internally calls value.IsHolding<T>(); if the VtValue IS holding a T,
    // then pass the held value to _Extractor::Extract<T>(value).
    _Extractor e;
    boost::mpl::for_each<HdVtBufferSource::AcceptedTypes
                         >(_TypeChecker<_Extractor>(value, e));
    // Guarantee that the _Extractor::Extract method was called exactly once
    // and issue a runtime error otherwise. (The only case where Extract wont
    // be called is when the VtValue is holding an unacceptable type).
    if (e.GetNumComponents() == 0) {
        TF_RUNTIME_ERROR("HdVtBufferSource initialized with VtValue holding "
                         "unacceptable type: %s",
                         value.GetType().GetTypeName().c_str());
    }
    return e;
}

// ------------------------------------------------------------------------- //
// HdVtBufferSource Implementation
// ------------------------------------------------------------------------- //

HdVtBufferSource::HdVtBufferSource(TfToken const& name, VtValue const& value,
                                   bool staticArray)
    : _name(name), _value(value), _staticArray(staticArray)
{
    HD_TRACE_FUNCTION();
    // Extract element size from value.
    // This also guarantees the VtValue is an accepted type.
    //
    // note: make sure to extract the pointer from [_value], not [value],
    // since if the VtValue holds a tiny type whose size is less than
    // sizeof(void*), it uses LocalStorage and doesn't allocate a COW storage.
    // In that case the pointer to the held type of [value] is on stack,
    // would be invalid.
    _Extractor arrayInfo = _ExtractArrayData(_value);

    // Copy the values out of the extractor.
    _glComponentDataType = arrayInfo.GetGLDataType().componentType;
    _glElementDataType = arrayInfo.GetGLDataType().elementType;
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
        VtArray<GfMatrix4f> fmatrices(static_cast<unsigned int>(matrices.size()));
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
    return static_cast<int>(_size / (_numComponents *
                    HdConversions::GetComponentSize(_glComponentDataType)));
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
