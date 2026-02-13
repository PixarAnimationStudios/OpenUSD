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
#include "pxr/imaging/hd/materialBindingsSchema.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/materialInterfaceMappingSchema.h"
#include "pxr/imaging/hd/materialInterfaceParameterSchema.h"
#include "pxr/imaging/hd/materialInterfaceSchema.h"
#include "pxr/imaging/hd/materialNetworkSchema.h"
#include "pxr/imaging/hd/materialNodeParameterSchema.h"
#include "pxr/imaging/hd/materialNodeSchema.h"
#include "pxr/imaging/hd/materialOverrideSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primOriginSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexObserver.h" 
#include "pxr/imaging/hd/tokens.h" 
#include "pxr/imaging/hd/vectorSchema.h"
#include "pxr/imaging/hd/vectorSchemaTypeDefs.h"

#include "pxr/base/arch/hash.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/vt/visitValue.h"
#include "pxr/usd/usdShade/tokens.h"

#include <array>
#include <inttypes.h>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEBUG_CODES(
    HDSI_MATERIAL_OVERRIDES
);

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDSI_MATERIAL_OVERRIDES,
        "Log material override resolving information.");
}

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (materialOverrideDependency)
);

namespace { // begin anonymous namespace

// Struct used alongside VtVisitValue to create a hash of the interface
// and parameter values used as material overrides
struct VtHasher
{
    uint64_t* _hash;

    VtHasher(uint64_t* hash)
    : _hash(hash)
    {}

    template <class T>
    bool operator()(const VtArray<T>& array) const {
        if (!_hash) {
            return false;
        }

        for (const T& value : array) {
            *_hash = ArchHash64((const char*)&value, sizeof(T), *_hash);
        }
        return true;
    }

