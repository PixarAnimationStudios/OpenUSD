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

using namespace foo::bar::baz;

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
    _AddToken(cls, "binding", UsdContrivedTokens->binding);
    _AddToken(cls, "cornerIndices", UsdContrivedTokens->cornerIndices);
    _AddToken(cls, "cornerSharpnesses", UsdContrivedTokens->cornerSharpnesses);
    _AddToken(cls, "creaseLengths", UsdContrivedTokens->creaseLengths);
    _AddToken(cls, "holeIndices", UsdContrivedTokens->holeIndices);
    _AddToken(cls, "justDefault", UsdContrivedTokens->justDefault);
    _AddToken(cls, "libraryToken1", UsdContrivedTokens->libraryToken1);
    _AddToken(cls, "libraryToken2", UsdContrivedTokens->libraryToken2);
    _AddToken(cls, "myColorFloat", UsdContrivedTokens->myColorFloat);
    _AddToken(cls, "myDouble", UsdContrivedTokens->myDouble);
    _AddToken(cls, "myFloat", UsdContrivedTokens->myFloat);
    _AddToken(cls, "myNormals", UsdContrivedTokens->myNormals);
    _AddToken(cls, "myPoints", UsdContrivedTokens->myPoints);
    _AddToken(cls, "myUniformBool", UsdContrivedTokens->myUniformBool);
    _AddToken(cls, "myVaryingToken", UsdContrivedTokens->myVaryingToken);
    _AddToken(cls, "myVecfArray", UsdContrivedTokens->myVecfArray);
    _AddToken(cls, "myVelocities", UsdContrivedTokens->myVelocities);
    _AddToken(cls, "namespacedProperty", UsdContrivedTokens->namespacedProperty);
    _AddToken(cls, "newToken", UsdContrivedTokens->newToken);
    _AddToken(cls, "pivotPosition", UsdContrivedTokens->pivotPosition);
    _AddToken(cls, "relCanShareApiNameWithAttr", UsdContrivedTokens->relCanShareApiNameWithAttr);
    _AddToken(cls, "riStatementsAttributesUserGofur_GeomOnHairdensity", UsdContrivedTokens->riStatementsAttributesUserGofur_GeomOnHairdensity);
    _AddToken(cls, "temp", UsdContrivedTokens->temp);
    _AddToken(cls, "testingAsset", UsdContrivedTokens->testingAsset);
    _AddToken(cls, "transform", UsdContrivedTokens->transform);
    _AddToken(cls, "unsignedChar", UsdContrivedTokens->unsignedChar);
    _AddToken(cls, "unsignedInt", UsdContrivedTokens->unsignedInt);
    _AddToken(cls, "unsignedInt64Array", UsdContrivedTokens->unsignedInt64Array);
    _AddToken(cls, "variableTokenAllowed1", UsdContrivedTokens->variableTokenAllowed1);
    _AddToken(cls, "variabletokenAllowed2", UsdContrivedTokens->variabletokenAllowed2);
    _AddToken(cls, "variableTokenDefault", UsdContrivedTokens->variableTokenDefault);
}
