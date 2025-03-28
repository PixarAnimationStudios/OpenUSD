//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/matfiltSceneIndexPlugins.h"
#include "hdPrman/material.h"
#include "hdPrman/matfiltConvertPreviewMaterial.h"
#include "hdPrman/tokens.h"

#ifdef PXR_MATERIALX_SUPPORT_ENABLED
#include "hdPrman/matfiltMaterialX.h"
#endif

#include "hdPrman/virtualStructResolvingSceneIndex.h"

#include "pxr/base/tf/stringUtils.h"

#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/materialFilteringSceneIndexBase.h"
#include "pxr/imaging/hd/materialNetworkInterface.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#if HD_API_VERSION >= 76
#include "pxr/imaging/hdsi/nodeIdentifierResolvingSceneIndex.h"
#endif

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (applyConditionals)
    (OSL)
    ((previewMatPluginName, "HdPrman_PreviewMaterialFilteringSceneIndexPlugin"))
    ((materialXPluginName,  "HdPrman_MaterialXFilteringSceneIndexPlugin"))
    ((vstructPluginName,    "HdPrman_VirtualStructResolvingSceneIndexPlugin"))
    ((nodeIdPluginName, "HdPrman_NodeIdentifierResolvingSceneIndexPlugin"))
);

/// Ordering of the matfilt operations. This is necessary when using scene
/// index plugins instead of a filter chain which is populated in the required
/// order.
enum _MatfiltOrder
{
    Start = 0,
    ConnectionResolve = 100, // vstruct
    NodeTranslation = 110, // matx, preview surface,
    NodeIdResolution = 120, // nodeId's for sourceAsset shaders
    End = 200,
};

////////////////////////////////////////////////////////////////////////////////
// Plugin registrations
////////////////////////////////////////////////////////////////////////////////

// XXX: Hardcoded for now to match the legacy matfilt logic.
static const bool _resolveVstructsWithConditionals = true;

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<
        HdPrman_PreviewMaterialFilteringSceneIndexPlugin>();
    
    HdSceneIndexPluginRegistry::Define<
        HdPrman_MaterialXFilteringSceneIndexPlugin>();
    
    HdSceneIndexPluginRegistry::Define<
        HdPrman_VirtualStructResolvingSceneIndexPlugin>();

#if HD_API_VERSION >= 76
    HdSceneIndexPluginRegistry::Define<
        HdPrman_NodeIdentifierResolvingSceneIndexPlugin>();
#endif
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    for( auto const& rendererDisplayName : HdPrman_GetPluginDisplayNames() ) {
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            rendererDisplayName,
            _tokens->previewMatPluginName,
            nullptr, // no argument data necessary
            _MatfiltOrder::NodeTranslation,
            HdSceneIndexPluginRegistry::InsertionOrderAtStart);

        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            rendererDisplayName,
            _tokens->materialXPluginName,
            nullptr, // no argument data necessary
            _MatfiltOrder::NodeTranslation,
            HdSceneIndexPluginRegistry::InsertionOrderAtStart);

        HdContainerDataSourceHandle const inputArgs =
            HdRetainedContainerDataSource::New(
                _tokens->applyConditionals,
                HdRetainedTypedSampledDataSource<bool>::New(
                    _resolveVstructsWithConditionals));

        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            rendererDisplayName,
            _tokens->vstructPluginName,
            inputArgs,
            _MatfiltOrder::ConnectionResolve,
            HdSceneIndexPluginRegistry::InsertionOrderAtStart);

        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            rendererDisplayName,
            _tokens->nodeIdPluginName,
            inputArgs,
            _MatfiltOrder::NodeIdResolution,
            HdSceneIndexPluginRegistry::InsertionOrderAtStart);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Scene Index Implementations
////////////////////////////////////////////////////////////////////////////////