    template <class T>
    bool operator()(const T& value) const {
        if (!_hash) {
            return false;
        }

        *_hash = 
            ArchHash64((const char*)&value, sizeof(T), *_hash);
        return true;
    }
};

using TfTokenSet = std::unordered_set<TfToken, TfToken::HashFunctor>;
using TfTokenMap = std::unordered_map<TfToken, TfToken, TfToken::HashFunctor>;
using NestedTfTokenMap = 
    std::unordered_map<TfToken, TfTokenMap, TfToken::HashFunctor>;
using NestedTfTokenMapPtr = std::shared_ptr<NestedTfTokenMap>;

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
    // If the current _nodePath has any publicUI or parameter edit overrides, 
    // return the names of the material network parameters that have overrides.
    TfTokenSet 
    _GetOverrideNames()
    {
        TfTokenSet overrideNames;

        // If there are no overrides for this material, return an empty set.
        const HdMaterialOverrideSchema matOverSchema(
            _materialOverrideDsContainer);
        if (!matOverSchema) {
            return overrideNames;
        }

        // 1. Check for parameter edits.
        // Get the parameterValues data source and check if there are any 
        // parameter edits affecting the shader node at _nodePath.
        const HdNodeToInputToMaterialNodeParameterSchema parameterValuesSchema =
            matOverSchema.GetParameterValues();
        if (parameterValuesSchema) {
            HdMaterialNodeParameterContainerSchema nodeNameSchema = 
                parameterValuesSchema.Get(_nodePath);
            if (nodeNameSchema) {
                for (const TfToken& paramName : nodeNameSchema.GetNames()) {
                    // If we found a shader parameter override then we should
                    // add its name to GetNames()
                    overrideNames.insert(paramName);
                }                
            }
        }

        // 2. Check if our nodePath has interface mappings. If there are
        // no interface mappings, then there are no additional public UI 
        // override names to consider.
        if (!_reverseInterfaceMappingsPtr) {
            return overrideNames;
        }

        const auto searchParamsMap = 
            _reverseInterfaceMappingsPtr->find(_nodePath);
        if (searchParamsMap == _reverseInterfaceMappingsPtr->end()) {
            return overrideNames;
        }

        // 3. From the MaterialOverrides, check if we have an overridingDs
        // for the publicUI name
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
                // If we found a public UI override, then we should add its name
                // to GetNames()
                overrideNames.emplace(name);
            }
        }
        return overrideNames;
    }

    // Given 'name' of a material network parameter, return the overriding
    // data source (ie. the publicUI or parameter edit data source) if there is 
    // one specified.
    // Note that if both a publicUI and a parameter edit overrides for the same
    // data source exit, the public UI override takes precedence and will be
    // returned.
    HdContainerDataSourceHandle
    _GetOverrideContainerDataSource(const TfToken& name)
    {
        // Not using 'static' so we benefit from return value optimization
        const HdContainerDataSourceHandle emptyOverrideDs;

        // Nothing to do if there is no materialOverride data source
        const HdMaterialOverrideSchema matOverSchema(
            _materialOverrideDsContainer);
        if (!matOverSchema) {
            return emptyOverrideDs;
        }

        // If the same input is overridden both by an interface value and by
        // a parameter edit, the interface value edit should take precedence.
        // To enforce this requirement, process overrides to the PublicUI first,
        // and if one is found targeting the current parameter, return its data
        // source without bothering to look for a parameter edit.
        HdContainerDataSourceHandle overriddeContainerDs = 
            _GetPublicUIDataSource(name);
        if (overriddeContainerDs) {
            return overriddeContainerDs;
        }

        // If no interface edit was found, check for a parameter edit instead.
        overriddeContainerDs = _GetParameterEditDataSource(name);
        if (overriddeContainerDs) {
            return overriddeContainerDs;
        }

        return emptyOverrideDs;
    }

    // Given 'name' of a material network parameter, return its PublicUI data
    // source, if one is specified.
    HdContainerDataSourceHandle
    _GetPublicUIDataSource(const TfToken& name)
    {
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

    // Given 'name' of a material network parameter, return its Parameter Edit 
    // data source, if one is specified.
    HdContainerDataSourceHandle
    _GetParameterEditDataSource(const TfToken& name)
    {   
        const HdContainerDataSourceHandle emptyOverrideDs;

        const HdMaterialOverrideSchema matOverSchema(
            _materialOverrideDsContainer);
        if (!matOverSchema) {
            return emptyOverrideDs;
        }

        HdMaterialNodeParameterSchema overrideNodeParameterSchema =
            matOverSchema.GetParameterOverride(_nodePath, name);
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

        // Get the material network's public interface, which is required for
        // material override operations, but not for parameter edit operations
        const HdMaterialInterfaceSchema
            interfaceSchema = matNetworkSchema.GetInterface();
        std::shared_ptr<NestedTfTokenMap> reverseInterfaceMappingsPtr = nullptr;
        if (interfaceSchema) {
            // Build a reverse look-up for interface mappings which is keyed by
            // the material node parameter locations, which will be more 
            // efficient for look-ups when we later override the material node 
            // parameter
            reverseInterfaceMappingsPtr = 
                std::make_shared<NestedTfTokenMap>(
                    interfaceSchema.GetReverseInterfaceMappings());
        }

        return _MaterialNetworkContainerDataSource::New(
            matNetworkSchema.GetContainer(),
            matOverSchema.GetContainer(),
            reverseInterfaceMappingsPtr);
    }

private:
    HdContainerDataSourceHandle _inputDsContainer;
    HdContainerDataSourceHandle _materialDsContainer;
};

class _MaterialPrimContainerDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_MaterialPrimContainerDataSource);

    _MaterialPrimContainerDataSource(
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
        } else if (name == HdDependenciesSchema::GetSchemaToken()) {
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

class _MaterialBindingsContainerDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_MaterialBindingsContainerDataSource);

    _MaterialBindingsContainerDataSource(
        const HdContainerDataSourceHandle& inputDsContainer,
        const SdfPath& newBinding)
    : _inputDsContainer(inputDsContainer),
      _newBinding(newBinding)
    {   
    }

    // HdContainerDataSource overrides
    TfTokenVector GetNames() override
    {
        return _inputDsContainer->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        const HdDataSourceBaseHandle result = _inputDsContainer->Get(name);

        const HdContainerDataSourceHandle resultContainer = 
            HdContainerDataSource::Cast(result);
        if (!resultContainer) {
            return result;
        }

        static const TfTokenSet purposes = 
        {
            UsdShadeTokens->full,
            HdMaterialBindingsSchemaTokens->allPurpose,
        };

        // If the locator name matches one of the purposes listed above,
        // replace the materialBinding data source with one that uses 
        // _newBinding for its path.
        if (purposes.count(name) > 0) {
            const HdContainerDataSourceHandle overrideDs = 
                HdRetainedContainerDataSource::New(
                    HdMaterialBindingSchemaTokens->path, 
                    HdRetainedTypedSampledDataSource<SdfPath>::New(_newBinding));
            return HdOverlayContainerDataSource::OverlayedContainerDataSources(
                overrideDs, HdContainerDataSource::Cast(result));
        }

        return result;
    }

private:
    HdContainerDataSourceHandle _inputDsContainer;

    // Path to the material to use as a new binding
    SdfPath _newBinding;
};

