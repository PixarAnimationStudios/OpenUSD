//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdsi/materialOverrideResolvingSceneIndex.h"

#include "pxr/imaging/hd/containerDataSourceEditor.h"
#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/materialInterfaceMappingSchema.h"
#include "pxr/imaging/hd/materialNetworkSchema.h"
#include "pxr/imaging/hd/materialNodeParameterSchema.h"
#include "pxr/imaging/hd/materialNodeSchema.h"
#include "pxr/imaging/hd/materialOverrideSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexObserver.h" 
#include "pxr/imaging/hd/tokens.h" 
#include "pxr/imaging/hd/vectorSchema.h"
#include "pxr/imaging/hd/vectorSchemaTypeDefs.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/trace/trace.h"

#include <memory>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (materialOverrideDependency)
);

namespace { // begin anonymous namespace

using TfTokenSet = std::unordered_set<TfToken, TfToken::HashFunctor>;
using TfTokenMap = std::unordered_map<TfToken, TfToken, TfToken::HashFunctor>;
using NestedTfTokenMap = 
    std::unordered_map<TfToken, TfTokenMap, TfToken::HashFunctor>;
using NestedTfTokenMapPtr = std::shared_ptr<NestedTfTokenMap>;

// Given a material network container data source, returns a map of reversed
// interface mappings.  If no interface mappings were found, returns an empty
// map.
// 
// Interface mappings are mapped like this:
// publicUIName -> [(nodePath, inputName),...]
// 
// The returned map of reversed interface mappings is mapped like this:
// nodePath -> (inputName -> publicUIName)
NestedTfTokenMap
_BuildReverseInterfaceMappings(
    const HdContainerDataSourceHandle& matNetworkDsContainer)
{
    NestedTfTokenMap reverseInterfaceMappings;

    const HdMaterialNetworkSchema matNetworkSchema(matNetworkDsContainer);
    if (!matNetworkSchema) {
        return reverseInterfaceMappings;
    }

    HdMaterialInterfaceMappingsContainerSchema interfaceMappingsSchema = 
        matNetworkSchema.GetInterfaceMappings();
    if (!interfaceMappingsSchema) {
        return reverseInterfaceMappings;
    }

    for (const TfToken& publicUIName : interfaceMappingsSchema.GetNames()) {
        // Each publicUIName maps to a list of material node parameters ie.
        // [(nodePath, inputName), ...]
        const HdMaterialInterfaceMappingVectorSchema 
            interfaceMappingsVectorSchema =
            interfaceMappingsSchema.Get(publicUIName);
        if (!interfaceMappingsVectorSchema) {
            continue;
        }

        const size_t numElems = 
            interfaceMappingsVectorSchema.GetNumElements();
        for (size_t i = 0; i < numElems; i++) {
            // Each interfaceMapping should be a (nodePath, inputName) pair 
            HdMaterialInterfaceMappingSchema interfaceMappingSchema =
                interfaceMappingsVectorSchema.GetElement(i);
            if (!interfaceMappingSchema) {
                continue;
            }

            const TfToken nodePath = 
                interfaceMappingSchema.GetNodePath()->GetTypedValue(0);
            const TfToken inputName = 
                interfaceMappingSchema.GetInputName()->GetTypedValue(0);

            reverseInterfaceMappings[nodePath][inputName] = publicUIName;
        }
    }
    return reverseInterfaceMappings;
}

class _ParametersContainerDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_ParametersContainerDataSource);

    _ParametersContainerDataSource(
        const HdContainerDataSourceHandle& parametersDsContainer,
        const HdContainerDataSourceHandle& materialOverrideDsContainer,
        NestedTfTokenMapPtr reverseInterfaceMappingsPtr,
        const TfToken& nodePath)
    : _parametersDsContainer(parametersDsContainer),
      _materialOverrideDsContainer(materialOverrideDsContainer),
      _reverseInterfaceMappingsPtr(reverseInterfaceMappingsPtr),
      _nodePath(nodePath)
    {
    }

    // HdContainerDataSource overrides
    TfTokenVector GetNames() override
    {
        // Return all parameter names and material override names, since
        // it's possible that the specific parameter name does not exist yet.
        TfTokenVector names = _parametersDsContainer->GetNames();

        const TfTokenSet overrideNames = _GetOverrideNames();

        for (const TfToken& overrideName : overrideNames) {
            if (std::find(names.begin(), names.end(), overrideName) 
                == names.end()) 
            {
                names.emplace_back(overrideName);
            }   
        }

        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        // Don't return early if we don't have a ds for 'name' yet, because
        // it's possible there was no value authored for this parameter so it
        // doesn't have a ds, BUT we can still author a material override for
        // this parameter, and then it will need to have a ds.
        const HdDataSourceBaseHandle result = _parametersDsContainer->Get(name);

        // Get the override ds if there is one.  If there is no override, this
        // gets an empty container.
        const HdContainerDataSourceHandle overrideContainerDs = 
            _GetOverrideContainerDataSource(name);

        // Overlay the overridingDs onto the originalDs. If there is no
        // originalDs, overlays on a default empty container.
        return HdOverlayContainerDataSource::OverlayedContainerDataSources(
            overrideContainerDs,
            HdContainerDataSource::Cast(result)); 
    }

