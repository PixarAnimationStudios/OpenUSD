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

namespace pxrUsdUsdVolWrapTokens {

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
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "bool_", UsdVolTokens->bool_);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "color", UsdVolTokens->color);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "double2", UsdVolTokens->double2);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "double3", UsdVolTokens->double3);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "double_", UsdVolTokens->double_);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "field", UsdVolTokens->field);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "fieldClass", UsdVolTokens->fieldClass);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "fieldDataType", UsdVolTokens->fieldDataType);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "fieldIndex", UsdVolTokens->fieldIndex);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "fieldName", UsdVolTokens->fieldName);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "fieldPurpose", UsdVolTokens->fieldPurpose);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "filePath", UsdVolTokens->filePath);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "float2", UsdVolTokens->float2);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "float3", UsdVolTokens->float3);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "float_", UsdVolTokens->float_);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "fogVolume", UsdVolTokens->fogVolume);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "half", UsdVolTokens->half);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "half2", UsdVolTokens->half2);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "half3", UsdVolTokens->half3);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "int2", UsdVolTokens->int2);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "int3", UsdVolTokens->int3);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "int64", UsdVolTokens->int64);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "int_", UsdVolTokens->int_);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "levelSet", UsdVolTokens->levelSet);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "mask", UsdVolTokens->mask);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "matrix3d", UsdVolTokens->matrix3d);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "matrix4d", UsdVolTokens->matrix4d);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "none", UsdVolTokens->none);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "normal", UsdVolTokens->normal);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "point", UsdVolTokens->point);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "quatd", UsdVolTokens->quatd);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "staggered", UsdVolTokens->staggered);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "string", UsdVolTokens->string);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "uint", UsdVolTokens->uint);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "unknown", UsdVolTokens->unknown);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "vector", UsdVolTokens->vector);
    pxrUsdUsdVolWrapTokens::_AddToken(cls, "vectorDataRoleHint", UsdVolTokens->vectorDataRoleHint);
}