class _BindablePrimContainerDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_BindablePrimContainerDataSource);

    _BindablePrimContainerDataSource(
        const HdContainerDataSourceHandle& inputDsContainer,
        const SdfPath& newBinding)
    : _inputDsContainer(inputDsContainer),
      _newBinding(newBinding)
    {
    }

    // HdContainerDataSource overrides
    TfTokenVector GetNames() override
    {
        return _inputDsContainer->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        const HdDataSourceBaseHandle result = _inputDsContainer->Get(name);

        const HdContainerDataSourceHandle resultContainer = 
            HdContainerDataSource::Cast(result);
        if (!resultContainer) {
            return result;
        }

        if (name == HdMaterialBindingsSchema::GetSchemaToken()) {
            return _MaterialBindingsContainerDataSource::New(
                resultContainer, _newBinding); 
        }

        return result;
    }

private:
    HdContainerDataSourceHandle _inputDsContainer;
    SdfPath _newBinding;
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
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    // XXX Do we need to query the input scene for a generated material?
    //     Likely not, which means that we can delay the above query...
    // Cannot early return based on prim.dataSource until after this block since
    // generated materials won't have a data source until after
    // _CreateGeneratedMaterialDataSource is called.
    if (_IsGeneratedMaterial(primPath)) {
        // If processing a generated material, create its data source as a copy 
        // of the material it was generated from
        _CreateGeneratedMaterialDataSource(prim, primPath);
    }

    if (!prim.dataSource) {
        return prim;
    }

    if (prim.primType == HdPrimTypeTokens->material) {
        // When processing a material, replace its data source with a wrapped 
        // data source, which will do the actual work of applying the override 
        // values to the correct material node parameters.
        prim.dataSource = 
            _MaterialPrimContainerDataSource::New(prim.dataSource, primPath);
    } else {
        auto it = _primData.find(primPath);
        if (it == _primData.end()) {
            return prim;
        }

        // When processing a geom with a materialOverride data source, replace 
        // its data source with a wrapped data source which will change its 
        // materialBindings to point to the generated material which contains 
        // the desired overrides
        prim.dataSource = 
            _BindablePrimContainerDataSource::New(
                prim.dataSource, it->second.generatedMaterialPath);
    }

    return prim;
}

SdfPathVector
HdsiMaterialOverrideResolvingSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    SdfPathVector childPrimPaths = 
        _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    
    // Add any generated materials
    const PathSet generatedChildPrimPaths = 
        _GetGeneratedMaterials(primPath);
    childPrimPaths.insert(
        childPrimPaths.end(), 
        generatedChildPrimPaths.begin(), 
        generatedChildPrimPaths.end());

    return childPrimPaths;
}

void
HdsiMaterialOverrideResolvingSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    _SendPrimsAdded(_AddGeneratedMaterials(entries));
}

void
HdsiMaterialOverrideResolvingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    _SendPrimsDirtied(_DirtyGeneratedMaterials(entries));
}

void
HdsiMaterialOverrideResolvingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    _SendPrimsRemoved(entries);
}

HdMaterialOverrideSchema
HdsiMaterialOverrideResolvingSceneIndex::_GetMaterialOverrides(
    const SdfPath& primPath) const
{
    const HdSceneIndexBaseRefPtr inputScene = _GetInputSceneIndex();
    const HdSceneIndexPrim prim = inputScene->GetPrim(primPath);
    return HdMaterialOverrideSchema::GetFromParent(prim.dataSource);
}

SdfPath
HdsiMaterialOverrideResolvingSceneIndex::_GetBoundMaterial(
    const SdfPath& primPath) const
{
    const HdSceneIndexBaseRefPtr inputScene = _GetInputSceneIndex();
    const HdSceneIndexPrim prim = inputScene->GetPrim(primPath);
    if (!prim.dataSource) {
        return {};
    }

    // Does this prim have a material binding data source ?
    HdMaterialBindingsSchema matBindingsSchema = 
        HdMaterialBindingsSchema::GetFromParent(prim.dataSource);
    if (!matBindingsSchema.IsDefined()) {
        return {};
    }

    // Find a material bound for a purpose of rendering the final frame
    // We will check these purposes in order, and take the first one that is
    // specified: 'full', 'allPurpose' and ''
    // These are the binding purposes expected to be used in a RenderMan
    // context. Material Overrides are not currently supported for 
    // preview materials.
    static const std::array<TfToken, 3> purposes = {
        UsdShadeTokens->full,
        HdMaterialBindingsSchemaTokens->_allPurposeToken,
        HdMaterialBindingsSchemaTokens->allPurpose
    };

    std::optional<HdMaterialBindingSchema> materialBindingSchemaOpt;
    for (const TfToken& purpose : purposes) {
        materialBindingSchemaOpt = matBindingsSchema.GetMaterialBinding(purpose);
        if (materialBindingSchemaOpt.has_value() && 
            materialBindingSchemaOpt.value().IsDefined()) {
            break;
        }
    }
    
    if (!materialBindingSchemaOpt) {
        return {};
    }

    const HdPathDataSourceHandle materialPathDs = 
        materialBindingSchemaOpt.value().GetPath();
    if (!materialPathDs) {
        return {};
    }
    return materialPathDs->GetTypedValue(0.0f);
}