private:
    // If the current _nodePath has any publicUI overrides, return the names
    // of the material network parameters that have publicUI overrides.
    TfTokenSet 
    _GetOverrideNames()
    {
        TfTokenSet overrideNames;

        // 1. Check if our nodePath has interface mappings. If there are
        // no interface mappings, then there are no override names to
        // consider.
        if (!_reverseInterfaceMappingsPtr) {
            return overrideNames;
        }

        const auto searchParamsMap = 
            _reverseInterfaceMappingsPtr->find(_nodePath);
        if (searchParamsMap == _reverseInterfaceMappingsPtr->end()) {
            return overrideNames;
        }

        // 2. From the MaterialOverrides, check if we have an overridingDs
        // for the publicUI name
        const HdMaterialOverrideSchema matOverSchema(
            _materialOverrideDsContainer);
        if (!matOverSchema) {
            return overrideNames;
        }

        HdMaterialNodeParameterContainerSchema 
            interfaceValuesContainerSchema = 
            matOverSchema.GetInterfaceValues();
        if (!interfaceValuesContainerSchema) {
            return overrideNames;
        }

        const TfTokenMap& paramsMap = searchParamsMap->second;
        for (const auto& [name, publicUIName] : paramsMap) {
            HdMaterialNodeParameterSchema overrideNodeParameterSchema =
                interfaceValuesContainerSchema.Get(publicUIName);
            if (overrideNodeParameterSchema) {
                // If we found an override , then we should add its name
                // to GetNames()
                overrideNames.emplace(name);
            }
        }
        return overrideNames;
    }

    // Given 'name' of a material network parameter, return the overriding
    // data source (ie. the publicUI data source) if there is one specified.
    HdContainerDataSourceHandle
    _GetOverrideContainerDataSource(const TfToken& name)
    {
        // Not using 'static' so we benefit from return value optimization
        const HdContainerDataSourceHandle emptyOverrideDs;
        
        // 1. Look up the MaterialNodeParameter from our 
        // reverseInterfaceMappingsPtr to see if it has a publicUI name
        // ie. nodePath -> (name -> publicUIName)
        if (!_reverseInterfaceMappingsPtr) {
            return emptyOverrideDs;
        }

        const auto searchParamsMap = 
            _reverseInterfaceMappingsPtr->find(_nodePath);
        if (searchParamsMap == _reverseInterfaceMappingsPtr->end()) {
            return emptyOverrideDs;
        }

        const TfTokenMap& paramsMap = searchParamsMap->second;
        const auto search = paramsMap.find(name);
        if (search == paramsMap.end()) {
            return emptyOverrideDs;
        }

        const TfToken& publicUIName = search->second;

        // 2. From the MaterialOverrides, check if we have an overridingDs
        // for the publicUI name
        const HdMaterialOverrideSchema matOverSchema(
            _materialOverrideDsContainer);
        if (!matOverSchema) {
            return emptyOverrideDs;
        }

        HdMaterialNodeParameterContainerSchema 
            interfaceValuesContainerSchema = 
                matOverSchema.GetInterfaceValues();
        if (!interfaceValuesContainerSchema) {
            return emptyOverrideDs;
        }

        HdMaterialNodeParameterSchema overrideNodeParameterSchema =
            interfaceValuesContainerSchema.Get(publicUIName);
        if (!overrideNodeParameterSchema) {
            return emptyOverrideDs;
        }

        return overrideNodeParameterSchema.GetContainer();    
    }
     
