//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/opaqueValue.h"
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
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/arrayEdit.h"
#include "pxr/base/vt/arrayEditBuilder.h"
#include "pxr/base/vt/value.h"

#include <algorithm>
#include <array>
#include <map>

PXR_NAMESPACE_OPEN_SCOPE

namespace Sdf_ParserHelpers {

using std::string;
using std::vector;

// Check that there are enough values to parse so we don't overflow
#define CHECK_BOUNDS(count, name)                                          \
    if (index + count > vars.size()) {                                     \
        TF_CODING_ERROR("Not enough values to parse value of type %s",     \
                        name);                                             \
        throw std::bad_variant_access();                                            \
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
MakeScalarValueImpl(GfHalf *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(1, "half");
    *out = GfHalf(vars[index++].Get<float>());
}

inline void
MakeScalarValueImpl(
    SdfTimeCode *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(1, "timecode");
    *out = SdfTimeCode(vars[index++].Get<double>());
}

template <class Int>
inline std::enable_if_t<std::is_integral<Int>::value>
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
    (*out)[0] = GfHalf(vars[index++].Get<float>());
    (*out)[1] = GfHalf(vars[index++].Get<float>());
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
    (*out)[0] = GfHalf(vars[index++].Get<float>());
    (*out)[1] = GfHalf(vars[index++].Get<float>());
    (*out)[2] = GfHalf(vars[index++].Get<float>());
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
    (*out)[0] = GfHalf(vars[index++].Get<float>());
    (*out)[1] = GfHalf(vars[index++].Get<float>());
    (*out)[2] = GfHalf(vars[index++].Get<float>());
    (*out)[3] = GfHalf(vars[index++].Get<float>());
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
    GfVec3h imag; GfHalf re;
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

inline void
MakeScalarValueImpl(
    SdfPathExpression *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(1, "pathExpression");
    *out = SdfPathExpression(vars[index++].Get<std::string>());
}

inline void
MakeScalarValueImpl(
    SdfOpaqueValue *out, vector<Value> const &vars, size_t &index) {
    TF_CODING_ERROR("Found authored opinion for opaque attribute");
    throw std::bad_variant_access();
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
    } catch (const std::bad_variant_access &) {
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
    } catch (const std::bad_variant_access &) {
        *errStrPtr = TfStringPrintf("Failed to parse at element %zd "
                                    "(at sub-part %zd if there are "
                                    "multiple parts)", shapeIndex,
                                    (index - origIndex) - 1);
        return VtValue();
    }
    return VtValue(array);
}

ArrayEditFactoryBase::~ArrayEditFactoryBase() = default;

bool
ArrayEditFactoryBase::Append(VtValue const &elem)
{
    return Insert(elem, Vt_ArrayEditOps::EndIndex);
}
void
ArrayEditFactoryBase::AppendRef(int64_t srcIndex)
{
    InsertRef(srcIndex, Vt_ArrayEditOps::EndIndex);
}

bool
ArrayEditFactoryBase::Prepend(VtValue const &elem)
{
    return Insert(elem, 0);
}
void
ArrayEditFactoryBase::PrependRef(int64_t srcIndex)
{
    InsertRef(srcIndex, 0);
}

// ArrayEditFactory implementations
template <class T>
struct ArrayEditFactory : ArrayEditFactoryBase
{
    ~ArrayEditFactory() override = default;

    bool Write(VtValue const &elem, int64_t index) override {
        return _TypeCheck(elem) &&
            (_builder.Write(elem.UncheckedGet<T>(), index), true);
    }
    void WriteRef(int64_t srcIndex, int64_t dstIndex) override {
        _builder.WriteRef(srcIndex, dstIndex);
    }
        
    bool Insert(VtValue const &elem, int64_t index) override {
        return _TypeCheck(elem) &&
            (_builder.Insert(elem.UncheckedGet<T>(), index), true);
    }
    void InsertRef(int64_t srcIndex, int64_t dstIndex) override {
        _builder.InsertRef(srcIndex, dstIndex);
    }

    void EraseRef(int64_t index) override {
        _builder.EraseRef(index);
    }

    void MinSize(int64_t size) override {
        _builder.MinSize(size);
    }
    bool MinSizeFill(int64_t size, VtValue const &fill) override {
        return _TypeCheck(fill) &&
            (_builder.MinSize(size, fill.UncheckedGet<T>()), true);
    }

    void MaxSize(int64_t size) override {
        _builder.MaxSize(size);
    }

    void SetSize(int64_t size) override {
        _builder.SetSize(size);
    }
    bool SetSizeFill(int64_t size, VtValue const &fill) override {
        return _TypeCheck(fill) &&
            (_builder.SetSize(size, fill.UncheckedGet<T>()), true);
    }

