//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/fallbackMaterialsSceneIndexPlugin.h"

#if PXR_VERSION >= 2302

#include "hdPrman/material.h"
#include "hdPrman/tokens.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/materialBindingSchema.h"
#include "pxr/imaging/hd/materialBindingsSchema.h"
#include "pxr/imaging/hd/materialConnectionSchema.h"
#include "pxr/imaging/hd/materialNetworkSchema.h"
#include "pxr/imaging/hd/materialNodeParameterSchema.h"
#include "pxr/imaging/hd/materialNodeSchema.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/mergingSceneIndex.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/retainedSceneIndex.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/staticTokens.h"

#include <set>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdPrman_FallbackMaterialsSceneIndexPlugin"))
);

namespace {

const SdfPath &
_GetFallbackSurfaceMaterialPath()
{
    static const SdfPath result("/__HdPrman/FallbackSurfaceMaterial");
    return result;
}

//const SdfPath &
//_GetFallbackVolumeMaterialPath()
//{
//    static const SdfPath result("/__HdPrman/FallbackVolumeMaterial");
//    return result;
//}

// The set of prim types that should receive the fallback surface material.
const std::set<TfToken> &
_GetGeometrySurfacePrimTypes()
{
    static const std::set<TfToken> result = {
        HdPrimTypeTokens->mesh,
        HdPrimTypeTokens->basisCurves,
        HdPrimTypeTokens->points,
        HdPrimTypeTokens->cone,
        HdPrimTypeTokens->cylinder,
        HdPrimTypeTokens->sphere
    };
    return result;
}

// The set of prim types that should receive the fallback volume material.
const std::set<TfToken> &
_GetGeometryVolumePrimTypes()
{
    static const std::set<TfToken> result = {
        HdPrimTypeTokens->volume,
    };
    return result;
}

// Convert an HdMaterialNetwork2 into a HdMaterialNetworkSchema container data
// source. Node names and upstream node paths use the full SdfPath string,
// matching the convention expected by the scene index emulation layer (which
// reconstructs SdfPaths via SdfPath(token.GetString())).
HdContainerDataSourceHandle
_ToMaterialNetworkDataSource(const HdMaterialNetwork2 &network)
{
    // Nodes.
    std::vector<TfToken> nodeNames;
    std::vector<HdDataSourceBaseHandle> nodeValues;
    nodeNames.reserve(network.nodes.size());
    nodeValues.reserve(network.nodes.size());

    for (const auto &pathAndNode : network.nodes) {
        const SdfPath &nodePath = pathAndNode.first;
        const HdMaterialNode2 &node = pathAndNode.second;

        // Parameters.
        std::vector<TfToken> paramNames;
        std::vector<HdDataSourceBaseHandle> paramValues;
        paramNames.reserve(node.parameters.size());
        paramValues.reserve(node.parameters.size());
        for (const auto &nameAndValue : node.parameters) {
            paramNames.push_back(nameAndValue.first);
            paramValues.push_back(
                HdMaterialNodeParameterSchema::Builder()
                    .SetValue(
                        HdRetainedTypedSampledDataSource<VtValue>::New(
                            nameAndValue.second))
                    .Build());
        }

        // Input connections.
        std::vector<TfToken> connNames;
        std::vector<HdDataSourceBaseHandle> connValues;
        connNames.reserve(node.inputConnections.size());
        connValues.reserve(node.inputConnections.size());
        for (const auto &inputNameAndConns : node.inputConnections) {
            const std::vector<HdMaterialConnection2> &conns =
                inputNameAndConns.second;
            std::vector<HdDataSourceBaseHandle> connDs;
            connDs.reserve(conns.size());
            for (const HdMaterialConnection2 &conn : conns) {
                connDs.push_back(
                    HdMaterialConnectionSchema::Builder()
                        .SetUpstreamNodePath(
                            HdRetainedTypedSampledDataSource<TfToken>::New(
                                conn.upstreamNode.GetToken()))
                        .SetUpstreamNodeOutputName(
                            HdRetainedTypedSampledDataSource<TfToken>::New(
                                conn.upstreamOutputName))
                        .Build());
            }
            connNames.push_back(inputNameAndConns.first);
            connValues.push_back(
                HdMaterialConnectionVectorSchema::BuildRetained(
                    connDs.size(), connDs.data()));
        }

        nodeNames.push_back(nodePath.GetToken());
        nodeValues.push_back(
            HdMaterialNodeSchema::Builder()
                .SetNodeIdentifier(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        node.nodeTypeId))
                .SetParameters(
                    HdRetainedContainerDataSource::New(
                        paramNames.size(),
                        paramNames.data(),
                        paramValues.data()))
                .SetInputConnections(
                    HdRetainedContainerDataSource::New(
                        connNames.size(),
                        connNames.data(),
                        connValues.data()))
                .Build());
    }

