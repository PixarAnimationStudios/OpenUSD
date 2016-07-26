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
#include "pxr/usd/sdf/parserHelpers.h"
#include "pxr/usd/sdf/schema.h"

#include "pxr/base/gf/half.h"
#include "pxr/base/gf/matrix2d.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/quatd.h"
#include "pxr/base/gf/quatf.h"
#include "pxr/base/gf/quath.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2h.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3h.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4h.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/mpl/for_each.hpp>

#include <utility>

namespace Sdf_ParserHelpers {

using std::pair;
using std::make_pair;

using boost::get;

// Check that there are enough values to parse so we don't overflow
#define CHECK_BOUNDS(count, name)                                          \
    if (index + count > vars.size()) {                                     \
        TF_CODING_ERROR("Not enough values to parse value of type %s",     \
                        name);                                             \
        throw boost::bad_get();                                            \
    }

inline void
MakeScalarValueImpl(string *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(1, "string");
    *out = vars[index++].Get<std::string>();
}

inline void
MakeScalarValueImpl(TfToken *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(1, "token");
    *out = TfToken(vars[index++].Get<std::string>());
}

inline void
MakeScalarValueImpl(double *out,
                                vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(1, "double");
    *out = vars[index++].Get<double>();
}

inline void
MakeScalarValueImpl(float *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(1, "float");
    *out = vars[index++].Get<float>();
}

inline void
MakeScalarValueImpl(half *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(1, "half");
    *out = half(vars[index++].Get<float>());
}

template <class Int>
inline typename boost::enable_if<boost::is_integral<Int> >::type
MakeScalarValueImpl(Int *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(1, ArchGetDemangled<Int>().c_str());
    *out = vars[index++].Get<Int>();
}

inline void
MakeScalarValueImpl(GfVec2d *out,
                                vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(2, "Vec2d");
    (*out)[0] = vars[index++].Get<double>();
    (*out)[1] = vars[index++].Get<double>();
}

inline void
MakeScalarValueImpl(GfVec2f *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(2, "Vec2f");
    (*out)[0] = vars[index++].Get<float>();
    (*out)[1] = vars[index++].Get<float>();
}

inline void
MakeScalarValueImpl(GfVec2h *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(2, "Vec2h");
    (*out)[0] = half(vars[index++].Get<float>());
    (*out)[1] = half(vars[index++].Get<float>());
}

inline void
MakeScalarValueImpl(GfVec2i *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(2, "Vec2i");
    (*out)[0] = vars[index++].Get<int>();
    (*out)[1] = vars[index++].Get<int>();
}

inline void
MakeScalarValueImpl(GfVec3d *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(3, "Vec3d");
    (*out)[0] = vars[index++].Get<double>();
    (*out)[1] = vars[index++].Get<double>();
    (*out)[2] = vars[index++].Get<double>();
}

inline void
MakeScalarValueImpl(GfVec3f *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(3, "Vec3f");
    (*out)[0] = vars[index++].Get<float>();
    (*out)[1] = vars[index++].Get<float>();
    (*out)[2] = vars[index++].Get<float>();
}

inline void
MakeScalarValueImpl(GfVec3h *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(3, "Vec3h");
    (*out)[0] = half(vars[index++].Get<float>());
    (*out)[1] = half(vars[index++].Get<float>());
    (*out)[2] = half(vars[index++].Get<float>());
}

inline void
MakeScalarValueImpl(GfVec3i *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(3, "Vec3i");
    (*out)[0] = vars[index++].Get<int>();
    (*out)[1] = vars[index++].Get<int>();
    (*out)[2] = vars[index++].Get<int>();
}

inline void
MakeScalarValueImpl(GfVec4d *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(4, "Vec4d");
    (*out)[0] = vars[index++].Get<double>();
    (*out)[1] = vars[index++].Get<double>();
    (*out)[2] = vars[index++].Get<double>();
    (*out)[3] = vars[index++].Get<double>();
}

inline void
MakeScalarValueImpl(GfVec4f *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(4, "Vec4f");
    (*out)[0] = vars[index++].Get<float>();
    (*out)[1] = vars[index++].Get<float>();
    (*out)[2] = vars[index++].Get<float>();
    (*out)[3] = vars[index++].Get<float>();
}

inline void
MakeScalarValueImpl(GfVec4h *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(4, "Vec4h");
    (*out)[0] = half(vars[index++].Get<float>());
    (*out)[1] = half(vars[index++].Get<float>());
    (*out)[2] = half(vars[index++].Get<float>());
    (*out)[3] = half(vars[index++].Get<float>());
}

inline void
MakeScalarValueImpl(GfVec4i *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(4, "Vec4i");
    (*out)[0] = vars[index++].Get<int>();
    (*out)[1] = vars[index++].Get<int>();
    (*out)[2] = vars[index++].Get<int>();
    (*out)[3] = vars[index++].Get<int>();
}

inline void
MakeScalarValueImpl(GfMatrix2d *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(4, "Matrix2d");
    (*out)[0][0] = vars[index++].Get<double>();
    (*out)[0][1] = vars[index++].Get<double>();
    (*out)[1][0] = vars[index++].Get<double>();
    (*out)[1][1] = vars[index++].Get<double>();
}

inline void
MakeScalarValueImpl(GfMatrix3d *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(9, "Matrix3d");
    (*out)[0][0] = vars[index++].Get<double>();
    (*out)[0][1] = vars[index++].Get<double>();
    (*out)[0][2] = vars[index++].Get<double>();
    (*out)[1][0] = vars[index++].Get<double>();
    (*out)[1][1] = vars[index++].Get<double>();
    (*out)[1][2] = vars[index++].Get<double>();
    (*out)[2][0] = vars[index++].Get<double>();
    (*out)[2][1] = vars[index++].Get<double>();
    (*out)[2][2] = vars[index++].Get<double>();
}

inline void
MakeScalarValueImpl(GfMatrix4d *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(16, "Matrix4d");
    (*out)[0][0] = vars[index++].Get<double>();
    (*out)[0][1] = vars[index++].Get<double>();
    (*out)[0][2] = vars[index++].Get<double>();
    (*out)[0][3] = vars[index++].Get<double>();
    (*out)[1][0] = vars[index++].Get<double>();
    (*out)[1][1] = vars[index++].Get<double>();
    (*out)[1][2] = vars[index++].Get<double>();
    (*out)[1][3] = vars[index++].Get<double>();
    (*out)[2][0] = vars[index++].Get<double>();
    (*out)[2][1] = vars[index++].Get<double>();
    (*out)[2][2] = vars[index++].Get<double>();
    (*out)[2][3] = vars[index++].Get<double>();
    (*out)[3][0] = vars[index++].Get<double>();
    (*out)[3][1] = vars[index++].Get<double>();
    (*out)[3][2] = vars[index++].Get<double>();
    (*out)[3][3] = vars[index++].Get<double>();
}

inline void
MakeScalarValueImpl(GfQuatd *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(4, "Quatd");
    // Values in order are re, i, j, k.
    GfVec3d imag; double re;
    MakeScalarValueImpl(&re, vars, index);
    out->SetReal(re);
    MakeScalarValueImpl(&imag, vars, index);
    out->SetImaginary(imag);
}

inline void
MakeScalarValueImpl(GfQuatf *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(4, "Quatf");
    // Values in order are re, i, j, k.
    GfVec3f imag; float re;
    MakeScalarValueImpl(&re, vars, index);
    out->SetReal(re);
    MakeScalarValueImpl(&imag, vars, index);
    out->SetImaginary(imag);
}

inline void
MakeScalarValueImpl(GfQuath *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(4, "Quath");
    // Values in order are re, i, j, k.
    GfVec3h imag; half re;
    MakeScalarValueImpl(&re, vars, index);
    out->SetReal(re);
    MakeScalarValueImpl(&imag, vars, index);
    out->SetImaginary(imag);
}

inline void
MakeScalarValueImpl(
    SdfAssetPath *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(1, "asset");
    *out = vars[index++].Get<SdfAssetPath>();
}

template <typename T>
inline VtValue
MakeScalarValueTemplate(vector<unsigned int> const &,
                        vector<Value> const &vars, size_t &index,
                        string *errStrPtr) {
    T t;
    size_t origIndex = index;
    try {
        MakeScalarValueImpl(&t, vars, index);
    } catch (const boost::bad_get &) {
        *errStrPtr = TfStringPrintf("Failed to parse value (at sub-part %zd "
                                    "if there are multiple parts)",
                                    (index - origIndex) - 1);
        return VtValue();
    }
    return VtValue(t);
}

template <typename T>
inline VtValue
MakeShapedValueTemplate(vector<unsigned int> const &shape,
                        vector<Value> const &vars, size_t &index,
                        string *errStrPtr) {
    if (shape.empty())
        return VtValue(VtArray<T>());
//    TF_AXIOM(shape.size() == 1);
    unsigned int size = 1;
    TF_FOR_ALL(i, shape)
        size *= *i;

    VtArray<T> array(size);
    size_t shapeIndex = 0;
    size_t origIndex = index;
    try {
        TF_FOR_ALL(i, array) {
            MakeScalarValueImpl(&(*i), vars, index);
            shapeIndex++;
        }
    } catch (const boost::bad_get &) {
        *errStrPtr = TfStringPrintf("Failed to parse at element %zd "
                                    "(at sub-part %zd if there are "
                                    "multiple parts)", shapeIndex,
                                    (index - origIndex) - 1);
        return VtValue();
    }
    return VtValue(array);
}

typedef map<std::string, ValueFactory> _ValueFactoryMap;

// Walk through types and register factories.
struct _MakeFactoryMap {