SdfPath
HdsiMaterialOverrideResolvingSceneIndex::_AddGeneratedMaterial(
    const TfToken& primType,
    const SdfPath& primPath)
{
    // If this prim is a material, no special processing is required
    if (primType == HdPrimTypeTokens->material) {
        return {};
    }

    // If this prim is not renderable, then do not create a generated material
    if (!HdPrimTypeIsGprim(primType)) {
        return {};
    }

    // If this geom prim does not have material overrides, 
    // no further processing is required
    const HdMaterialOverrideSchema matOverrideSchema = 
        _GetMaterialOverrides(primPath);
    if (!matOverrideSchema.IsDefined()) {
        return {};
    }

    // If this geom with material overrides does not have
    // any materials bound to it, no further processing is required
    const SdfPath materialPath = _GetBoundMaterial(primPath);
    if (materialPath.IsEmpty()) {
        return {};
    }

    // Check to see if there is already a material satisfying this set of
    // material overrides
    const uint64_t matOverHash = _GetHash(matOverrideSchema);
    TF_DEBUG(HDSI_MATERIAL_OVERRIDES).Msg(
        "Adding a generated material for %s.\n"
        "\tA hash of %" PRIu64 " was computed.\n",
        primPath.GetText(), matOverHash);
    if (matOverHash == 0) {
        return {};
    }

    TF_DEBUG(HDSI_MATERIAL_OVERRIDES).Msg(
        "\tThis prim uses a the following base material: %s\n", 
        materialPath.GetText());

    auto hashIt = _materialHashMap.find(materialPath);
    if (hashIt != _materialHashMap.end()) {
        const auto& hashToNewMat = hashIt->second;
        auto newMatIt = hashToNewMat.find(matOverHash);
        if (newMatIt != hashToNewMat.end()) {
            const SdfPath generatedMaterialPath = newMatIt->second;

            // Use cached material
            TF_DEBUG(HDSI_MATERIAL_OVERRIDES).Msg(
                "\t!Cache hit! This prim will re-use this generated material"
                ": %s\n", generatedMaterialPath.GetText());

            auto materialDataIt = _materialData.find(generatedMaterialPath);
            if (materialDataIt == _materialData.end()) {
                TF_DEBUG(HDSI_MATERIAL_OVERRIDES).Msg(
                    "\t!ERROR! _materialData is unaware of generated material"
                    ": %s\n", generatedMaterialPath.GetText());
                return {};
            }
            
            if (materialDataIt->second.originalMaterialPath.IsEmpty()) {
                TF_DEBUG(HDSI_MATERIAL_OVERRIDES).Msg(
                    "\t!ERROR! _materialData does not have an "
                    "originalMaterialPath entry for generated material"
                    ": %s\n", generatedMaterialPath.GetText());
                return {};
            }
            
            _primData[primPath] = {generatedMaterialPath, matOverHash};
            _materialData[generatedMaterialPath].boundPrims.insert(primPath);
            return generatedMaterialPath;
        }
    }

    // Working with a geom, with material overrides, and a 
    // material bound to it.
    // A new material will need to be generated from the bound material.
    // This new generated material will need to be bound to this prim.
    // This is done so that overrides applied to the generated material do 
    // not spill to other prims bound to the original material.
    //
    // Example:
    // Given a prim with a material /World/.../Asset/Looks/Material bound 
    // to it, the following values will be generated below:
    // - materialPath /World/.../Asset/Looks/Material
    // - materialScopePath: /World/.../Asset/Looks
    // - newMaterialPath: /World/.../Asset/Looks/__MOR_Material_primName
    const std::string newMaterialName = "__MOR_" + materialPath.GetName() 
        + "_" + primPath.GetName();
    const SdfPath materialScopePath = materialPath.GetParentPath();
    SdfPath newMaterialPath = 
        materialScopePath.AppendChild(TfToken(newMaterialName));

    // Make sure this name is unique
    int materialSuffix = 1;
    while (_materialData.find(newMaterialPath) != _materialData.end()) {
        newMaterialPath = materialScopePath.AppendChild(
            TfToken(newMaterialName + std::to_string(materialSuffix++)));
    }

    _scopeToNewMaterialPaths[materialScopePath].insert(newMaterialPath);
    _oldToNewMaterialPaths[materialPath].insert(newMaterialPath);
    _materialData[newMaterialPath] = {materialPath, {primPath}};
    _primData[primPath] = {newMaterialPath, matOverHash};
    _materialHashMap[materialPath][matOverHash] = newMaterialPath;

    TF_DEBUG(HDSI_MATERIAL_OVERRIDES).Msg(
        "\tAdding new generated material %s\n", newMaterialPath.GetText());
    return newMaterialPath;
}