private:
    HdContainerDataSourceHandle _parametersDsContainer;
    HdContainerDataSourceHandle _materialOverrideDsContainer;

    // Maps material node parameters to their public UI name.  
    // Ie. nodePath -> (inputName -> publicUIName)
    NestedTfTokenMapPtr _reverseInterfaceMappingsPtr;

    // The name of the MaterialNode that this MaterialNodeParameter belongs to.
    TfToken _nodePath;
};

class _MaterialNodeContainerDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_MaterialNodeContainerDataSource);

    _MaterialNodeContainerDataSource(
        const HdContainerDataSourceHandle& materialNodeDsContainer,
        const HdContainerDataSourceHandle& materialOverrideDsContainer,
        NestedTfTokenMapPtr reverseInterfaceMappingsPtr,
        const TfToken& nodePath)
    : _materialNodeDsContainer(materialNodeDsContainer),
      _materialOverrideDsContainer(materialOverrideDsContainer),
      _reverseInterfaceMappingsPtr(reverseInterfaceMappingsPtr),
      _nodePath(nodePath)
    {
    }

    // HdContainerDataSource overrides
    TfTokenVector GetNames() override
    {
        return _materialNodeDsContainer->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        const HdDataSourceBaseHandle result = 
            _materialNodeDsContainer->Get(name);

        // Only do work if our material node has 'parameters'
        if (name != HdMaterialNodeSchemaTokens->parameters) {
            return result;
        }

        const HdContainerDataSourceHandle resultContainer =
            HdContainerDataSource::Cast(result);
        if (!resultContainer) {
            return result;
        }

        return _ParametersContainerDataSource::New(
            resultContainer, 
            _materialOverrideDsContainer,
            _reverseInterfaceMappingsPtr,
            _nodePath
            );
    }

private:
    HdContainerDataSourceHandle _materialNodeDsContainer;
    HdContainerDataSourceHandle _materialOverrideDsContainer;

    // Maps material node parameters to their public UI name.  
    // Ie. nodePath -> (inputName -> publicUIName)
    NestedTfTokenMapPtr _reverseInterfaceMappingsPtr;

    // The name of the MaterialNode that this MaterialNodeParameter belongs to.
    TfToken _nodePath;
};

class _NodesContainerDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_NodesContainerDataSource);

    _NodesContainerDataSource(
        const HdContainerDataSourceHandle& nodesDsContainer,
        const HdContainerDataSourceHandle& materialOverrideDsContainer,
        NestedTfTokenMapPtr reverseInterfaceMappingsPtr)
    : _nodesDsContainer(nodesDsContainer),
      _materialOverrideDsContainer(materialOverrideDsContainer),
      _reverseInterfaceMappingsPtr(reverseInterfaceMappingsPtr)
    {
    }

    // HdContainerDataSource overrides
    TfTokenVector GetNames() override
    {
        return _nodesDsContainer->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        const HdDataSourceBaseHandle result = _nodesDsContainer->Get(name);

        const HdContainerDataSourceHandle resultContainer =
            HdContainerDataSource::Cast(result);
        if (!resultContainer) {
            return result;
        }

        // Members of the 'nodes' data source are only material nodes
        return _MaterialNodeContainerDataSource::New(
            resultContainer, 
            _materialOverrideDsContainer,
            _reverseInterfaceMappingsPtr,
            name);
    }

private:
    HdContainerDataSourceHandle _nodesDsContainer;
    HdContainerDataSourceHandle _materialOverrideDsContainer;

    // Maps material node parameters to their public UI name.  
    // Ie. nodePath -> (inputName -> publicUIName)
    NestedTfTokenMapPtr _reverseInterfaceMappingsPtr;
};

