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
#include "pxr/imaging/hd/sceneIndexUtil.h"
#if HD_API_VERSION >= 76
#include "pxr/imaging/hdsi/nodeIdentifierResolvingSceneIndex.h"
#endif

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (OSL)
    ((pluginName, "HdPrman_MatFiltSceneIndexPlugin"))
);

////////////////////////////////////////////////////////////////////////////////
// Plugin registration
////////////////////////////////////////////////////////////////////////////////

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<HdPrman_MatFiltSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    for (auto const& rendererDisplayName : HdPrman_GetPluginDisplayNames()) {
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            rendererDisplayName,
            _tokens->pluginName,
            nullptr, // no argument data necessary
            HdPrman_MatFiltSceneIndexPlugin::GetInsertionPhase(),
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
// Scene Index Plugin Implementation
////////////////////////////////////////////////////////////////////////////////

HdPrman_MatFiltSceneIndexPlugin::HdPrman_MatFiltSceneIndexPlugin() = default;

/* static */
HdSceneIndexPluginRegistry::InsertionPhase
HdPrman_MatFiltSceneIndexPlugin::GetInsertionPhase()
{
    // Arbitrary choice that was previously hardcoded in an enum when each of
    // the matfilt ops were their own scene index plugin (100-120).
    return 100;
}

HdSceneIndexBaseRefPtr
HdPrman_MatFiltSceneIndexPlugin::_AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs)
{
    TF_UNUSED(inputArgs);

    // Chain the mat filt ops in the following order:
    // 1. Connection Resolve (vstruct)
    // 2. Node Translation (matx, preview surface)
    // 3. Node ID Resolution

    HdSceneIndexBaseRefPtr si = inputScene;
     // XXX: Hardcoded for now to match the legacy matfilt logic.
    static const bool _resolveVstructsWithConditionals = true;
    si = HdPrman_VirtualStructResolvingSceneIndex::New(
        si, _resolveVstructsWithConditionals);
    
    si = _PreviewMaterialFilteringSceneIndex::New(si);
#ifdef PXR_MATERIALX_SUPPORT_ENABLED
    si = _MaterialXFilteringSceneIndex::New(si);
#endif
#if HD_API_VERSION >= 76
    si = HdSiNodeIdentifierResolvingSceneIndex::New(
        si, /* sourceType */_tokens->OSL);
#endif

    if (TfGetEnvSetting<bool>(HD_USE_ENCAPSULATING_SCENE_INDICES)) {
        si = HdMakeEncapsulatingSceneIndex({inputScene}, si);
        si->SetDisplayName("HdPrman MatFilt Scene Indices");
    }

    return si;
}


PXR_NAMESPACE_CLOSE_SCOPE