HdSceneIndexObserver::AddedPrimEntries 
HdsiMaterialOverrideResolvingSceneIndex::_AddGeneratedMaterials(
    const HdSceneIndexObserver::AddedPrimEntries& entries)
{
    HdSceneIndexObserver::AddedPrimEntries newEntries(entries);
    for (const HdSceneIndexObserver::AddedPrimEntry& entry : entries) {
        const SdfPath newMaterialPath = 
            _AddGeneratedMaterial(entry.primType, entry.primPath);
        if (newMaterialPath.IsEmpty()) {
            continue;
        }

        newEntries.push_back({newMaterialPath, HdPrimTypeTokens->material});
    }
    return newEntries;
}

HdSceneIndexObserver::DirtiedPrimEntries
HdsiMaterialOverrideResolvingSceneIndex::_DirtyGeneratedMaterials(
    const HdSceneIndexObserver::DirtiedPrimEntries& entries)
{
    // Use sets to produce a minimal number of invalidations
    PathSet addedEntriesSet;
    PathSet removedEntriesSet;
    PathSet dirtiedEntriesSet;
    const HdSceneIndexBaseRefPtr inputScene = _GetInputSceneIndex();

    // Set of prims processed as part of this block of dirtying operations
    PathSet processedPrimsSet;

    for (const HdSceneIndexObserver::DirtiedPrimEntry& entry : entries) {
        if (processedPrimsSet.count(entry.primPath) > 0) {
            TF_DEBUG(HDSI_MATERIAL_OVERRIDES).Msg(
                "Skipping %s because it was already processed in this "
                "change block.\n", entry.primPath.GetText());
            continue;
        }

        auto materialIt = _oldToNewMaterialPaths.find(entry.primPath);
        auto primIt = _primData.find(entry.primPath);

        if (materialIt != _oldToNewMaterialPaths.end()) {
            processedPrimsSet.insert(entry.primPath);
            _DirtyBaseMaterial(entry.primPath, materialIt->second, 
                &dirtiedEntriesSet);
        } else if (primIt != _primData.end()) {
            processedPrimsSet.insert(entry.primPath);
            _DirtyGeometry(entry, inputScene, primIt->second, 
                &processedPrimsSet, &addedEntriesSet, &dirtiedEntriesSet, 
                &removedEntriesSet);
        } else if (entry.dirtyLocators.Intersects(
            HdMaterialOverrideSchema::GetDefaultLocator())) {
            processedPrimsSet.insert(entry.primPath);
            _DirtyMaterialOverrideLocator(entry.primPath, inputScene, 
                &addedEntriesSet, &dirtiedEntriesSet);
        }
    }

    HdSceneIndexObserver::RemovedPrimEntries removedEntries;
    for (const SdfPath& removedPath : removedEntriesSet) {
        removedEntries.emplace_back(removedPath);
    }
    _SendPrimsRemoved(removedEntries);

    HdSceneIndexObserver::AddedPrimEntries addedEntries;
    for (const SdfPath& addedPath : addedEntriesSet) {
        addedEntries.emplace_back(addedPath, HdPrimTypeTokens->material);
    }
    _SendPrimsAdded(addedEntries);

    HdSceneIndexObserver::DirtiedPrimEntries newEntries(entries);
    for (const SdfPath& dirtiedPath : dirtiedEntriesSet) {
        static const HdDataSourceLocator containerLocator(
            HdDataSourceLocatorSentinelTokens->container);
            
        static const HdDataSourceLocatorSet locators {
            containerLocator,
            {HdMaterialSchema::GetSchemaToken(), 
                HdDataSourceLocatorSentinelTokens->container},
            {HdMaterialBindingsSchema::GetSchemaToken(), 
                HdDataSourceLocatorSentinelTokens->container}};
        newEntries.emplace_back(dirtiedPath, locators);
    }

    if (TfDebug::IsEnabled(HDSI_MATERIAL_OVERRIDES) && 
        // Only print if there are generated materials
        !_scopeToNewMaterialPaths.empty() &&
            // Only print if this dirty function did some work
            (!dirtiedEntriesSet.empty() || !addedEntriesSet.empty() || 
            !removedEntriesSet.empty())) {

        fprintf(stdout, "Current list of generated materials:\n");
        for (const auto& [materialScope, materialList] : 
            _scopeToNewMaterialPaths) {
            fprintf(stdout, "\t%s\n", materialScope.GetText());
            for (const SdfPath& materialPath : materialList) {
                fprintf(stdout, "\t\t%s\n", materialPath.GetText());
            }
        } 
    }

    return newEntries;
}

