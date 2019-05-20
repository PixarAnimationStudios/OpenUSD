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
#include "pxr/usd/usd/tokens.h"

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

void wrapUsdTokens()
{
    boost::python::class_<UsdTokensType, boost::noncopyable>
        cls("Tokens", boost::python::no_init);
    _AddToken(cls, "apiSchemas", UsdTokens->apiSchemas);
    _AddToken(cls, "apiSchemaType", UsdTokens->apiSchemaType);
    _AddToken(cls, "clipActive", UsdTokens->clipActive);
    _AddToken(cls, "clipAssetPaths", UsdTokens->clipAssetPaths);
    _AddToken(cls, "clipManifestAssetPath", UsdTokens->clipManifestAssetPath);
    _AddToken(cls, "clipPrimPath", UsdTokens->clipPrimPath);
    _AddToken(cls, "clips", UsdTokens->clips);
    _AddToken(cls, "clipSets", UsdTokens->clipSets);
    _AddToken(cls, "clipTemplateAssetPath", UsdTokens->clipTemplateAssetPath);
    _AddToken(cls, "clipTemplateEndTime", UsdTokens->clipTemplateEndTime);
    _AddToken(cls, "clipTemplateStartTime", UsdTokens->clipTemplateStartTime);
    _AddToken(cls, "clipTemplateStride", UsdTokens->clipTemplateStride);
    _AddToken(cls, "clipTimes", UsdTokens->clipTimes);
    _AddToken(cls, "collection", UsdTokens->collection);
    _AddToken(cls, "exclude", UsdTokens->exclude);
    _AddToken(cls, "excludes", UsdTokens->excludes);
    _AddToken(cls, "expandPrims", UsdTokens->expandPrims);
    _AddToken(cls, "expandPrimsAndProperties", UsdTokens->expandPrimsAndProperties);
    _AddToken(cls, "expansionRule", UsdTokens->expansionRule);
    _AddToken(cls, "explicitOnly", UsdTokens->explicitOnly);
    _AddToken(cls, "includeRoot", UsdTokens->includeRoot);
    _AddToken(cls, "includes", UsdTokens->includes);
    _AddToken(cls, "isPrivateApply", UsdTokens->isPrivateApply);
    _AddToken(cls, "multipleApply", UsdTokens->multipleApply);
    _AddToken(cls, "nonApplied", UsdTokens->nonApplied);
    _AddToken(cls, "propertyNamespacePrefix", UsdTokens->propertyNamespacePrefix);
    _AddToken(cls, "singleApply", UsdTokens->singleApply);
}