    // Terminals.
    std::vector<TfToken> terminalNames;
    std::vector<HdDataSourceBaseHandle> terminalValues;
    terminalNames.reserve(network.terminals.size());
    terminalValues.reserve(network.terminals.size());
    for (const auto &nameAndConn : network.terminals) {
        const HdMaterialConnection2 &conn = nameAndConn.second;
        terminalNames.push_back(nameAndConn.first);
        terminalValues.push_back(
            HdMaterialConnectionSchema::Builder()
                .SetUpstreamNodePath(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        conn.upstreamNode.GetToken()))
                .SetUpstreamNodeOutputName(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        conn.upstreamOutputName))
                .Build());
    }

    return
        HdMaterialNetworkSchema::Builder()
            .SetNodes(
                HdRetainedContainerDataSource::New(
                    nodeNames.size(),
                    nodeNames.data(),
                    nodeValues.data()))
            .SetTerminals(
                HdRetainedContainerDataSource::New(
                    terminalNames.size(),
                    terminalNames.data(),
                    terminalValues.data()))
            .Build();
}

static HdMaterialNetwork2
_GetFallbackSurfaceMaterialNetwork()
{
    // We expect this to be called once, at init time, but drop a trace
    // scope in just in case that changes.  Accordingly, we also don't
    // bother creating static tokens for the single-use cases below.
    HD_TRACE_FUNCTION();

    const std::map<SdfPath, HdMaterialNode2> nodes = {
        {
            // path
            SdfPath("Primvar_displayColor"),
            // node info
            HdMaterialNode2 {
                // nodeTypeId
                TfToken("PxrPrimvar"),
                // parameters
                {
                    { TfToken("varname"),
                      VtValue(TfToken("displayColor")) },
                    { TfToken("defaultColor"),
                      VtValue(GfVec3f(0.5, 0.5, 0.5)) },
                    { TfToken("type"),
                      VtValue(TfToken("color")) },
                },
            },
        },
        {
            // path
            SdfPath("Primvar_displayRoughness"),
            // node info
            HdMaterialNode2 {
                // nodeTypeId
                TfToken("PxrPrimvar"),
                // parameters
                {
                    { TfToken("varname"),
                      VtValue(TfToken("displayRoughness")) },
                    { TfToken("defaultFloat"),
                      VtValue(1.0f) },
                    { TfToken("type"),
                      VtValue(TfToken("float")) },
                },
            },
        },
        {
            // path
            SdfPath("Primvar_displayOpacity"),
            // node info
            HdMaterialNode2 {
                // nodeTypeId
                TfToken("PxrPrimvar"),
                // parameters
                {
                    { TfToken("varname"),
                      VtValue(TfToken("displayOpacity")) },
                    { TfToken("defaultFloat"),
                      VtValue(1.0f) },
                    { TfToken("type"),
                      VtValue(TfToken("float")) },
                },
            },
        },
        {
            // path
            SdfPath("Primvar_displayMetallic"),
            // node info
            HdMaterialNode2 {
                // nodeTypeId
                TfToken("PxrPrimvar"),
                // parameters
                {
                    { TfToken("varname"),
                      VtValue(TfToken("displayMetallic")) },
                    { TfToken("defaultFloat"),
                      VtValue(0.0f) },
                    { TfToken("type"),
                      VtValue(TfToken("float")) },
                },
            },
        },

        // UsdPreviewSurfaceParameters
        {
            // path
            SdfPath("UsdPreviewSurfaceParameters"),
            // node info
            HdMaterialNode2 {
                // nodeTypeId
                TfToken("UsdPreviewSurfaceParameters"),
                // parameters
                {},
                // connections
                {
                    { TfToken("diffuseColor"),
                      { { SdfPath("Primvar_displayColor"),
                            TfToken("resultRGB") } } },
                    { TfToken("roughness"),
                      { { SdfPath("Primvar_displayRoughness"),
                          TfToken("resultF") } } },
                    { TfToken("metallic"),
                      { { SdfPath("Primvar_displayMetallic"),
                          TfToken("resultF") } } },
                    { TfToken("opacity"),
                      { { SdfPath("Primvar_displayOpacity"),
                          TfToken("resultF") } } },
                },
            },
        },
        // PxrSurface (connected to UsdPreviewSurfaceParameters)
        {
            // path
            SdfPath("PxrSurface"),
            // node info
            HdMaterialNode2 {
                // nodeTypeId
                TfToken("PxrSurface"),
                // parameters
                {
                    { TfToken("specularModelType"),
                      VtValue(int(1)) },
                    { TfToken("diffuseDoubleSided"),
                      VtValue(int(1)) },
                    { TfToken("specularDoubleSided"),
                      VtValue(int(1)) },
                    { TfToken("specularFaceColor"),
                      VtValue(GfVec3f(0.04)) },
                    { TfToken("specularEdgeColor"),
                      VtValue(GfVec3f(1.0)) },
                },
                // connections
                {
                    { TfToken("diffuseColor"),
                      {{ SdfPath("UsdPreviewSurfaceParameters"),
                         TfToken("diffuseColorOut") }} },
                    { TfToken("diffuseGain"),
                      {{ SdfPath("UsdPreviewSurfaceParameters"),
                         TfToken("diffuseGainOut") }} },
                    { TfToken("specularFaceColor"),
                      {{ SdfPath("UsdPreviewSurfaceParameters"),
                         TfToken("specularFaceColorOut") }} },
                    { TfToken("specularEdgeColor"),
                      {{ SdfPath("UsdPreviewSurfaceParameters"),
                         TfToken("specularEdgeColorOut") }} },
                    { TfToken("specularRoughness"),
                      {{ SdfPath("UsdPreviewSurfaceParameters"),
                         TfToken("specularRoughnessOut") }} },
                    { TfToken("presence"),
                      {{ SdfPath("Primvar_displayOpacity"),
                         TfToken("resultF") }} },
                },
            },
        },
    };

    const std::map<TfToken, HdMaterialConnection2> terminals = {
        { TfToken("surface"),
          HdMaterialConnection2 {
            SdfPath("PxrSurface"),
            TfToken("outputName") }
        },
    };

    const TfTokenVector primvars = {
        TfToken("displayColor"),
        TfToken("displayMetallic"),
        TfToken("displayOpacity"),
        TfToken("displayRoughness"),
    };

    return HdMaterialNetwork2{nodes, terminals, primvars};
}