class _MaterialNetworkContainerDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_MaterialNetworkContainerDataSource);

    _MaterialNetworkContainerDataSource(
        const HdContainerDataSourceHandle& materialNetworkDsContainer,
        const HdContainerDataSourceHandle& materialOverrideDsContainer,
        NestedTfTokenMapPtr reverseInterfaceMappingsPtr)
    : _materialNetworkDsContainer(materialNetworkDsContainer),
      _materialOverrideDsContainer(materialOverrideDsContainer),
      _reverseInterfaceMappingsPtr(reverseInterfaceMappingsPtr)
    {
    }

    // HdContainerDataSource overrides
    TfTokenVector GetNames() override
    {
        return _materialNetworkDsContainer->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        const HdDataSourceBaseHandle result = 
            _materialNetworkDsContainer->Get(name);

        // Only do work if our material network has 'nodes'
        if (name != HdMaterialNetworkSchemaTokens->nodes) {
            return result;
        }

        const HdContainerDataSourceHandle resultContainer =
            HdContainerDataSource::Cast(result);
        if (!resultContainer) {
            return result;
        }

        return _NodesContainerDataSource::New(
            resultContainer, 
            _materialOverrideDsContainer,
            _reverseInterfaceMappingsPtr);
    }

private:
    HdContainerDataSourceHandle _materialNetworkDsContainer;
    HdContainerDataSourceHandle _materialOverrideDsContainer;

    // Maps material node parameters to their public UI name.  
    // Ie. nodePath -> (inputName -> publicUIName)
    NestedTfTokenMapPtr _reverseInterfaceMappingsPtr;
};

class _MaterialContainerDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_MaterialContainerDataSource);

    _MaterialContainerDataSource(
        const HdContainerDataSourceHandle& inputDsContainer,
        const HdContainerDataSourceHandle& materialDsContainer)
    : _inputDsContainer(inputDsContainer),
      _materialDsContainer(materialDsContainer)
    {
    }

    // HdContainerDataSource overrides
    TfTokenVector GetNames() override
    {
        return _materialDsContainer->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        const HdDataSourceBaseHandle result = _materialDsContainer->Get(name);

        const HdContainerDataSourceHandle resultContainer =
            HdContainerDataSource::Cast(result);
        if (!resultContainer) {
            return result;
        }

        // Only do work if we have a material network
        const HdMaterialNetworkSchema matNetworkSchema(resultContainer);
        if (!matNetworkSchema) {
            return result;
        }

        // Only do work if we have material overrides
        const HdMaterialOverrideSchema matOverSchema = 
            HdMaterialOverrideSchema::GetFromParent(_inputDsContainer);
        if (!matOverSchema) {
            return result;
        }

        // Only do work if the material network has interface mappings
        const HdMaterialInterfaceMappingsContainerSchema 
            interfaceMappingsSchema = matNetworkSchema.GetInterfaceMappings();
        if (!interfaceMappingsSchema) {
            return result;
        }

        // Build a reverse look-up for interface mappings which is keyed by
        // the material node parameter locations, which will be more 
        // efficient for look-ups when we later override the material node 
        // parameter
        auto reverseInterfaceMappingsPtr(
            std::make_shared<NestedTfTokenMap>(
                _BuildReverseInterfaceMappings(matNetworkSchema.GetContainer()))
            );

        return _MaterialNetworkContainerDataSource::New(
            matNetworkSchema.GetContainer(),
            matOverSchema.GetContainer(),
            reverseInterfaceMappingsPtr);
    }

private:
    HdContainerDataSourceHandle _inputDsContainer;
    HdContainerDataSourceHandle _materialDsContainer;
};