namespace
{

void
_TransformPreviewMaterialNetwork(
    HdMaterialNetworkInterface *networkInterface)
{
    std::vector<std::string> errors;
    MatfiltConvertPreviewMaterial(networkInterface, &errors);
    if (!errors.empty()) {
        TF_RUNTIME_ERROR(
            "Error filtering preview material network for prim %s: %s\n",
                networkInterface->GetMaterialPrimPath().GetText(),
                TfStringJoin(errors).c_str());
    }
}

TF_DECLARE_REF_PTRS(_PreviewMaterialFilteringSceneIndex);

class _PreviewMaterialFilteringSceneIndex :
    public HdMaterialFilteringSceneIndexBase
{
public:

    static _PreviewMaterialFilteringSceneIndexRefPtr New(
        const HdSceneIndexBaseRefPtr &inputScene) 
    {
        return TfCreateRefPtr(
            new _PreviewMaterialFilteringSceneIndex(inputScene));
    }

protected:
    _PreviewMaterialFilteringSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex)
    : HdMaterialFilteringSceneIndexBase(inputSceneIndex)
    {
    }

    FilteringFnc _GetFilteringFunction() const override
    {
        return _TransformPreviewMaterialNetwork;
    }
};

/// ----------------------------------------------------------------------------

#ifdef PXR_MATERIALX_SUPPORT_ENABLED

void
_TransformMaterialXNetwork(
    HdMaterialNetworkInterface *networkInterface)
{
    std::vector<std::string> errors;
    MatfiltMaterialX(networkInterface, &errors);
    if (!errors.empty()) {
        TF_RUNTIME_ERROR(
            "Error filtering preview material network for prim %s: %s\n",
                networkInterface->GetMaterialPrimPath().GetText(),
                TfStringJoin(errors).c_str());
    }
}

TF_DECLARE_REF_PTRS(_MaterialXFilteringSceneIndex);

class _MaterialXFilteringSceneIndex :
    public HdMaterialFilteringSceneIndexBase
{
public:

    static _MaterialXFilteringSceneIndexRefPtr New(
        const HdSceneIndexBaseRefPtr &inputScene) 
    {
        return TfCreateRefPtr(
            new _MaterialXFilteringSceneIndex(inputScene));
    }

protected:
    _MaterialXFilteringSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex)
    : HdMaterialFilteringSceneIndexBase(inputSceneIndex)
    {
    }

    FilteringFnc _GetFilteringFunction() const override
    {
        return _TransformMaterialXNetwork;
    }
};

#endif

/// ----------------------------------------------------------------------------

// Note: HdPrman_VirtualStructResolvingSceneIndex is defined in its own
// translation unit for unit testing purposes.
// 

} // anonymous namespace

////////////////////////////////////////////////////////////////////////////////
// Scene Index Plugin Implementations
////////////////////////////////////////////////////////////////////////////////

HdPrman_PreviewMaterialFilteringSceneIndexPlugin::
HdPrman_PreviewMaterialFilteringSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_PreviewMaterialFilteringSceneIndexPlugin::_AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs)
{
    TF_UNUSED(inputArgs);
    return _PreviewMaterialFilteringSceneIndex::New(inputScene);
}

/// ----------------------------------------------------------------------------

HdPrman_MaterialXFilteringSceneIndexPlugin::
HdPrman_MaterialXFilteringSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_MaterialXFilteringSceneIndexPlugin::_AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs)
{
    TF_UNUSED(inputArgs);
#if PXR_MATERIALX_SUPPORT_ENABLED
    return _MaterialXFilteringSceneIndex::New(inputScene);
#else
    return inputScene;
#endif
}

/// ----------------------------------------------------------------------------

HdPrman_VirtualStructResolvingSceneIndexPlugin::
HdPrman_VirtualStructResolvingSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_VirtualStructResolvingSceneIndexPlugin::_AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs)
{
    bool applyConditionals = false;
    if (HdBoolDataSourceHandle val = HdBoolDataSource::Cast(
            inputArgs->Get(_tokens->applyConditionals))) {
        applyConditionals = val->GetTypedValue(0.0f);
    } else {
        TF_CODING_ERROR("Missing argument to plugin %s",
                        _tokens->vstructPluginName.GetText());
    }
    return HdPrman_VirtualStructResolvingSceneIndex::New(
                inputScene, applyConditionals);
}

/// ----------------------------------------------------------------------------

#if HD_API_VERSION >= 76
HdPrman_NodeIdentifierResolvingSceneIndexPlugin::
HdPrman_NodeIdentifierResolvingSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_NodeIdentifierResolvingSceneIndexPlugin::_AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs)
{
    TF_UNUSED(inputArgs);
    return HdSiNodeIdentifierResolvingSceneIndex::New(
                inputScene, /* sourceType */_tokens->OSL);
}
#endif

PXR_NAMESPACE_CLOSE_SCOPE
