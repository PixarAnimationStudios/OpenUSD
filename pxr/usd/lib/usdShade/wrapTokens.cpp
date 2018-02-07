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
#include "pxr/usd/usdShade/tokens.h"

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

void wrapUsdShadeTokens()
{
    boost::python::class_<UsdShadeTokensType, boost::noncopyable>
        cls("Tokens", boost::python::no_init);
    _AddToken(cls, "allPurpose", UsdShadeTokens->allPurpose);
    _AddToken(cls, "bindMaterialAs", UsdShadeTokens->bindMaterialAs);
    _AddToken(cls, "connectedSourceFor", UsdShadeTokens->connectedSourceFor);
    _AddToken(cls, "derivesFrom", UsdShadeTokens->derivesFrom);
    _AddToken(cls, "displacement", UsdShadeTokens->displacement);
    _AddToken(cls, "fallbackStrength", UsdShadeTokens->fallbackStrength);
    _AddToken(cls, "full", UsdShadeTokens->full);
    _AddToken(cls, "infoId", UsdShadeTokens->infoId);
    _AddToken(cls, "inputs", UsdShadeTokens->inputs);
    _AddToken(cls, "interface_", UsdShadeTokens->interface_);
    _AddToken(cls, "interfaceOnly", UsdShadeTokens->interfaceOnly);
    _AddToken(cls, "interfaceRecipientsOf", UsdShadeTokens->interfaceRecipientsOf);
    _AddToken(cls, "lookBinding", UsdShadeTokens->lookBinding);
    _AddToken(cls, "materialBind", UsdShadeTokens->materialBind);
    _AddToken(cls, "materialBinding", UsdShadeTokens->materialBinding);
    _AddToken(cls, "materialBindingCollection", UsdShadeTokens->materialBindingCollection);
    _AddToken(cls, "materialVariant", UsdShadeTokens->materialVariant);
    _AddToken(cls, "outputs", UsdShadeTokens->outputs);
    _AddToken(cls, "preview", UsdShadeTokens->preview);
    _AddToken(cls, "strongerThanDescendants", UsdShadeTokens->strongerThanDescendants);
    _AddToken(cls, "surface", UsdShadeTokens->surface);
    _AddToken(cls, "weakerThanDescendants", UsdShadeTokens->weakerThanDescendants);
}