// The fallback material as a HdMaterialSchema container, keyed by the universal
// render context. Reuses _GetFallbackSurfaceMaterialNetwork() so
// there is a single source of truth for the fallback material definition.
HdContainerDataSourceHandle
_FallbackSurfaceMaterialDataSource()
{
    static const HdMaterialNetwork2 network =
        _GetFallbackSurfaceMaterialNetwork();
    const TfToken renderContext =
        TfToken("ri");
//        HdMaterialSchemaTokens->universalRenderContext;
    HdDataSourceBaseHandle const networkDs =
        _ToMaterialNetworkDataSource(network);
    return HdMaterialSchema::BuildRetained(1, &renderContext, &networkDs);
}

// A retained scene index holding the single fallback material prim.
HdSceneIndexBaseRefPtr
_FallbackMaterialScene()
{
    HdRetainedSceneIndexRefPtr const scene = HdRetainedSceneIndex::New();

    scene->AddPrims(
        { { _GetFallbackSurfaceMaterialPath(),
            HdPrimTypeTokens->material,
            HdRetainedContainerDataSource::New(
                HdMaterialSchema::GetSchemaToken(),
                _FallbackSurfaceMaterialDataSource()) } });

    return scene;
}

// The materialBindings data source underlaid onto geometry prims. It binds the
// allPurpose material to the fallback material path.
HdContainerDataSourceHandle
_FallbackSurfaceMaterialBindingsDataSource()
{
    static const TfToken purposes[] = {
        HdMaterialBindingsSchemaTokens->allPurpose
        //TfToken("full")
    };
    HdDataSourceBaseHandle const bindings[] = {
        HdMaterialBindingSchema::Builder()
            .SetPath(
                HdRetainedTypedSampledDataSource<SdfPath>::New(
                    _GetFallbackSurfaceMaterialPath()))
            .Build()
    };

    static const HdContainerDataSourceHandle result =
        HdRetainedContainerDataSource::New(
            HdMaterialBindingsSchema::GetSchemaToken(),
            HdMaterialBindingsSchema::BuildRetained(
                TfArraySize(purposes), purposes, bindings));

    return result;
}