void
HdsiMaterialOverrideResolvingSceneIndex::_DirtyBaseMaterial(
    const SdfPath& primPath,
    const PathSet& generatedMaterials,
    PathSet* dirtiedPaths) const
{
    if (!dirtiedPaths) {
        return;
    }

    if (generatedMaterials.empty()) {
        return;
    }

    TF_DEBUG(HDSI_MATERIAL_OVERRIDES).Msg(
        "Processing dirty prim entry %s.\n", primPath.GetText());
    TF_DEBUG(HDSI_MATERIAL_OVERRIDES).Msg(
        "\tThis material was used to generate material overrides by "
        "creating the following materials:\n");

    // From a user standpoint, generated materials should be transparent.
    // If the material they were generated from changes, the changes
    // should be reflected in them.
    // Therefore, If a material used to generate other materials is 
    // dirtied, add the generated materials to the list of dirtied prims.
    for (const SdfPath& newMaterialPath : generatedMaterials) {
        TF_DEBUG(HDSI_MATERIAL_OVERRIDES).Msg(
            "\t\t%s\n", newMaterialPath.GetText());
        dirtiedPaths->insert(newMaterialPath);    
    }
}

void
HdsiMaterialOverrideResolvingSceneIndex::_DirtyGeometry(
    const HdSceneIndexObserver::DirtiedPrimEntry& entry,
    const HdSceneIndexBaseRefPtr inputScene,
    const PrimData& primData,
    PathSet* processedPrimsSet,
    PathSet* addedPaths,
    PathSet* dirtiedPaths,
    PathSet* removedPaths)
{
    if (!processedPrimsSet || !addedPaths || !dirtiedPaths) {
        return;
    }

    // If the set of material overrides on a prim changes also dirty
    // the generated material bound to it.
    if (!entry.dirtyLocators.Intersects(
        HdMaterialOverrideSchema::GetDefaultLocator())) {
        return;
    }

    // Sanity check: the dirty prim should be available on the input
    // scene index. This is needed to get the prim's type
    const HdSceneIndexPrim prim = inputScene->GetPrim(entry.primPath);
    if (!prim) {
        return;
    }
    
    // Sanity check: if prim data is available for this entry, then data
    // about its generated material should also be available.
    const SdfPath genMaterialPath = primData.generatedMaterialPath;
    auto newMaterialDataIt = _materialData.find(genMaterialPath);
    if (newMaterialDataIt == _materialData.end()) {
        return;
    }

    // Only geometry prims can receive material overrides
    if (!HdPrimTypeIsGprim(prim.primType)) {
        return;
    }

    // Processing a geometry prim which had previously received a 
    // generated material to express its material overrides.
    // The material overrides have now changed.
    TF_DEBUG(HDSI_MATERIAL_OVERRIDES).Msg(
        "Processing dirty prim entry %s.\n", entry.primPath.GetText());
    processedPrimsSet->insert(entry.primPath);

    // Remove the material that was created to express the intersection
    // of the previous material overrides and base material
    // Invalidate relevant bookkeeping data
    removedPaths->insert(genMaterialPath);

    std::vector<std::pair<TfToken, SdfPath>> primsToProcess;
    primsToProcess.push_back({prim.primType, entry.primPath});

    // Other prims that shared the same generated material with 
    // entry.primPath prior to that prim changing its material overrides
    // might need to have their generated materials adjusted.
    // This is needed, for example, in the case where multiple prims
    // shared the same generated material and only one of them 
    // received new material overrides
    auto primPathsIt = _materialData.find(genMaterialPath);
    if (primPathsIt != _materialData.end()) {
        for (const SdfPath& primPath : primPathsIt->second.boundPrims) {
            if (primPath == entry.primPath) {
                continue;
            }

            const HdSceneIndexPrim associatedPrim = 
                inputScene->GetPrim(primPath);
            if (!associatedPrim) {
                continue;
            }

            if (processedPrimsSet->count(primPath) > 0) {
                TF_DEBUG(HDSI_MATERIAL_OVERRIDES).Msg(
                "\tSkipping %s because it was already processed in this "
                "change block.\n", primPath.GetText());
                continue;
            }

            TF_DEBUG(HDSI_MATERIAL_OVERRIDES).Msg(
                "\tAlso mark %s as dirty because it uses the same "
                "generated material.\n", primPath.GetText());
            processedPrimsSet->insert(primPath);
            primsToProcess.push_back({associatedPrim.primType, primPath});
        }
    }

    // Invalidate stale data before generating new materials
    _InvalidateMaps(entry.primPath);

    // Figure out what the new material to express the new material
    // overrides would be. This could result in a cache hit.
    for (const auto& [primType, primPath] : primsToProcess) {
        const SdfPath newGeneratedMaterialPath = 
            _AddGeneratedMaterial(primType, primPath);
        // Adding primPath to the dirty pool to express
        // that its material bindings have changed (either because they
        // reverted to the original material or because a new generated
        // material is needed)
        dirtiedPaths->insert(primPath);
        if (!newGeneratedMaterialPath.IsEmpty()) {
            // If a new generated material is needed, mark it as being
            // added
            addedPaths->insert(newGeneratedMaterialPath);
        }
    }
}

