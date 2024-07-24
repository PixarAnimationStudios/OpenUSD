//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
    _AddToken(cls, "coordSys", UsdShadeTokens->coordSys);
    _AddToken(cls, "coordSys_MultipleApplyTemplate_Binding", UsdShadeTokens->coordSys_MultipleApplyTemplate_Binding);
    _AddToken(cls, "displacement", UsdShadeTokens->displacement);
    _AddToken(cls, "fallbackStrength", UsdShadeTokens->fallbackStrength);
    _AddToken(cls, "full", UsdShadeTokens->full);
    _AddToken(cls, "id", UsdShadeTokens->id);
    _AddToken(cls, "infoId", UsdShadeTokens->infoId);
    _AddToken(cls, "infoImplementationSource", UsdShadeTokens->infoImplementationSource);
    _AddToken(cls, "inputs", UsdShadeTokens->inputs);
    _AddToken(cls, "interfaceOnly", UsdShadeTokens->interfaceOnly);
    _AddToken(cls, "materialBind", UsdShadeTokens->materialBind);
    _AddToken(cls, "materialBinding", UsdShadeTokens->materialBinding);
    _AddToken(cls, "materialBindingCollection", UsdShadeTokens->materialBindingCollection);
    _AddToken(cls, "materialVariant", UsdShadeTokens->materialVariant);
    _AddToken(cls, "outputs", UsdShadeTokens->outputs);
    _AddToken(cls, "outputsDisplacement", UsdShadeTokens->outputsDisplacement);
    _AddToken(cls, "outputsSurface", UsdShadeTokens->outputsSurface);
    _AddToken(cls, "outputsVolume", UsdShadeTokens->outputsVolume);
    _AddToken(cls, "preview", UsdShadeTokens->preview);
    _AddToken(cls, "sdrMetadata", UsdShadeTokens->sdrMetadata);
    _AddToken(cls, "sourceAsset", UsdShadeTokens->sourceAsset);
    _AddToken(cls, "sourceCode", UsdShadeTokens->sourceCode);
    _AddToken(cls, "strongerThanDescendants", UsdShadeTokens->strongerThanDescendants);
    _AddToken(cls, "subIdentifier", UsdShadeTokens->subIdentifier);
    _AddToken(cls, "surface", UsdShadeTokens->surface);
    _AddToken(cls, "universalRenderContext", UsdShadeTokens->universalRenderContext);
    _AddToken(cls, "universalSourceType", UsdShadeTokens->universalSourceType);
    _AddToken(cls, "volume", UsdShadeTokens->volume);
    _AddToken(cls, "weakerThanDescendants", UsdShadeTokens->weakerThanDescendants);
    _AddToken(cls, "ConnectableAPI", UsdShadeTokens->ConnectableAPI);
    _AddToken(cls, "CoordSysAPI", UsdShadeTokens->CoordSysAPI);
    _AddToken(cls, "Material", UsdShadeTokens->Material);
    _AddToken(cls, "MaterialBindingAPI", UsdShadeTokens->MaterialBindingAPI);
    _AddToken(cls, "NodeDefAPI", UsdShadeTokens->NodeDefAPI);
    _AddToken(cls, "NodeGraph", UsdShadeTokens->NodeGraph);
    _AddToken(cls, "Shader", UsdShadeTokens->Shader);
}