// Scene index that underlays a fallback material binding on every geometry
// prim. Authored bindings take precedence (strong-first overlay), so only
// unbound gprims inherit the fallback.
TF_DECLARE_REF_PTRS(_HdPrmanFallbackMaterialsSceneIndex);

class _HdPrmanFallbackMaterialsSceneIndex
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    static _HdPrmanFallbackMaterialsSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr &inputSceneIndex)
    {
        return TfCreateRefPtr(
            new _HdPrmanFallbackMaterialsSceneIndex(inputSceneIndex));
    }

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override
    {
        HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

        if (prim.dataSource) {
            // Underlay the fallback material bindings.
            // XXX should we only do this if no mateiral binding exists?
            // so that we are robust if e.g. a preview material is
            // provided?
            if (_GetGeometrySurfacePrimTypes().count(prim.primType)) {
                prim.dataSource =
                    HdOverlayContainerDataSource::New(
                        prim.dataSource,
                        _FallbackSurfaceMaterialBindingsDataSource());
            } else if (_GetGeometryVolumePrimTypes().count(prim.primType)) {
                // TODO
//                prim.dataSource =
//                    HdOverlayContainerDataSource::New(
//                        prim.dataSource,
//                        _FallbackMaterialBindingsDataSource());
            }
        }

        return prim;
    }

    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override
    {
        return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    }

protected:
    _HdPrmanFallbackMaterialsSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex)
      : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
    {
    }

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override
    {
        _SendPrimsAdded(entries);
    }

    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override
    {
        _SendPrimsRemoved(entries);
    }

    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override
    {
        _SendPrimsDirtied(entries);
    }
};

} // anonymous namespace

////////////////////////////////////////////////////////////////////////////////
// Plugin registrations
////////////////////////////////////////////////////////////////////////////////

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<
        HdPrman_FallbackMaterialsSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 5;

    for (auto const &pluginDisplayName : HdPrman_GetPluginDisplayNames()) {
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            pluginDisplayName,
            _tokens->sceneIndexPluginName,
            /* inputArgs = */ nullptr,
            insertionPhase,
            HdSceneIndexPluginRegistry::InsertionOrderAtStart);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Scene Index Implementations
////////////////////////////////////////////////////////////////////////////////

HdPrman_FallbackMaterialsSceneIndexPlugin::
HdPrman_FallbackMaterialsSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_FallbackMaterialsSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    // Merge the input scene with the retained fallback material scene, then
    // wrap the result with the binding filter so unbound gprims are bound to
    // the fallback material.
    HdMergingSceneIndexRefPtr const merged = HdMergingSceneIndex::New();
    merged->AddInputScene(inputScene, SdfPath::AbsoluteRootPath());

    static HdSceneIndexBaseRefPtr const fallbackMaterialsScene =
        _FallbackMaterialScene();
    merged->AddInputScene(fallbackMaterialsScene, SdfPath("/__HdPrman"));

    return _HdPrmanFallbackMaterialsSceneIndex::New(merged);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2302