    VtValue FinalizeAndReset() override {
        VtArrayEdit<T> edit = _builder.FinalizeAndReset();
        return _errMsg.empty() ? VtValue::Take(edit) : VtValue {};
    }

    std::string GetErrorMessage() const override {
        return _errMsg;
    }

private:
    bool _TypeCheck(VtValue const &val) {
        if (!val.IsHolding<T>()) {
            _errMsg = TfStringPrintf("Unexpected value '%s' of type '%s' "
                                     "building a VtArrayEdit<%s>",
                                     TfStringify(val).c_str(),
                                     val.GetTypeName().c_str(),
                                     ArchGetDemangled<T>().c_str());
            return false;
        }
        return true;
    }
        
    VtArrayEditBuilder<T> _builder;
    std::string _errMsg;
};

struct ValueAndArrayEditFactory {
    ValueFactory valueFactory;
    ArrayEditFactoryBase *(*makeArrayEditFactory)();
};

using _ValueFactoryMap = std::map<std::string, ValueAndArrayEditFactory>;

// Walk through types and register factories.
struct _MakeFactoryMap {

    explicit _MakeFactoryMap(_ValueFactoryMap *factories) :
        _factories(factories) {}

    template <class CppType, bool SupportsArrays=true>
    void Add(const SdfValueTypeName& scalar, const char* alias = nullptr) {
        constexpr bool isShaped = true;

        const std::string scalarName =
            alias ? std::string(alias) : scalar.GetAsToken().GetString();

        _ValueFactoryMap &f = *_factories;
        f[scalarName].valueFactory = ValueFactory(
            scalarName, scalar.GetDimensions(), !isShaped,
            MakeScalarValueTemplate<CppType>);

        if constexpr (SupportsArrays) {
            const SdfValueTypeName array = scalar.GetArrayType();

            const std::string arrayName = alias
                ? std::string(alias) + "[]"
                : array.GetAsToken().GetString();

            f[arrayName].valueFactory = ValueFactory(
                arrayName, array.GetDimensions(), isShaped,
                MakeShapedValueTemplate<CppType>);

            f[scalarName].makeArrayEditFactory =
                []() -> ArrayEditFactoryBase * {
                    return new ArrayEditFactory<CppType>;
                };
        }
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
    //     for (const auto& typeName : SdfSchema::GetInstance().GetAllTypes()) {
    //        builder(typeName);
    //    }
    //            For symmetry (and I think it would actually be useful
    //            when converting usd into other formats) there should be
    //            a method to convert a VtValue holding the appropriate C++
    //            type into a vector of VtValues holding a primitive type.
    //            E.g. a VtValue holding a GfVec3f would return three
    //            VtValues each holding a float.
    builder.Add<bool>(SdfValueTypeNames->Bool);
    builder.Add<uint8_t>(SdfValueTypeNames->UChar);
    builder.Add<int32_t>(SdfValueTypeNames->Int);
    builder.Add<uint32_t>(SdfValueTypeNames->UInt);
    builder.Add<int64_t>(SdfValueTypeNames->Int64);
    builder.Add<uint64_t>(SdfValueTypeNames->UInt64);
    builder.Add<GfHalf>(SdfValueTypeNames->Half);
    builder.Add<float>(SdfValueTypeNames->Float);
    builder.Add<double>(SdfValueTypeNames->Double);
    builder.Add<SdfTimeCode>(SdfValueTypeNames->TimeCode);
    builder.Add<std::string>(SdfValueTypeNames->String);
    builder.Add<TfToken>(SdfValueTypeNames->Token);
    builder.Add<SdfAssetPath>(SdfValueTypeNames->Asset);
    builder.Add<SdfOpaqueValue,
                /*SupportsArrays=*/false>(SdfValueTypeNames->Opaque);
    builder.Add<SdfOpaqueValue,
                /*SupportsArrays=*/false>(SdfValueTypeNames->Group);
    builder.Add<SdfPathExpression>(SdfValueTypeNames->PathExpression);

    builder.Add<GfVec2i>(SdfValueTypeNames->Int2);
    builder.Add<GfVec2h>(SdfValueTypeNames->Half2);
    builder.Add<GfVec2f>(SdfValueTypeNames->Float2);
    builder.Add<GfVec2d>(SdfValueTypeNames->Double2);
    builder.Add<GfVec3i>(SdfValueTypeNames->Int3);
    builder.Add<GfVec3h>(SdfValueTypeNames->Half3);
    builder.Add<GfVec3f>(SdfValueTypeNames->Float3);
    builder.Add<GfVec3d>(SdfValueTypeNames->Double3);
    builder.Add<GfVec4i>(SdfValueTypeNames->Int4);
    builder.Add<GfVec4h>(SdfValueTypeNames->Half4);
    builder.Add<GfVec4f>(SdfValueTypeNames->Float4);
    builder.Add<GfVec4d>(SdfValueTypeNames->Double4);
    builder.Add<GfVec3h>(SdfValueTypeNames->Point3h);
    builder.Add<GfVec3f>(SdfValueTypeNames->Point3f);
    builder.Add<GfVec3d>(SdfValueTypeNames->Point3d);
    builder.Add<GfVec3h>(SdfValueTypeNames->Vector3h);
    builder.Add<GfVec3f>(SdfValueTypeNames->Vector3f);
    builder.Add<GfVec3d>(SdfValueTypeNames->Vector3d);
    builder.Add<GfVec3h>(SdfValueTypeNames->Normal3h);
    builder.Add<GfVec3f>(SdfValueTypeNames->Normal3f);
    builder.Add<GfVec3d>(SdfValueTypeNames->Normal3d);
    builder.Add<GfVec3h>(SdfValueTypeNames->Color3h);
    builder.Add<GfVec3f>(SdfValueTypeNames->Color3f);
    builder.Add<GfVec3d>(SdfValueTypeNames->Color3d);
    builder.Add<GfVec4h>(SdfValueTypeNames->Color4h);
    builder.Add<GfVec4f>(SdfValueTypeNames->Color4f);
    builder.Add<GfVec4d>(SdfValueTypeNames->Color4d);
    builder.Add<GfQuath>(SdfValueTypeNames->Quath);
    builder.Add<GfQuatf>(SdfValueTypeNames->Quatf);
    builder.Add<GfQuatd>(SdfValueTypeNames->Quatd);
    builder.Add<GfMatrix2d>(SdfValueTypeNames->Matrix2d);
    builder.Add<GfMatrix3d>(SdfValueTypeNames->Matrix3d);
    builder.Add<GfMatrix4d>(SdfValueTypeNames->Matrix4d);
    builder.Add<GfMatrix4d>(SdfValueTypeNames->Frame4d);
    builder.Add<GfVec2f>(SdfValueTypeNames->TexCoord2f);
    builder.Add<GfVec2d>(SdfValueTypeNames->TexCoord2d);
    builder.Add<GfVec2h>(SdfValueTypeNames->TexCoord2h);
    builder.Add<GfVec3f>(SdfValueTypeNames->TexCoord3f);
    builder.Add<GfVec3d>(SdfValueTypeNames->TexCoord3d);
    builder.Add<GfVec3h>(SdfValueTypeNames->TexCoord3h);

    // XXX: Backwards compatibility.  These should be removed when
    //      all assets are updated.  At the time of this writing
    //      under pxr only assets used by usdImaging need updating.
    //      Those assets must be moved anyway for open sourcing so
    //      I'm leaving this for now.  (Also note that at least one
    //      of those tests, testUsdImagingEmptyMesh, uses the prim
    //      type PxVolume which is not in pxr.)  Usd assets outside
    //      pxr must also be updated.
    builder.Add<GfVec2i>(SdfValueTypeNames->Int2, "Vec2i");
    builder.Add<GfVec2h>(SdfValueTypeNames->Half2, "Vec2h");
    builder.Add<GfVec2f>(SdfValueTypeNames->Float2, "Vec2f");
    builder.Add<GfVec2d>(SdfValueTypeNames->Double2, "Vec2d");
    builder.Add<GfVec3i>(SdfValueTypeNames->Int3, "Vec3i");
    builder.Add<GfVec3h>(SdfValueTypeNames->Half3, "Vec3h");
    builder.Add<GfVec3f>(SdfValueTypeNames->Float3, "Vec3f");
    builder.Add<GfVec3d>(SdfValueTypeNames->Double3, "Vec3d");
    builder.Add<GfVec4i>(SdfValueTypeNames->Int4, "Vec4i");
    builder.Add<GfVec4h>(SdfValueTypeNames->Half4, "Vec4h");
    builder.Add<GfVec4f>(SdfValueTypeNames->Float4, "Vec4f");
    builder.Add<GfVec4d>(SdfValueTypeNames->Double4, "Vec4d");
    builder.Add<GfVec3f>(SdfValueTypeNames->Point3f, "PointFloat");
    builder.Add<GfVec3d>(SdfValueTypeNames->Point3d, "Point");
    builder.Add<GfVec3f>(SdfValueTypeNames->Vector3f, "NormalFloat");
    builder.Add<GfVec3d>(SdfValueTypeNames->Vector3d, "Normal");
    builder.Add<GfVec3f>(SdfValueTypeNames->Normal3f, "VectorFloat");
    builder.Add<GfVec3d>(SdfValueTypeNames->Normal3d, "Vector");
    builder.Add<GfVec3f>(SdfValueTypeNames->Color3f, "ColorFloat");
    builder.Add<GfVec3d>(SdfValueTypeNames->Color3d, "Color");
    builder.Add<GfQuath>(SdfValueTypeNames->Quath, "Quath");
    builder.Add<GfQuatf>(SdfValueTypeNames->Quatf, "Quatf");
    builder.Add<GfQuatd>(SdfValueTypeNames->Quatd, "Quatd");
    builder.Add<GfMatrix2d>(SdfValueTypeNames->Matrix2d, "Matrix2d");
    builder.Add<GfMatrix3d>(SdfValueTypeNames->Matrix3d, "Matrix3d");
    builder.Add<GfMatrix4d>(SdfValueTypeNames->Matrix4d, "Matrix4d");
    builder.Add<GfMatrix4d>(SdfValueTypeNames->Frame4d, "Frame");
    builder.Add<GfMatrix4d>(SdfValueTypeNames->Matrix4d, "Transform");
    builder.Add<int>(SdfValueTypeNames->Int, "PointIndex");
    builder.Add<int>(SdfValueTypeNames->Int, "EdgeIndex");
    builder.Add<int>(SdfValueTypeNames->Int, "FaceIndex");
    builder.Add<TfToken>(SdfValueTypeNames->Token, "Schema");

    // Set up the special None factory.
    (*_valueFactories)[std::string("None")].valueFactory = ValueFactory(
        std::string(), SdfTupleDimensions(), false, NULL);

}

ValueFactory const &GetValueFactoryForMenvaName(std::string const &name,
                                                bool *found)
{
    _ValueFactoryMap::const_iterator it = _valueFactories->find(name);
    if (it != _valueFactories->end()) {
        *found = true;
        return it->second.valueFactory;
    }
    
    // No factory for given name.
    static ValueFactory const& none = (*_valueFactories)["None"].valueFactory;
    *found = false;
    return none;
}

std::unique_ptr<ArrayEditFactoryBase>
MakeArrayEditFactoryForMenvaName(std::string const &name)
{
    _ValueFactoryMap::const_iterator it = _valueFactories->find(name);
    if (it != _valueFactories->end() && it->second.makeArrayEditFactory) {
        return std::unique_ptr<ArrayEditFactoryBase> {
            it->second.makeArrayEditFactory()
        };
    }
    // No factory.
    return nullptr;
}    

} // namespace Sdf_ParserHelpers

bool
Sdf_BoolFromString( const std::string &str, bool *parseOk )
{
    if (parseOk)
        *parseOk = true;

    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);

