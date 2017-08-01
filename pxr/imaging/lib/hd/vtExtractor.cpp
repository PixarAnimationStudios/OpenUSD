//
// Copyright 2017 Pixar
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

#include "pxr/imaging/hd/vtExtractor.h"

#include "pxr/imaging/hd/patchIndex.h"

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

#include "pxr/base/vt/array.h"
#include "pxr/base/vt/types.h"


#include "pxr/imaging/hd/glUtils.h"

#include <boost/mpl/vector/vector40.hpp>

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


PXR_NAMESPACE_OPEN_SCOPE


// TODO: Add support for scalar types (float, int, bool)

// TODO: Make this internal or perhaps use a TfHashMap/dispatch table.

// This struct is required to avoid instantiation of the actual types during
// type dispatch.
template <typename T> struct THolder {
typedef T Type;
};

// The valid types a HdBufferSource can be constructed from.
typedef boost::mpl::vector32<
        THolder<bool>,
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
        > _AcceptedTypes;

// ------------------------------------------------------------------------- //
// Generic helpers for extracting data from VtValue into raw byte array
// ------------------------------------------------------------------------- //
// What's happening here:
//
//   * A Buffer Source creates a Hd_VtExtractor and calls Extract() with a
//     VtValue to access type information and convert it to a raw byte array.
//
//   * Extract() does a type dispatch and checks if the VtValue
//     provided is holding one of the _AcceptedTypes
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
template<> _GLDataType _GetGLType<bool>()
        { return _GLDataType(GL_BOOL, GL_BOOL); }
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

Hd_VtExtractor::Hd_VtExtractor()
 : _glComponentType(0)
 , _glElementType(0)
 , _size(0)
 , _numComponents(0)
 , _data(nullptr)
{
}

void
Hd_VtExtractor::Extract(const VtValue &value)
{
    // Type dispatch to _Extractor.
    //
    // Iterate over each acceptable type T, calling typeChecker<T>(), which
    // internally calls value.IsHolding<T>(); if the VtValue IS holding a T,
    // then pass the held value to _Extractor::Extract<T>(value).
    _Extractor e;

    boost::mpl::for_each<_AcceptedTypes>(_TypeChecker<_Extractor>(value, e));

    // Guarantee that the _Extractor::Extract method was called exactly once
    // and issue a runtime error otherwise. (The only case where Extract wont
    // be called is when the VtValue is holding an unacceptable type).
    if (e.GetNumComponents() == 0) {
        TF_RUNTIME_ERROR("Trying to extract a VtValue holding "
                         "unacceptable type: %s",
                         value.GetType().GetTypeName().c_str());
        return;
    }

    const _GLDataType &glDataType  = e.GetGLDataType();

    _glComponentType = glDataType.componentType;
    _glElementType   = glDataType.elementType;
    _size            = e.GetSize();
    _numComponents   = e.GetNumComponents();
    _data            = e.GetData();
}

PXR_NAMESPACE_CLOSE_SCOPE
