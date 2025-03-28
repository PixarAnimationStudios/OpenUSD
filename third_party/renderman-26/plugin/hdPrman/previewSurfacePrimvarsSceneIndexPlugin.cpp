//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/previewSurfacePrimvarsSceneIndexPlugin.h"

// Version gated due to dependence on
// HdPrman_MaterialPrimvarTransferSceneIndexPlugin.
#if PXR_VERSION >= 2302

#include "hdPrman/tokens.h"

#include "pxr/imaging/hd/dataSourceMaterialNetworkInterface.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/tokens.h"


PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdPrman_PreviewSurfacePrimvarsSceneIndexPlugin"))
    (UsdPreviewSurface)
    ((displacementBoundSphere, "displacementbound:sphere"))
);

////////////////////////////////////////////////////////////////////////////////
// Plugin registrations
////////////////////////////////////////////////////////////////////////////////

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<
        HdPrman_PreviewSurfacePrimvarsSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    // Want this to happen *before* the
    // HdPrman_MaterialPrimvarTransferSceneIndexPlugin, allowing the material 
    // primvar added in this scene index to get transferred.
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 2;

    for (auto const& pluginDisplayName : HdPrman_GetPluginDisplayNames()) {
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            pluginDisplayName,
            _tokens->sceneIndexPluginName,
            nullptr,
            insertionPhase,
            HdSceneIndexPluginRegistry::InsertionOrderAtStart);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Scene Index Implementations
////////////////////////////////////////////////////////////////////////////////

namespace
{

bool
_HasPreviewSurface(
    HdContainerDataSourceHandle materialPrim,
    const SdfPath &materialPrimPath)
{
    // Inspect material network for UsdPreviewSurface node.
    if (HdDataSourceBaseHandle materialDs = materialPrim->Get(
            HdMaterialSchemaTokens->material)) {
        if (HdContainerDataSourceHandle materialContainer =
            HdContainerDataSource::Cast(materialDs)) {
                        
            // Fetch default context with empty token.
            HdDataSourceBaseHandle networkDs =
                materialContainer->Get(TfToken());
                        
            if (HdContainerDataSourceHandle networkContainer =
                HdContainerDataSource::Cast(networkDs)) {
                HdDataSourceMaterialNetworkInterface networkInterface(
                    materialPrimPath, networkContainer, materialPrim);

                const TfTokenVector nodeNames =
                    networkInterface.GetNodeNames();
                for (const TfToken &nodeName : nodeNames) {
                    const TfToken nodeType =
                        networkInterface.GetNodeType(nodeName);
                    if (nodeType == _tokens->UsdPreviewSurface) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

class _MaterialPrimDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_MaterialPrimDataSource);

    _MaterialPrimDataSource(
        const HdContainerDataSourceHandle &inputPrim,
        const SdfPath &primPath)
    : _inputPrim(inputPrim)
    , _primPath(primPath)
    {
    }

    TfTokenVector
    GetNames() override
    {
        TfTokenVector result = _inputPrim->GetNames();

        // Add primvars to names if material prim is using a UsdPreviewSurface 
        // and it's not already there.
        if ((std::find(result.begin(), result.end(),
            HdPrimvarsSchema::GetSchemaToken()) == result.end()) &&
            _HasPreviewSurface(_inputPrim, _primPath)) {
            result.push_back(HdPrimvarsSchema::GetSchemaToken());
        }
        return result;
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        HdDataSourceBaseHandle result = _inputPrim->Get(name);

        // If material is using a UsdPreviewSurface, add one more primvar.
        //
        // We wish to always set this primvar on gprims using 
        // UsdPreviewSurface, regardless of the material's displacement 
        // value. The primvar should have no effect if there is no 
        // displacement on the material. By adding this primvar to the 
        // material here, we can ensure it gets transferred to any gprims
        // using the material via the HdsiMaterialPrimvarTransferSceneIndex. 
        if (name == HdPrimvarsSchema::GetSchemaToken() &&
            _HasPreviewSurface(_inputPrim, _primPath)) {
            
            static const HdSampledDataSourceHandle valueDs =
                HdRetainedTypedSampledDataSource<float>::New(1.f);
            static const HdTokenDataSourceHandle interpolationDs =
                HdPrimvarSchema::BuildInterpolationDataSource(
                    HdPrimvarSchemaTokens->constant);
            static const HdTokenDataSourceHandle roleDs =
                HdPrimvarSchema::BuildRoleDataSource(TfToken());
            static const HdContainerDataSourceHandle primvarsDs = 
                HdRetainedContainerDataSource::New(
                    _tokens->displacementBoundSphere,
                    HdPrimvarSchema::Builder()
                        .SetPrimvarValue(valueDs)
                        .SetInterpolation(interpolationDs)
                        .SetRole(roleDs)
                        .Build());

            if (result) {
                return HdOverlayContainerDataSource::New(
                    HdContainerDataSource::Cast(result),
                    primvarsDs);
            } else {
                return primvarsDs;
            }
        }
        
        return result;
    }

private:
    HdContainerDataSourceHandle _inputPrim;
    SdfPath _primPath;
};


TF_DECLARE_REF_PTRS(_HdPrmanPreviewSurfacePrimvarsSceneIndex);

class _HdPrmanPreviewSurfacePrimvarsSceneIndex
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    static _HdPrmanPreviewSurfacePrimvarsSceneIndexRefPtr
        New(const HdSceneIndexBaseRefPtr &inputSceneIndex)
    {
        return TfCreateRefPtr(
            new _HdPrmanPreviewSurfacePrimvarsSceneIndex(
                inputSceneIndex));
    }

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override
    {
        HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
        if (prim.primType == HdPrimTypeTokens->material && prim.dataSource) {
            prim.dataSource = _MaterialPrimDataSource::New(
                prim.dataSource, primPath);
        }

        return prim;
    }

    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override
    {
        return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    }

protected:
    _HdPrmanPreviewSurfacePrimvarsSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex)
      : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
    {
    }

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override
    {
        if (!_IsObserved()) {
            return;
        }

        _SendPrimsAdded(entries);
    }

    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override
    {
        if (!_IsObserved()) {
            return;
        }

        _SendPrimsRemoved(entries);
    }

    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override
    {
        if (!_IsObserved()) {
            return;
        }

        _SendPrimsDirtied(entries);
    }
};

} // anonymous namespace

////////////////////////////////////////////////////////////////////////////////
// Scene Index Plugin Implementations
////////////////////////////////////////////////////////////////////////////////

HdPrman_PreviewSurfacePrimvarsSceneIndexPlugin::
HdPrman_PreviewSurfacePrimvarsSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_PreviewSurfacePrimvarsSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    return _HdPrmanPreviewSurfacePrimvarsSceneIndex::New(inputScene);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2302