    if (strcmp(s.c_str(), "false") == 0)
        return false;
    if (strcmp(s.c_str(), "true") == 0)
        return true;
    if (strcmp(s.c_str(), "no") == 0)
        return false;
    if (strcmp(s.c_str(), "yes") == 0)
        return true;

    if (strcmp(s.c_str(), "0") == 0)
        return false;
    if (strcmp(s.c_str(), "1") == 0)
        return true;

    if (parseOk)
        *parseOk = false;
    return true;
}

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
    static const size_t LocalSize = 2048;
    char localBuf[LocalSize];
    char *buf = n <= LocalSize ? localBuf : (char *)malloc(n);

    char *s = buf;

    const char *p = x + trimBothSides;
    const char * const end = x + trimBothSides + n;

    while (p < end) {
        const char *escOrEnd =
            static_cast<const char *>(memchr(p, '\\', std::distance(p, end)));
        if (!escOrEnd) {
            escOrEnd = end;
        }
               
        const size_t nchars = std::distance(p, escOrEnd);
        memcpy(s, p, nchars);
        s += nchars;
        p += nchars;

        if (escOrEnd != end) {
            TfEscapeStringReplaceChar(&p, &s);
            ++p;
        }
    }

    // Trim to final length.
    std::string(buf, s-buf).swap(ret);
    if (buf != localBuf) {
        free(buf);
    }

    if (numLines) {
        *numLines = std::count(ret.begin(), ret.end(), '\n');
    }
    
    return ret;
}

std::string 
Sdf_EvalAssetPath(const char* x, size_t n, bool tripleDelimited)
{
    // See _StringFromAssetPath for the code that writes asset paths.

    // Asset paths are assumed to only contain printable characters and 
    // no escape sequences except for the "@@@" delimiter.
    const int numDelimiters = tripleDelimited ? 3 : 1;
    std::string ret(x + numDelimiters, n - (2 * numDelimiters));
    if (tripleDelimited) {
        ret = TfStringReplace(ret, "\\@@@", "@@@");
    }

    // Go through SdfAssetPath for validation -- this will raise an error and
    // produce the empty asset path if 'ret' contains invalid characters.
    return SdfAssetPath(ret).GetAssetPath();
}

PXR_NAMESPACE_CLOSE_SCOPE
