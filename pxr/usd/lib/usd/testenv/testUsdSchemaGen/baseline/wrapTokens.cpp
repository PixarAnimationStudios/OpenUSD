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
#include "pxr/usd/usdContrived/tokens.h"

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

void wrapUsdContrivedTokens()
{
    boost::python::class_<UsdContrivedTokensType, boost::noncopyable>
        cls("Tokens", boost::python::no_init);
    _AddToken(cls, "asset", UsdContrivedTokens->asset);
    _AddToken(cls, "assetArray", UsdContrivedTokens->assetArray);
    _AddToken(cls, "binding", UsdContrivedTokens->binding);
    _AddToken(cls, "bool", UsdContrivedTokens->bool);
    _AddToken(cls, "boolArray", UsdContrivedTokens->boolArray);
    _AddToken(cls, "color3d", UsdContrivedTokens->color3d);
    _AddToken(cls, "color3dArray", UsdContrivedTokens->color3dArray);
    _AddToken(cls, "color3f", UsdContrivedTokens->color3f);
    _AddToken(cls, "color3fArray", UsdContrivedTokens->color3fArray);
    _AddToken(cls, "color3h", UsdContrivedTokens->color3h);
    _AddToken(cls, "color3hArray", UsdContrivedTokens->color3hArray);
    _AddToken(cls, "color4d", UsdContrivedTokens->color4d);
    _AddToken(cls, "color4dArray", UsdContrivedTokens->color4dArray);
    _AddToken(cls, "color4f", UsdContrivedTokens->color4f);
    _AddToken(cls, "color4fArray", UsdContrivedTokens->color4fArray);
    _AddToken(cls, "color4h", UsdContrivedTokens->color4h);
    _AddToken(cls, "color4hArray", UsdContrivedTokens->color4hArray);
    _AddToken(cls, "cornerIndices", UsdContrivedTokens->cornerIndices);
    _AddToken(cls, "cornerSharpnesses", UsdContrivedTokens->cornerSharpnesses);
    _AddToken(cls, "creaseLengths", UsdContrivedTokens->creaseLengths);
    _AddToken(cls, "double2", UsdContrivedTokens->double2);
    _AddToken(cls, "double2Array", UsdContrivedTokens->double2Array);
    _AddToken(cls, "double3", UsdContrivedTokens->double3);
    _AddToken(cls, "double3Array", UsdContrivedTokens->double3Array);
    _AddToken(cls, "double4", UsdContrivedTokens->double4);
    _AddToken(cls, "double4Array", UsdContrivedTokens->double4Array);
    _AddToken(cls, "double_", UsdContrivedTokens->double_);
    _AddToken(cls, "doubleArray", UsdContrivedTokens->doubleArray);
    _AddToken(cls, "float2", UsdContrivedTokens->float2);
    _AddToken(cls, "float2Array", UsdContrivedTokens->float2Array);
    _AddToken(cls, "float3", UsdContrivedTokens->float3);
    _AddToken(cls, "float3Array", UsdContrivedTokens->float3Array);
    _AddToken(cls, "float4", UsdContrivedTokens->float4);
    _AddToken(cls, "float4Array", UsdContrivedTokens->float4Array);
    _AddToken(cls, "float_", UsdContrivedTokens->float_);
    _AddToken(cls, "floatArray", UsdContrivedTokens->floatArray);
    _AddToken(cls, "frame4d", UsdContrivedTokens->frame4d);
    _AddToken(cls, "frame4dArray", UsdContrivedTokens->frame4dArray);
    _AddToken(cls, "half", UsdContrivedTokens->half);
    _AddToken(cls, "half2", UsdContrivedTokens->half2);
    _AddToken(cls, "half2Array", UsdContrivedTokens->half2Array);
    _AddToken(cls, "half3", UsdContrivedTokens->half3);
    _AddToken(cls, "half3Array", UsdContrivedTokens->half3Array);
    _AddToken(cls, "half4", UsdContrivedTokens->half4);
    _AddToken(cls, "half4Array", UsdContrivedTokens->half4Array);
    _AddToken(cls, "halfArray", UsdContrivedTokens->halfArray);
    _AddToken(cls, "holeIndices", UsdContrivedTokens->holeIndices);
    _AddToken(cls, "int2", UsdContrivedTokens->int2);
    _AddToken(cls, "int2Array", UsdContrivedTokens->int2Array);
    _AddToken(cls, "int3", UsdContrivedTokens->int3);
    _AddToken(cls, "int3Array", UsdContrivedTokens->int3Array);
    _AddToken(cls, "int4", UsdContrivedTokens->int4);
    _AddToken(cls, "int4Array", UsdContrivedTokens->int4Array);
    _AddToken(cls, "int64", UsdContrivedTokens->int64);
    _AddToken(cls, "int64Array", UsdContrivedTokens->int64Array);
    _AddToken(cls, "int_", UsdContrivedTokens->int_);
    _AddToken(cls, "intArray", UsdContrivedTokens->intArray);
    _AddToken(cls, "justDefault", UsdContrivedTokens->justDefault);
    _AddToken(cls, "libraryToken1", UsdContrivedTokens->libraryToken1);
    _AddToken(cls, "libraryToken2", UsdContrivedTokens->libraryToken2);
    _AddToken(cls, "matrix2d", UsdContrivedTokens->matrix2d);
    _AddToken(cls, "matrix2dArray", UsdContrivedTokens->matrix2dArray);
    _AddToken(cls, "matrix3d", UsdContrivedTokens->matrix3d);
    _AddToken(cls, "matrix3dArray", UsdContrivedTokens->matrix3dArray);
    _AddToken(cls, "matrix4d", UsdContrivedTokens->matrix4d);
    _AddToken(cls, "matrix4dArray", UsdContrivedTokens->matrix4dArray);
    _AddToken(cls, "myDouble", UsdContrivedTokens->myDouble);
    _AddToken(cls, "myUniformBool", UsdContrivedTokens->myUniformBool);
    _AddToken(cls, "myVaryingToken", UsdContrivedTokens->myVaryingToken);
    _AddToken(cls, "myVecfArray", UsdContrivedTokens->myVecfArray);
    _AddToken(cls, "namespacedProperty", UsdContrivedTokens->namespacedProperty);
    _AddToken(cls, "newToken", UsdContrivedTokens->newToken);
    _AddToken(cls, "normal3d", UsdContrivedTokens->normal3d);
    _AddToken(cls, "normal3dArray", UsdContrivedTokens->normal3dArray);
    _AddToken(cls, "normal3f", UsdContrivedTokens->normal3f);
    _AddToken(cls, "normal3fArray", UsdContrivedTokens->normal3fArray);
    _AddToken(cls, "normal3h", UsdContrivedTokens->normal3h);
    _AddToken(cls, "normal3hArray", UsdContrivedTokens->normal3hArray);
    _AddToken(cls, "pivotPosition", UsdContrivedTokens->pivotPosition);
    _AddToken(cls, "point3d", UsdContrivedTokens->point3d);
    _AddToken(cls, "point3dArray", UsdContrivedTokens->point3dArray);
    _AddToken(cls, "point3f", UsdContrivedTokens->point3f);
    _AddToken(cls, "point3fArray", UsdContrivedTokens->point3fArray);
    _AddToken(cls, "point3h", UsdContrivedTokens->point3h);
    _AddToken(cls, "point3hArray", UsdContrivedTokens->point3hArray);
    _AddToken(cls, "quatd", UsdContrivedTokens->quatd);
    _AddToken(cls, "quatdArray", UsdContrivedTokens->quatdArray);
    _AddToken(cls, "quatf", UsdContrivedTokens->quatf);
    _AddToken(cls, "quatfArray", UsdContrivedTokens->quatfArray);
    _AddToken(cls, "quath", UsdContrivedTokens->quath);
    _AddToken(cls, "quathArray", UsdContrivedTokens->quathArray);
    _AddToken(cls, "relCanShareApiNameWithAttr", UsdContrivedTokens->relCanShareApiNameWithAttr);
    _AddToken(cls, "riStatementsAttributesUserGofur_GeomOnHairdensity", UsdContrivedTokens->riStatementsAttributesUserGofur_GeomOnHairdensity);
    _AddToken(cls, "string", UsdContrivedTokens->string);
    _AddToken(cls, "stringArray", UsdContrivedTokens->stringArray);
    _AddToken(cls, "temp", UsdContrivedTokens->temp);
    _AddToken(cls, "testingAsset", UsdContrivedTokens->testingAsset);
    _AddToken(cls, "token", UsdContrivedTokens->token);
    _AddToken(cls, "tokenArray", UsdContrivedTokens->tokenArray);
    _AddToken(cls, "transform", UsdContrivedTokens->transform);
    _AddToken(cls, "uchar", UsdContrivedTokens->uchar);
    _AddToken(cls, "ucharArray", UsdContrivedTokens->ucharArray);
    _AddToken(cls, "uint", UsdContrivedTokens->uint);
    _AddToken(cls, "uint64", UsdContrivedTokens->uint64);
    _AddToken(cls, "uint64Array", UsdContrivedTokens->uint64Array);
    _AddToken(cls, "uintArray", UsdContrivedTokens->uintArray);
    _AddToken(cls, "variableTokenAllowed1", UsdContrivedTokens->variableTokenAllowed1);
    _AddToken(cls, "variabletokenAllowed2", UsdContrivedTokens->variabletokenAllowed2);
    _AddToken(cls, "variableTokenDefault", UsdContrivedTokens->variableTokenDefault);
    _AddToken(cls, "vector3d", UsdContrivedTokens->vector3d);
    _AddToken(cls, "vector3dArray", UsdContrivedTokens->vector3dArray);
    _AddToken(cls, "vector3f", UsdContrivedTokens->vector3f);
    _AddToken(cls, "vector3fArray", UsdContrivedTokens->vector3fArray);
    _AddToken(cls, "vector3h", UsdContrivedTokens->vector3h);
    _AddToken(cls, "vector3hArray", UsdContrivedTokens->vector3hArray);
}
