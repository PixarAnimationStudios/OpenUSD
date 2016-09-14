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
/// \file alembicUtil.cpp

#include "pxr/usd/usdAbc/alembicUtil.h"
#include "pxr/base/tf/ostreamMethods.h"
#include <Alembic/Abc/IArrayProperty.h>
#include <Alembic/Abc/IScalarProperty.h>

#include <boost/foreach.hpp>

TF_DEFINE_PUBLIC_TOKENS(UsdAbc_AlembicContextFlagNames,
                        USDABC_ALEMBIC_CONTEXT_FLAG_NAMES);

namespace UsdAbc_AlembicUtil {

using namespace ::Alembic::Abc;

//
// Usd property value types.
//

// Supported Usd data types in conversions.  We simply take every Sdf
// value type and create tokens for the scalar and shaped (i.e. array)
// versions.  For example, we get the tokens named Bool with value "bool"
// and BoolArray with value "bool[]" for the bool type.
#define USD_MAKE_USD_TYPE(r, unused, elem) \
    ((SDF_VALUE_TAG(elem), SDF_VALUE_TRAITS_TYPE(elem)::Name())) \
    ((BOOST_PP_CAT(SDF_VALUE_TAG(elem), Array), SDF_VALUE_TRAITS_TYPE(elem)::ShapedName()))
TF_DEFINE_PUBLIC_TOKENS(UsdAbc_UsdDataTypes,
    BOOST_PP_SEQ_FOR_EACH(USD_MAKE_USD_TYPE, ~, SDF_VALUE_TYPES)
    _SDF_VALUE_TYPE_NAME_TOKENS
);
#undef USD_MAKE_USD_TYPE

TF_DEFINE_PUBLIC_TOKENS(UsdAbcPrimTypeNames, USD_ABC_PRIM_TYPE_NAMES);
TF_DEFINE_PUBLIC_TOKENS(UsdAbcPropertyNames, USD_ABC_PROPERTY_NAMES);
TF_DEFINE_PUBLIC_TOKENS(UsdAbcCustomMetadata, USD_ABC_CUSTOM_METADATA);

//
// UsdAbc_AlembicType
//

std::string
UsdAbc_AlembicType::Stringify() const
{
    if (extent == 1) {
        return TfStringPrintf("%s%s", PODName(pod), array ? "[]" : "");
    }
    else {
        return TfStringPrintf("%s[%d]%s",
                              PODName(pod), extent, array ? "[]" : "");
    }
}

bool
UsdAbc_AlembicType::operator==(const UsdAbc_AlembicType& rhs) const
{
    return pod    == rhs.pod and
           extent == rhs.extent and
           array  == rhs.array;
}

bool
UsdAbc_AlembicType::operator<(const UsdAbc_AlembicType& rhs) const
{
    if (pod        < rhs.pod) {
        return true;
    }
    if (rhs.pod    < pod) {
        return false;
    }
    if (extent     < rhs.extent) {
        return true;
    }
    if (rhs.extent < extent) {
        return false;
    }
    return array < rhs.array;
}

//
// Utilities
//

/// Format an Alembic version number as a string.
std::string
UsdAbc_FormatAlembicVersion(abc::int32_t n)
{
    return TfStringPrintf("%d.%d.%d", n / 10000, (n / 100) % 100, n % 100);
}

//
// POD property to/from Usd.
//

template <class UsdType, class AlembicType, size_t extent>
struct _ConvertPODScalar {
    bool operator()(const ICompoundProperty& parent, const std::string& name,
                    const ISampleSelector& iss,
                    const UsdAbc_AlembicDataAny& dst) const
    {
        // Something to hold the sample.
        AlembicType sample[extent];

        // Get the sample.
        IScalarProperty property(parent, name);
        property.get(sample, iss);

        // Copy to dst.
        return dst.Set(_ConvertPODToUsd<UsdType, AlembicType,extent>()(sample));
    }

