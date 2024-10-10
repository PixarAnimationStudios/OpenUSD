//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/retesselationSceneIndexPlugin.h"

#include "pxr/base/tf/token.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/containerDataSourceEditor.h"
#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/schema.h"
#include "pxr/imaging/hd/tokens.h"
#if HD_API_VERSION >= 51
#include "pxr/imaging/hd/materialBindingsSchema.h"
#endif
#include "pxr/imaging/hd/dataSourceMaterialNetworkInterface.h"
#include "pxr/imaging/hd/materialBindingSchema.h"

#include "hdPrman/debugCodes.h"
#include "hdPrman/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    // render context / material network selector
    ((renderContext, "ri")));

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<HdPrman_RetesselationSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 0;

    for (const auto& pluginDisplayName : HdPrman_GetPluginDisplayNames()) {
        // XPU Doesn't Currently Need Geometry Dirtied For Displacement Edits
        if (pluginDisplayName.find("XPU") == std::string::npos) {
            HdSceneIndexPluginRegistry::GetInstance()
                .RegisterSceneIndexForRenderer(
                    pluginDisplayName, HdPrmanPluginTokens->retesselation,
                    nullptr, insertionPhase,
                    HdSceneIndexPluginRegistry::InsertionOrderAtStart);
        }
    }
}

TF_DECLARE_REF_PTRS(_RetesselationSceneIndex);

class _RetesselationSceneIndex : public HdSingleInputFilteringSceneIndexBase
{

public:
    static _RetesselationSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr& inputSceneIndex)
    {
        return TfCreateRefPtr(new _RetesselationSceneIndex(inputSceneIndex));
    }

    virtual HdSceneIndexPrim GetPrim(const SdfPath& primPath) const
    {
        return _GetInputSceneIndex()->GetPrim(primPath);
    }

    virtual SdfPathVector GetChildPrimPaths(const SdfPath& primPath) const
    {
        return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    }