    explicit _MakeFactoryMap(_ValueFactoryMap *factories) :
        _factories(factories) {}

    template <class CppType>
    void add(const SdfValueTypeName& scalar, const char* alias = NULL)
    {
        static const bool isShaped = true;

        const SdfValueTypeName array = scalar.GetArrayType();

        const std::string scalarName =
            alias ? std::string(alias)        : scalar.GetAsToken().GetString();
        const std::string arrayName =
            alias ? std::string(alias) + "[]" : array.GetAsToken().GetString();

        _ValueFactoryMap &f = *_factories;
        f[scalarName] =
            ValueFactory(scalarName, scalar.GetDimensions(), not isShaped,
                         boost::bind(MakeScalarValueTemplate<CppType>,
                                     _1, _2, _3, _4));
        f[arrayName] =
            ValueFactory(arrayName, array.GetDimensions(), isShaped,
                         boost::bind(MakeShapedValueTemplate<CppType>,
                                     _1, _2, _3, _4));
    }
    
    _ValueFactoryMap *_factories;
};

TF_MAKE_STATIC_DATA(_ValueFactoryMap, _valueFactories) {
    _MakeFactoryMap builder(_valueFactories);
    // XXX: Would be better if SdfValueTypeName had a method to take
    //      a vector of VtValues and return a VtValue holding the
    //      appropriate C++ type (which mostly involves moving the
    //      MakeScalarValueImpl functions into the value type name
    //      registration code).  Then we could do this:
    //    BOOST_FOREACH(const SdfValueTypeName& typeName,
    //                  SdfSchema::GetInstance().GetAllTypes()) {
    //        builder(typeName);
    //    }
    //            For symmetry (and I think it would actually be useful
    //            when converting usd into other formats) there should be
    //            a method to convert a VtValue holding the appropriate C++
    //            type into a vector of VtValues holding a primitive type.
    //            E.g. a VtValue holding a GfVec3f would return three
    //            VtValues each holding a float.
    builder.add<bool>(SdfValueTypeNames->Bool);
    builder.add<uint8_t>(SdfValueTypeNames->UChar);
    builder.add<int32_t>(SdfValueTypeNames->Int);
    builder.add<uint32_t>(SdfValueTypeNames->UInt);
    builder.add<int64_t>(SdfValueTypeNames->Int64);
    builder.add<uint64_t>(SdfValueTypeNames->UInt64);
    builder.add<half>(SdfValueTypeNames->Half);
    builder.add<float>(SdfValueTypeNames->Float);
    builder.add<double>(SdfValueTypeNames->Double);
    builder.add<std::string>(SdfValueTypeNames->String);
    builder.add<TfToken>(SdfValueTypeNames->Token);
    builder.add<SdfAssetPath>(SdfValueTypeNames->Asset);
    builder.add<GfVec2i>(SdfValueTypeNames->Int2);
    builder.add<GfVec2h>(SdfValueTypeNames->Half2);
    builder.add<GfVec2f>(SdfValueTypeNames->Float2);
    builder.add<GfVec2d>(SdfValueTypeNames->Double2);
    builder.add<GfVec3i>(SdfValueTypeNames->Int3);
    builder.add<GfVec3h>(SdfValueTypeNames->Half3);
    builder.add<GfVec3f>(SdfValueTypeNames->Float3);
    builder.add<GfVec3d>(SdfValueTypeNames->Double3);
    builder.add<GfVec4i>(SdfValueTypeNames->Int4);
    builder.add<GfVec4h>(SdfValueTypeNames->Half4);
    builder.add<GfVec4f>(SdfValueTypeNames->Float4);
    builder.add<GfVec4d>(SdfValueTypeNames->Double4);
    builder.add<GfVec3h>(SdfValueTypeNames->Point3h);
    builder.add<GfVec3f>(SdfValueTypeNames->Point3f);
    builder.add<GfVec3d>(SdfValueTypeNames->Point3d);
    builder.add<GfVec3h>(SdfValueTypeNames->Vector3h);
    builder.add<GfVec3f>(SdfValueTypeNames->Vector3f);
    builder.add<GfVec3d>(SdfValueTypeNames->Vector3d);
    builder.add<GfVec3h>(SdfValueTypeNames->Normal3h);
    builder.add<GfVec3f>(SdfValueTypeNames->Normal3f);
    builder.add<GfVec3d>(SdfValueTypeNames->Normal3d);
    builder.add<GfVec3h>(SdfValueTypeNames->Color3h);
    builder.add<GfVec3f>(SdfValueTypeNames->Color3f);
    builder.add<GfVec3d>(SdfValueTypeNames->Color3d);
    builder.add<GfVec4h>(SdfValueTypeNames->Color4h);
    builder.add<GfVec4f>(SdfValueTypeNames->Color4f);
    builder.add<GfVec4d>(SdfValueTypeNames->Color4d);
    builder.add<GfQuath>(SdfValueTypeNames->Quath);
    builder.add<GfQuatf>(SdfValueTypeNames->Quatf);
    builder.add<GfQuatd>(SdfValueTypeNames->Quatd);
    builder.add<GfMatrix2d>(SdfValueTypeNames->Matrix2d);
    builder.add<GfMatrix3d>(SdfValueTypeNames->Matrix3d);
    builder.add<GfMatrix4d>(SdfValueTypeNames->Matrix4d);
    builder.add<GfMatrix4d>(SdfValueTypeNames->Frame4d);

    // XXX: Backwards compatibility.  These should be removed when
    //      all assets are updated.  At the time of this writing
    //      under pxr only assets used by usdImaging need updating.
    //      Those assets must be moved anyway for open sourcing so
    //      I'm leaving this for now.  (Also note that at least one
    //      of those tests, testUsdImagingEmptyMesh, uses the prim
    //      type PxVolume which is not in pxr.)  Usd assets outside
    //      pxr must also be updated.
    builder.add<GfVec2i>(SdfValueTypeNames->Int2, "Vec2i");
    builder.add<GfVec2h>(SdfValueTypeNames->Half2, "Vec2h");
    builder.add<GfVec2f>(SdfValueTypeNames->Float2, "Vec2f");
    builder.add<GfVec2d>(SdfValueTypeNames->Double2, "Vec2d");
    builder.add<GfVec3i>(SdfValueTypeNames->Int3, "Vec3i");
    builder.add<GfVec3h>(SdfValueTypeNames->Half3, "Vec3h");
    builder.add<GfVec3f>(SdfValueTypeNames->Float3, "Vec3f");
    builder.add<GfVec3d>(SdfValueTypeNames->Double3, "Vec3d");
    builder.add<GfVec4i>(SdfValueTypeNames->Int4, "Vec4i");
    builder.add<GfVec4h>(SdfValueTypeNames->Half4, "Vec4h");
    builder.add<GfVec4f>(SdfValueTypeNames->Float4, "Vec4f");
    builder.add<GfVec4d>(SdfValueTypeNames->Double4, "Vec4d");
    builder.add<GfVec3f>(SdfValueTypeNames->Point3f, "PointFloat");
    builder.add<GfVec3d>(SdfValueTypeNames->Point3d, "Point");
    builder.add<GfVec3f>(SdfValueTypeNames->Vector3f, "NormalFloat");
    builder.add<GfVec3d>(SdfValueTypeNames->Vector3d, "Normal");
    builder.add<GfVec3f>(SdfValueTypeNames->Normal3f, "VectorFloat");
    builder.add<GfVec3d>(SdfValueTypeNames->Normal3d, "Vector");
    builder.add<GfVec3f>(SdfValueTypeNames->Color3f, "ColorFloat");
    builder.add<GfVec3d>(SdfValueTypeNames->Color3d, "Color");
    builder.add<GfQuath>(SdfValueTypeNames->Quath, "Quath");
    builder.add<GfQuatf>(SdfValueTypeNames->Quatf, "Quatf");
    builder.add<GfQuatd>(SdfValueTypeNames->Quatd, "Quatd");
    builder.add<GfMatrix2d>(SdfValueTypeNames->Matrix2d, "Matrix2d");
    builder.add<GfMatrix3d>(SdfValueTypeNames->Matrix3d, "Matrix3d");
    builder.add<GfMatrix4d>(SdfValueTypeNames->Matrix4d, "Matrix4d");
    builder.add<GfMatrix4d>(SdfValueTypeNames->Frame4d, "Frame");
    builder.add<GfMatrix4d>(SdfValueTypeNames->Matrix4d, "Transform");
    builder.add<int>(SdfValueTypeNames->Int, "PointIndex");
    builder.add<int>(SdfValueTypeNames->Int, "EdgeIndex");
    builder.add<int>(SdfValueTypeNames->Int, "FaceIndex");
    builder.add<TfToken>(SdfValueTypeNames->Token, "Schema");

    // Set up the special None factory.
    (*_valueFactories)[std::string("None")] = ValueFactory(
        std::string(""), SdfTupleDimensions(), false, NULL);

}

ValueFactory const &GetValueFactoryForMenvaName(std::string const &name,
                                                bool *found)
{
    // XXX: This call is probably not needed anymore; constructing the
    // schema doesn't affect the value factories we register here.
    // I'm leaving this here for now to clean up in a separate change.
    SdfSchema::GetInstance();

    _ValueFactoryMap::const_iterator it = _valueFactories->find(name);
    if (it != _valueFactories->end()) {
        *found = true;
        return it->second;
    }
    
    // No factory for given name.
    static ValueFactory const& none = (*_valueFactories)[std::string("None")];
    *found = false;
    return none;
}

};


std::string
Sdf_EvalQuotedString(const char* x, size_t n, size_t trimBothSides, 
                     unsigned int* numLines)
{
    std::string ret;

    // Handle empty strings
    if (n <= 2 * trimBothSides)
        return ret;

    n -= 2 * trimBothSides;

    // Use local buf, or malloc one if not enough space.
    // (this is a little too much if there are escape chars in the string,
    // but we can live with it to avoid traversing the string twice)
    static const size_t LocalSize = 128;
    char localBuf[LocalSize];
    char *buf = n <= LocalSize ? localBuf : (char *)malloc(n);

    char *s = buf;
    for (const char *p = x + trimBothSides,
             *end = x + trimBothSides + n; p != end; ++p) {
        if (*p != '\\') {
            *s++ = *p;
        } else {
            TfEscapeStringReplaceChar(&p, &s);
        }
    }

    // Trim to final length.
    std::string(buf, s-buf).swap(ret);
    if (buf != localBuf)
        free(buf);

    if (numLines)
        *numLines = std::count(ret.begin(), ret.end(), '\n');
    
    return ret;
}