void
HdsiMaterialOverrideResolvingSceneIndex::_DirtyMaterialOverrideLocator(
    const SdfPath& primPath,
    const HdSceneIndexBaseRefPtr inputScene,
    PathSet* addedPaths,
    PathSet* dirtiedPaths)
{
    if (!addedPaths || !dirtiedPaths) {
        return;
    }

    TF_DEBUG(HDSI_MATERIAL_OVERRIDES).Msg(
        "Processing dirty prim entry %s.\n", primPath.GetText());
    TF_DEBUG(HDSI_MATERIAL_OVERRIDES).Msg(
        "\tThis prim is receiving a material override for the first "
        "time.\n");

    // A prim which did not use to have a material override now 
    // received one. Add a generated material to account for this 
    // override if necessary
    const HdSceneIndexPrim prim = inputScene->GetPrim(primPath);
    if (!prim) {
        return;
    }

    const SdfPath newMatPath = 
        _AddGeneratedMaterial(prim.primType, primPath);
    if (!newMatPath.IsEmpty()) {
        // Adding again to dirty its material binding
        dirtiedPaths->insert(primPath);
        addedPaths->insert(newMatPath);
    }
}

PathSet
HdsiMaterialOverrideResolvingSceneIndex::_GetGeneratedMaterials(
    const SdfPath& primPath) const
{
    auto it = _scopeToNewMaterialPaths.find(primPath);
    if (it != _scopeToNewMaterialPaths.end()) {
        return it->second;
    }
    return {};
}

bool
HdsiMaterialOverrideResolvingSceneIndex::_IsGeneratedMaterial(
    const SdfPath& primPath) const
{
    auto materialIt = _materialData.find(primPath);
    return materialIt != _materialData.end();
}