protected:
    _RetesselationSceneIndex(const HdSceneIndexBaseRefPtr& inputSceneIndex)
        : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
    {
    }

    void _GetDirtyGeometryPrims(
        HdSceneIndexObserver::DirtiedPrimEntries& dirtyEntries,
        const SdfPath& materialPath,
        const SdfPath& geometryPath = SdfPath::AbsoluteRootPath())
    {
#if PXR_VERSION <= 2308 && defined(ARCH_OS_WINDOWS)
        return;
#else
        // Check Material Binding
        const HdSceneIndexPrim prim
            = _GetInputSceneIndex()->GetPrim(geometryPath);
#if HD_API_VERSION >= 51
        HdMaterialBindingsSchema materialBindings
            = HdMaterialBindingsSchema::GetFromParent(prim.dataSource);
        if (HdMaterialBindingSchema materialBinding
            = materialBindings.GetMaterialBinding()) {
            if (const HdPathDataSourceHandle ds = materialBinding.GetPath()) {
#else
        if (HdMaterialBindingSchema materialBinding
            = HdMaterialBindingSchema::GetFromParent(prim.dataSource)) {
            if (const HdPathDataSourceHandle ds
                = materialBinding.GetMaterialBinding()) {
#endif
                if (ds->GetTypedValue(0.0f) == materialPath) {
                    TF_DEBUG(HDPRMAN_RETESSELATION)
                        .Msg("Dirtying Geometry (%s) for Displacement Edit (%s)\n",
                            geometryPath.GetText(), materialPath.GetText());
                    dirtyEntries.push_back({ geometryPath,
                              HdPrimvarsSchema::GetDefaultLocator() });
                }
            }
        }

        // Check Child Prims
        for (const SdfPath& child :
             _GetInputSceneIndex()->GetChildPrimPaths(geometryPath)) {
            _GetDirtyGeometryPrims(dirtyEntries, materialPath, child);
        }
#endif
    }

    virtual void _PrimsAdded(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::AddedPrimEntries& entries)
    {
        for (const HdSceneIndexObserver::AddedPrimEntry& entry : entries) {

            // Get All Material Prims
            if (entry.primType == HdPrimTypeTokens->material) {
                const HdSceneIndexPrim prim
                    = _GetInputSceneIndex()->GetPrim(entry.primPath);

                // Get Material Data
#if PXR_VERSION <= 2308 && defined(ARCH_OS_WINDOWS)
                // Don't Support Windows
                const HdContainerDataSourceHandle materialDatasource;
#elif HD_API_VERSION >= 63
                const HdContainerDataSourceHandle materialDatasource
                    = HdMaterialSchema::GetFromParent(prim.dataSource)
                          .GetMaterialNetwork(_tokens->renderContext)
                          .GetContainer();
#else
                const HdContainerDataSourceHandle materialDatasource
                    = HdMaterialSchema::GetFromParent(prim.dataSource)
                          .GetMaterialNetwork(_tokens->renderContext);
#endif
                if (!materialDatasource)
                    continue;
#if PXR_VERSION <= 2308
                const HdDataSourceMaterialNetworkInterface materialNetwork(
                    entry.primPath, materialDatasource);
#else
                const HdDataSourceMaterialNetworkInterface materialNetwork(
                    entry.primPath, materialDatasource, prim.dataSource);
#endif
                const auto displacementTerminal
                    = materialNetwork.GetTerminalConnection(
                        HdMaterialTerminalTokens->displacement);

                // Displacement Was Added
                if (displacementTerminal.first) {
                    TF_DEBUG(HDPRMAN_RETESSELATION)
                        .Msg("Displacement Material Added (%s)\n",
                            entry.primPath.GetText());
                    cacheDisplacementNetworks.insert(
                        { entry.primPath, materialNetwork });
                    HdSceneIndexObserver::DirtiedPrimEntries dirtyGeometryEntries;
                    _GetDirtyGeometryPrims(dirtyGeometryEntries, entry.primPath);
                    _SendPrimsDirtied(dirtyGeometryEntries);
                }
            }
        }

        _SendPrimsAdded(entries);
    }

    virtual void _PrimsRemoved(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::RemovedPrimEntries& entries)
    {
        for (const HdSceneIndexObserver::RemovedPrimEntry& entry : entries) {
            // Displacement Was Removed
            if (cacheDisplacementNetworks.count(entry.primPath)) {
                TF_DEBUG(HDPRMAN_RETESSELATION)
                    .Msg("Displacement Material Removed (%s)\n",
                        entry.primPath.GetText());
                cacheDisplacementNetworks.erase(entry.primPath);
                HdSceneIndexObserver::DirtiedPrimEntries dirtyGeometryEntries;
                _GetDirtyGeometryPrims(dirtyGeometryEntries, entry.primPath);
                _SendPrimsDirtied(dirtyGeometryEntries);
            }
        }

        _SendPrimsRemoved(entries);
    }

    bool _NodeDirtied(
        const HdDataSourceMaterialNetworkInterface::InputConnection&
            inputConnection,
        const HdDataSourceMaterialNetworkInterface::InputConnection&
            cacheInputConnection,
        const HdDataSourceMaterialNetworkInterface& displacementNetwork,
        const HdDataSourceMaterialNetworkInterface& cacheDisplacementNetwork)
        const
    {
        // Check Input Connections Match
        if (inputConnection.upstreamNodeName
                != cacheInputConnection.upstreamNodeName
            || inputConnection.upstreamOutputName
                != cacheInputConnection.upstreamOutputName)
            return true;

        // Check Node Changes
        const TfToken& nodeName = inputConnection.upstreamNodeName;

        // Check Node Type Changes
        if (displacementNetwork.GetNodeType(nodeName)
            != cacheDisplacementNetwork.GetNodeType(nodeName))
            return true;

        // Check Node Authored Value Changes
        const TfTokenVector authoredValues
            = displacementNetwork.GetAuthoredNodeParameterNames(nodeName);
        if (authoredValues
            != cacheDisplacementNetwork.GetAuthoredNodeParameterNames(nodeName))
            return true;
        for (const TfToken& value : authoredValues)
            if (displacementNetwork.GetNodeParameterValue(nodeName, value)
                != cacheDisplacementNetwork.GetNodeParameterValue(
                    nodeName, value))
                return true;

        // Check Node Input Connection Changes
        const TfTokenVector inputConnectionNames
            = displacementNetwork.GetNodeInputConnectionNames(nodeName);
        if (inputConnectionNames
            != cacheDisplacementNetwork.GetNodeInputConnectionNames(nodeName))
            return true;
        for (const TfToken& inputConnectionName : inputConnectionNames) {
            const auto inputConnections
                = displacementNetwork.GetNodeInputConnection(
                    nodeName, inputConnectionName);
            const auto cacheInputConnections
                = cacheDisplacementNetwork.GetNodeInputConnection(
                    nodeName, inputConnectionName);

            if (inputConnections.size() != cacheInputConnections.size())
                return true;

            // Recursively Check Input Nodes
            for (size_t i = 0; i < inputConnections.size(); i++)
                if (_NodeDirtied(
                        inputConnections[i], cacheInputConnections[i],
                        displacementNetwork, cacheDisplacementNetwork))
                    return true;
        }

        return false;
    }

    virtual void _PrimsDirtied(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::DirtiedPrimEntries& entries)
    {
        for (const HdSceneIndexObserver::DirtiedPrimEntry& entry : entries) {

            // Get All Material Prims
            const HdSceneIndexPrim prim
                = _GetInputSceneIndex()->GetPrim(entry.primPath);
            if (prim.primType == HdPrimTypeTokens->material) {

                // Get Material Data
#if PXR_VERSION <= 2308 && defined(ARCH_OS_WINDOWS)
                // Don't Support Windows
                const HdContainerDataSourceHandle materialDatasource;
#elif HD_API_VERSION >= 63
                const HdContainerDataSourceHandle materialDatasource
                    = HdMaterialSchema::GetFromParent(prim.dataSource)
                          .GetMaterialNetwork(_tokens->renderContext)
                          .GetContainer();
#else
                const HdContainerDataSourceHandle materialDatasource
                    = HdMaterialSchema::GetFromParent(prim.dataSource)
                          .GetMaterialNetwork(_tokens->renderContext);
#endif
                if (!materialDatasource) {
                    // Displacement Datasource Was Removed
                    if (cacheDisplacementNetworks.count(entry.primPath)) {
                        TF_DEBUG(HDPRMAN_RETESSELATION)
                            .Msg("Couldn't Find Displacement Material Datasource (%s)\n",
                                entry.primPath.GetText());
                        cacheDisplacementNetworks.erase(entry.primPath);
                        HdSceneIndexObserver::DirtiedPrimEntries dirtyGeometryEntries;
                        _GetDirtyGeometryPrims(dirtyGeometryEntries, entry.primPath);
                        _SendPrimsDirtied(dirtyGeometryEntries);
                    }
                    continue;
                }

#if PXR_VERSION <= 2308
                const HdDataSourceMaterialNetworkInterface materialNetwork(
                    entry.primPath, materialDatasource);
#else
                const HdDataSourceMaterialNetworkInterface materialNetwork(
                    entry.primPath, materialDatasource, prim.dataSource);
#endif
                const auto displacementTerminal
                    = materialNetwork.GetTerminalConnection(
                        HdMaterialTerminalTokens->displacement);
                auto cacheDisplacementNetwork
                    = cacheDisplacementNetworks.find(entry.primPath);

                // Displacement Terminal Was Removed
                if (!displacementTerminal.first
                    && cacheDisplacementNetwork
                        != cacheDisplacementNetworks.end()) {
                    TF_DEBUG(HDPRMAN_RETESSELATION)
                        .Msg("Displacement Terminal Removed (%s)\n",
                            entry.primPath.GetText());
                    cacheDisplacementNetworks.erase(entry.primPath);
                    HdSceneIndexObserver::DirtiedPrimEntries dirtyGeometryEntries;
                    _GetDirtyGeometryPrims(dirtyGeometryEntries, entry.primPath);
                    _SendPrimsDirtied(dirtyGeometryEntries);
                }
                // Displacement Terminal Was Added
                else if (
                    displacementTerminal.first
                    && cacheDisplacementNetwork
                        == cacheDisplacementNetworks.end()) {
                    TF_DEBUG(HDPRMAN_RETESSELATION)
                        .Msg("Displacement Terminal Added (%s)\n",
                            entry.primPath.GetText());
                    cacheDisplacementNetworks.insert(
                        { entry.primPath, materialNetwork });
                    HdSceneIndexObserver::DirtiedPrimEntries dirtyGeometryEntries;
                    _GetDirtyGeometryPrims(dirtyGeometryEntries, entry.primPath);
                    _SendPrimsDirtied(dirtyGeometryEntries);
                }
                // Displacement May Have Changed
                else if (
                    displacementTerminal.first
                    && cacheDisplacementNetwork
                        != cacheDisplacementNetworks.end()) {
                    const auto cacheDisplacementTerminal
                        = cacheDisplacementNetwork->second
                              .GetTerminalConnection(
                                  HdMaterialTerminalTokens->displacement);
                    const bool displacementChanged = _NodeDirtied(
                        displacementTerminal.second,
                        cacheDisplacementTerminal.second, materialNetwork,
                        cacheDisplacementNetwork->second);
                    TF_DEBUG(HDPRMAN_RETESSELATION)
                        .Msg("Displacement Network Edited? (%s) %s\n",
                            entry.primPath.GetText(),
                            (displacementChanged ? "Yes" : "No"));

                    if (displacementChanged) {
                        cacheDisplacementNetwork->second = materialNetwork;
                        HdSceneIndexObserver::DirtiedPrimEntries dirtyGeometryEntries;
                        _GetDirtyGeometryPrims(dirtyGeometryEntries, entry.primPath);
                        _SendPrimsDirtied(dirtyGeometryEntries);
                    }
                }
            }
        }

        _SendPrimsDirtied(entries);
    }

    std::unordered_map<
        SdfPath,
        HdDataSourceMaterialNetworkInterface,
        SdfPath::Hash>
        cacheDisplacementNetworks;
};

HdPrman_RetesselationSceneIndexPlugin::HdPrman_RetesselationSceneIndexPlugin()
    = default;

HdSceneIndexBaseRefPtr
HdPrman_RetesselationSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr& inputScene,
    const HdContainerDataSourceHandle& inputArgs)
{
    return _RetesselationSceneIndex::New(inputScene);
}

PXR_NAMESPACE_CLOSE_SCOPE
