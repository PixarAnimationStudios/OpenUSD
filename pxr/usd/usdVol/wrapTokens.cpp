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
    _AddToken(cls, "color", UsdVolTokens->color);
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
    _AddToken(cls, "none", UsdVolTokens->none);
    _AddToken(cls, "normal", UsdVolTokens->normal);
    _AddToken(cls, "point", UsdVolTokens->point);
    _AddToken(cls, "quatd", UsdVolTokens->quatd);
    _AddToken(cls, "staggered", UsdVolTokens->staggered);
    _AddToken(cls, "string", UsdVolTokens->string);
    _AddToken(cls, "uint", UsdVolTokens->uint);
    _AddToken(cls, "unknown", UsdVolTokens->unknown);
    _AddToken(cls, "vector", UsdVolTokens->vector);
    _AddToken(cls, "vectorDataRoleHint", UsdVolTokens->vectorDataRoleHint);
}