    _SampleForAlembic operator()(const VtValue& src) const
    {
        return _ConvertPODFromUsdScalar<UsdType, AlembicType, extent>()(src);
    }
};

template <class UsdType, class AlembicType, size_t extent>
struct _ConvertPODArray {
    bool operator()(const ICompoundProperty& parent, const std::string& name,
                    const ISampleSelector& iss,
                    const UsdAbc_AlembicDataAny& dst) const
    {
        // Something to hold the sample.
        ArraySamplePtr sample;

        // Get the sample.
        IArrayProperty property(parent, name);
        property.get(sample, iss);

        // Copy each element.
        VtArray<UsdType> result(sample->size());
        _ConvertPODToUsdArray<UsdType, AlembicType, extent>()(
                result.data(), sample->getData(), sample->size());

        // Copy to dst.
        return dst.Set(result);
    }

    _SampleForAlembic operator()(const VtValue& src) const
    {
        return _ConvertPODFromUsdArray<UsdType, AlembicType, extent>()(src);
    }
};

//
// _SampleForAlembic
//

_SampleForAlembic::_Holder::~_Holder()
{
    // Do nothing
}

bool
_SampleForAlembic::_Holder::Error(std::string* message) const
{
    if (message) {
        message->clear();
    }
    return false;
}

_SampleForAlembic::_EmptyHolder::_EmptyHolder()
{
    // Do nothing
}

_SampleForAlembic::_EmptyHolder::~_EmptyHolder()
{
    // Do nothing
}

_SampleForAlembic::_ErrorHolder::_ErrorHolder(const std::string& message) :
    _message(message)
{
    // Do nothing
}

bool
_SampleForAlembic::_ErrorHolder::Error(std::string* message) const
{
    if (message) {
        *message = _message;
    }
    return true;
}

_SampleForAlembic::_ErrorHolder::~_ErrorHolder()
{
    // Do nothing
}

_SampleForAlembic::_VtValueHolder::~_VtValueHolder()
{
    // Do nothing
}

_SampleForAlembic
_ErrorSampleForAlembic(const std::string& msg)
{
    return _SampleForAlembic(_SampleForAlembic::Error(msg));
}

//
// Alembic <-> Usd conversion registries.
//

//
// UsdAbc_AlembicDataConversion
//

UsdAbc_AlembicDataConversion::UsdAbc_AlembicDataConversion()
{
    // Do nothing
}

SdfValueTypeName
UsdAbc_AlembicDataConversion::FindConverter(
    const UsdAbc_AlembicType& alembicType) const
{
    BOOST_FOREACH(const _ConverterData &c, _typeConverters) {
        if (c.abcType == alembicType) {
            return c.usdType;
        }
    }
    return SdfValueTypeName();
}

const UsdAbc_AlembicDataConversion::ToUsdConverter& 
UsdAbc_AlembicDataConversion::GetToUsdConverter(
    const UsdAbc_AlembicType& alembicType,
    const SdfValueTypeName &usdType) const
{
    BOOST_FOREACH(const _ConverterData &c, _typeConverters) {
        if (c.usdType == usdType and c.abcType == alembicType) {
            return c.toUsdFn;
        }
    }
    static const ToUsdConverter empty;
    return empty;
}

UsdAbc_AlembicType
UsdAbc_AlembicDataConversion::FindConverter(
    const SdfValueTypeName& usdType) const
{
    BOOST_FOREACH(const _ConverterData &c, _typeConverters) {
        if (c.usdType == usdType) {
            return c.abcType;
        }
    }
    return UsdAbc_AlembicType();
}

const UsdAbc_AlembicDataConversion::FromUsdConverter&
UsdAbc_AlembicDataConversion::GetConverter(
    const SdfValueTypeName& usdType) const
{
    BOOST_FOREACH(const _ConverterData &c, _typeConverters) {
        if (c.usdType == usdType) {
            return c.fromUsdFn;
        }
    }

    static const FromUsdConverter empty;
    return empty;
}

void
UsdAbc_AlembicDataConversion::_AddConverter(
    const UsdAbc_AlembicType& alembicType,
    const SdfValueTypeName& usdType,
    const ToUsdConverter& usdConverter,
    const FromUsdConverter& abcConverter)
{
    _typeConverters.push_back(
        _ConverterData(usdType, alembicType, usdConverter, abcConverter)
    );
}

UsdAbc_AlembicConversions::UsdAbc_AlembicConversions()
{
    // Preferred conversions.
    data.AddConverter<bool,          bool_t>();
    data.AddConverter<unsigned char, abc::uint8_t>();
    data.AddConverter<int,           abc::int32_t>();
    data.AddConverter<unsigned int,  abc::uint32_t>();
    data.AddConverter<long,          abc::int64_t>();
    data.AddConverter<unsigned long, abc::uint64_t>();
    data.AddConverter<half,          half>();
    data.AddConverter<float,         float32_t>();
    data.AddConverter<double,        float64_t>();
    data.AddConverter<std::string,   std::string>();
    data.AddConverter<GfVec2i,		abc::int32_t, 2>();
    data.AddConverter<GfVec2h,      half, 2>();
    data.AddConverter<GfVec2f, float32_t, 2>();
    data.AddConverter<GfVec2d, float64_t, 2>();
    data.AddConverter<GfVec3i,   abc::int32_t, 3>();
    data.AddConverter<GfVec3h,      half, 3>();
    data.AddConverter<GfVec3f, float32_t, 3>();
    data.AddConverter<GfVec3d, float64_t, 3>();
    data.AddConverter<GfVec4i,   abc::int32_t, 4>();
    data.AddConverter<GfVec4h,      half, 4>();
    data.AddConverter<GfVec4f, float32_t, 4>();
    data.AddConverter<GfVec4d, float64_t, 4>();
    data.AddConverter<GfQuatf, float32_t, 4>();
    data.AddConverter<GfQuatd, float64_t, 4>();
    data.AddConverter<GfMatrix4d, float64_t, 16>();

    // Other conversions.
    data.AddConverter<int,          abc::int8_t>();
    data.AddConverter<int,          abc::int16_t>();
    data.AddConverter<unsigned int, abc::uint16_t>();
    data.AddConverter<TfToken,      std::string>();
    data.AddConverter<GfMatrix4d,   float32_t, 16>();

    // Role conversions.
    data.AddConverter<GfVec3h,      half, 3>(SdfValueTypeNames->Point3h);
    data.AddConverter<GfVec3f, float32_t, 3>(SdfValueTypeNames->Point3f);
    data.AddConverter<GfVec3d, float64_t, 3>(SdfValueTypeNames->Point3d);
    data.AddConverter<GfVec3h,      half, 3>(SdfValueTypeNames->Normal3h);
    data.AddConverter<GfVec3f, float32_t, 3>(SdfValueTypeNames->Normal3f);
    data.AddConverter<GfVec3d, float64_t, 3>(SdfValueTypeNames->Normal3d);
    data.AddConverter<GfVec3h,      half, 3>(SdfValueTypeNames->Vector3h);
    data.AddConverter<GfVec3f, float32_t, 3>(SdfValueTypeNames->Vector3f);
    data.AddConverter<GfVec3d, float64_t, 3>(SdfValueTypeNames->Vector3d);
    data.AddConverter<GfVec3h,      half, 3>(SdfValueTypeNames->Color3h);
    data.AddConverter<GfVec3f, float32_t, 3>(SdfValueTypeNames->Color3f);
    data.AddConverter<GfVec3d, float64_t, 3>(SdfValueTypeNames->Color3d);
    data.AddConverter<GfMatrix4d, float64_t, 16>(SdfValueTypeNames->Frame4d);
}

} // namespace UsdAbc_AlembicUtil
