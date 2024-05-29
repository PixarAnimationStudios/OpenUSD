//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include <boost/python/class.hpp>
#include "pxr/usd/usdVol/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Helper to return a static token as a string.  We wrap tokens as Python
// strings and for some reason simply wrapping the token using def_readonly
// bypasses to-Python conversion, leading to the error that there's no
// Python type for the C++ TfToken type.  So we wrap this functor instead.
class _WrapStaticToken {
public:
    _WrapStaticToken(const TfToken* token) : _token(token) { }

    std::string operator()() const
    {
        return _token->GetString();
    }

private:
    const TfToken* _token;
};

template <typename T>
void
_AddToken(T& cls, const char* name, const TfToken& token)
{
    cls.add_static_property(name,
                            boost::python::make_function(
                                _WrapStaticToken(&token),
                                boost::python::return_value_policy<
                                    boost::python::return_by_value>(),
                                boost::mpl::vector1<std::string>()));
}

} // anonymous

void wrapUsdVolTokens()
{
    boost::python::class_<UsdVolTokensType, boost::noncopyable>
        cls("Tokens", boost::python::no_init);
    _AddToken(cls, "bool_", UsdVolTokens->bool_);
    _AddToken(cls, "Color", UsdVolTokens->Color);
    _AddToken(cls, "double2", UsdVolTokens->double2);
    _AddToken(cls, "double3", UsdVolTokens->double3);
    _AddToken(cls, "double_", UsdVolTokens->double_);
    _AddToken(cls, "field", UsdVolTokens->field);
    _AddToken(cls, "fieldClass", UsdVolTokens->fieldClass);
    _AddToken(cls, "fieldDataType", UsdVolTokens->fieldDataType);
    _AddToken(cls, "fieldIndex", UsdVolTokens->fieldIndex);
    _AddToken(cls, "fieldName", UsdVolTokens->fieldName);
    _AddToken(cls, "fieldPurpose", UsdVolTokens->fieldPurpose);
    _AddToken(cls, "filePath", UsdVolTokens->filePath);
    _AddToken(cls, "float2", UsdVolTokens->float2);
    _AddToken(cls, "float3", UsdVolTokens->float3);
    _AddToken(cls, "float_", UsdVolTokens->float_);
    _AddToken(cls, "fogVolume", UsdVolTokens->fogVolume);
    _AddToken(cls, "half", UsdVolTokens->half);
    _AddToken(cls, "half2", UsdVolTokens->half2);
    _AddToken(cls, "half3", UsdVolTokens->half3);
    _AddToken(cls, "int2", UsdVolTokens->int2);
    _AddToken(cls, "int3", UsdVolTokens->int3);
    _AddToken(cls, "int64", UsdVolTokens->int64);
    _AddToken(cls, "int_", UsdVolTokens->int_);
    _AddToken(cls, "levelSet", UsdVolTokens->levelSet);
    _AddToken(cls, "mask", UsdVolTokens->mask);
    _AddToken(cls, "matrix3d", UsdVolTokens->matrix3d);
    _AddToken(cls, "matrix4d", UsdVolTokens->matrix4d);
    _AddToken(cls, "None_", UsdVolTokens->None_);
    _AddToken(cls, "Normal", UsdVolTokens->Normal);
    _AddToken(cls, "Point", UsdVolTokens->Point);
    _AddToken(cls, "quatd", UsdVolTokens->quatd);
    _AddToken(cls, "staggered", UsdVolTokens->staggered);
    _AddToken(cls, "string", UsdVolTokens->string);
    _AddToken(cls, "uint", UsdVolTokens->uint);
    _AddToken(cls, "unknown", UsdVolTokens->unknown);
    _AddToken(cls, "Vector", UsdVolTokens->Vector);
    _AddToken(cls, "vectorDataRoleHint", UsdVolTokens->vectorDataRoleHint);
    _AddToken(cls, "Field3DAsset", UsdVolTokens->Field3DAsset);
    _AddToken(cls, "FieldAsset", UsdVolTokens->FieldAsset);
    _AddToken(cls, "FieldBase", UsdVolTokens->FieldBase);
    _AddToken(cls, "OpenVDBAsset", UsdVolTokens->OpenVDBAsset);
    _AddToken(cls, "Volume", UsdVolTokens->Volume);
}