class _PrimContainerDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimContainerDataSource);

    _PrimContainerDataSource(
        const HdContainerDataSourceHandle& inputDsContainer,
        const SdfPath& primPath)
    : _inputDsContainer(inputDsContainer),
      _primPath(primPath)
    {   
    }

    // HdContainerDataSource overrides
    TfTokenVector GetNames() override
    {
        TfTokenVector names = _inputDsContainer->GetNames();

        if (std::find(names.begin(), names.end(), 
                HdDependenciesSchema::GetSchemaToken()) == names.end()) {
            names.push_back(HdDependenciesSchema::GetSchemaToken());
        }

        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        const HdDataSourceBaseHandle result = _inputDsContainer->Get(name);

        const HdContainerDataSourceHandle resultContainer = 
            HdContainerDataSource::Cast(result);
        if (!resultContainer) {
            return result;
        }

        if (name == HdMaterialSchema::GetSchemaToken()) {
            // Do work if we find 'material'
            return _MaterialContainerDataSource::New(
                _inputDsContainer, resultContainer); 
        } 
        else if (name == HdDependenciesSchema::GetSchemaToken()) {
            // Instead of implementing
            // HdsiMaterialOverrideResolvingSceneIndex::_PrimsDirtied(), we use 
            // the dependencies schema. The 'material' data source should depend
            // on changes to the 'materialOverride' data source.
            //
            // XXX: This coarse dependency between 'material' and 
            // 'materialOverride' will over-invalidate the material.
            // In the future, we can make the invalidation more fine-grained
            // by declaring the following dependencies:
            // * Each specific material node parameter of the material network
            //   should depend on its corresponding overriding material node 
            //   parameter from the material overrides. 
            //   Ie. If a specific material override gets updated, we only want 
            //   to replace that specific parameter in the network.
            // * Each specific material override should depend on its 
            //   corresponding interface mapping. 
            //   Ie. If the mapping itself changes and maps to a new network
            //   material node parameter, then that new material node parameter
            //   should receive the override.
            // * 'materialOverride' should depend on 'interfaceMappings' because
            //   if a publicUI is renamed, the corresponding material override
            //   also needs to be renamed.
            // * 'material' should depend on 'interfaceMappings' because if
            //   a mapping changes or is renamed, this affects the network 
            //   material node parameters.
            static HdLocatorDataSourceHandle const materialOverrideDsLocator =
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    HdMaterialOverrideSchema::GetDefaultLocator());
            static HdLocatorDataSourceHandle const materialDsLocator =
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    HdMaterialSchema::GetDefaultLocator());
                    
            // Overlay the material override dependency over any possible
            // existing dependencies
            return HdOverlayContainerDataSource::New(
                HdRetainedContainerDataSource::New(
                _tokens->materialOverrideDependency,
                HdDependencySchema::Builder()
                    .SetDependedOnPrimPath(
                        HdRetainedTypedSampledDataSource<SdfPath>::New(
                            _primPath))
                    .SetDependedOnDataSourceLocator(materialOverrideDsLocator)
                    .SetAffectedDataSourceLocator(materialDsLocator)
                    .Build()), 
                resultContainer);  
        }

        return result;
    }

private:
    HdContainerDataSourceHandle _inputDsContainer;

    // The scene index prim path
    SdfPath _primPath;
};

} // end anonymous namespace

HdsiMaterialOverrideResolvingSceneIndex::
    HdsiMaterialOverrideResolvingSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene)
: HdSingleInputFilteringSceneIndexBase(inputScene)
{
}

HdSceneIndexPrim
HdsiMaterialOverrideResolvingSceneIndex::GetPrim(const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    const HdSceneIndexBaseRefPtr inputScene = _GetInputSceneIndex();

    HdSceneIndexPrim prim = inputScene->GetPrim(primPath);

    if (prim.primType != HdPrimTypeTokens->material) {
        return prim;
    }

    // Only do work if we've found a "material" scene index prim.  Replace the 
    // data source with a wrapped data source, which will do the actual work of 
    // applying the override values to the correct material node parameters.
    if (prim.dataSource) {
        prim.dataSource = 
            _PrimContainerDataSource::New(prim.dataSource, primPath);
    }

    return prim;
}

SdfPathVector
HdsiMaterialOverrideResolvingSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdsiMaterialOverrideResolvingSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    TRACE_FUNCTION();

    _SendPrimsAdded(entries);
}

void
HdsiMaterialOverrideResolvingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    TRACE_FUNCTION();

    // We implement the dependencies schema instead of implementing 
    // _PrimsDirtied()
    _SendPrimsDirtied(entries);
}

void
HdsiMaterialOverrideResolvingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    TRACE_FUNCTION();

    _SendPrimsRemoved(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