void
HdsiMaterialOverrideResolvingSceneIndex::_CreateGeneratedMaterialDataSource(
    HdSceneIndexPrim& prim,
    const SdfPath& primPath) const
{
    TRACE_FUNCTION();

    static const HdContainerDataSourceHandle emptyHandle;

    // AddedPrimEntries will have already populated _materialData in the case
    // where a generated material is needed.
    auto materialIt = _materialData.find(primPath);
    if (materialIt == _materialData.end()) {
        return;
    }

    if (materialIt->second.boundPrims.empty()) {
        return;    
    }

    // Make a copy of the original material
    const HdSceneIndexBaseRefPtr& inputScene = _GetInputSceneIndex();
    const HdSceneIndexPrim originalMaterialPrim = 
        inputScene->GetPrim(materialIt->second.originalMaterialPath);
    prim.dataSource = HdMakeStaticCopy(originalMaterialPrim.dataSource);

    // Get the materialOverride data source from the geom that caused this
    // material to be generated
    const SdfPath materialOverridePath = *materialIt->second.boundPrims.begin();
    const HdSceneIndexPrim materialOverrideSourcePrim =
        inputScene->GetPrim(materialOverridePath);       
    const HdContainerDataSourceHandle materialOver = 
        materialOverrideSourcePrim.dataSource ?
            HdContainerDataSource::Cast(
                materialOverrideSourcePrim.dataSource->Get(
                    HdMaterialOverrideSchema::GetSchemaToken())) :
            emptyHandle;

    // Get the materialOverride data source from the original material, if any.
    // Overlay materialOverride data source from the geom on top of the 
    // materialOverrides from the original material
    const HdDataSourceBaseHandle originalMaterialOverrides = 
        prim.dataSource->Get(HdMaterialOverrideSchema::GetSchemaToken());
    const HdContainerDataSourceHandle overlayedMaterialOverrides = 
        HdOverlayContainerDataSource::OverlayedContainerDataSources(
            materialOver, 
            HdContainerDataSource::Cast(originalMaterialOverrides));

    // Overlay the fully resolved materialOverride data source on top of the
    // data source for the generated material
    prim.dataSource = 
        HdOverlayContainerDataSource::OverlayedContainerDataSources(
            HdRetainedContainerDataSource::New(
                HdMaterialOverrideSchema::GetSchemaToken(),
                overlayedMaterialOverrides),
            prim.dataSource);

    prim.primType = HdPrimTypeTokens->material;
}

uint64_t
HdsiMaterialOverrideResolvingSceneIndex::_GetHash(
    const HdMaterialOverrideSchema& materialOverrides) const
{
    uint64_t hash = 0;
    static const std::string interfaceValuesStr = 
        HdMaterialOverrideSchemaTokens->interfaceValues.GetString();
    static const std::string parameterValuesStr = 
        HdMaterialOverrideSchemaTokens->parameterValues.GetString();

    auto hashNodeParameterContainer = 
        [&hash](
            const HdMaterialNodeParameterContainerSchema& schema,
            const std::string& prefix) 
    {
        for (const TfToken& overrideValue : schema.GetNames()) {
            // Hash interface name
            const std::string name = prefix + overrideValue.GetString();
            hash = ArchHash64((const char *)name.c_str(), name.size(), hash);

            // Hash the interface value
            const HdMaterialNodeParameterSchema nodeParamSchema = 
                schema.Get(overrideValue);
            const VtValue value = nodeParamSchema.GetValue()->GetValue(0.0);
            VtVisitValue(value, VtHasher(&hash));
        }
    };

    // Hash interfaceValues
    const HdMaterialNodeParameterContainerSchema interfaceValuesSchema = 
        materialOverrides.GetInterfaceValues();
    hashNodeParameterContainer(interfaceValuesSchema, interfaceValuesStr);

    // Hash parameterValues
    const HdNodeToInputToMaterialNodeParameterSchema paramValuesSchema = 
        materialOverrides.GetParameterValues();
    for (const TfToken& nodeName : paramValuesSchema.GetNames()) {
        const HdMaterialNodeParameterContainerSchema paramValueNodeSchema =
            paramValuesSchema.Get(nodeName);
        hashNodeParameterContainer(paramValueNodeSchema, 
            parameterValuesStr + nodeName.GetString());
    }


    return hash;
}

void
HdsiMaterialOverrideResolvingSceneIndex::_InvalidateMaps(const SdfPath& primPath)
{
    auto primDataIt = _primData.find(primPath);
    if (primDataIt == _primData.end()) {
        return;
    }

    const PrimData& primData = primDataIt->second;
    const SdfPath generatedMaterialPath = primData.generatedMaterialPath;
    auto materialDataIt = _materialData.find(generatedMaterialPath);
    if (materialDataIt == _materialData.end()) {
        return;
    }

    const MaterialData& materialData = materialDataIt->second;
    const SdfPath originalMaterialPath = materialData.originalMaterialPath;
    const uint64_t materialOverrideHash = primData.materialOverrideHash;
    _materialHashMap[originalMaterialPath].erase(materialOverrideHash);

    const SdfPath materialScopePath = originalMaterialPath.GetParentPath();
    _scopeToNewMaterialPaths[materialScopePath].erase(generatedMaterialPath);
    _oldToNewMaterialPaths[originalMaterialPath].erase(generatedMaterialPath);
    for (const SdfPath& boundPrimPath : 
            _materialData[generatedMaterialPath].boundPrims) {
        _primData.erase(boundPrimPath);
    }

    _materialData.erase(generatedMaterialPath);
}

PXR_NAMESPACE_CLOSE_SCOPE
