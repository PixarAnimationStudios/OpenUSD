//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDSI_MATERIAL_OVERRIDE_RESOLVING_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_MATERIAL_OVERRIDE_RESOLVING_SCENE_INDEX_H

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/materialBindingSchema.h"
#include "pxr/imaging/hd/materialOverrideSchema.h"
#include "pxr/imaging/hdsi/api.h"

#include <optional>
#include <unordered_map>
#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

using PathSet = std::unordered_set<SdfPath, SdfPath::Hash>;

TF_DECLARE_WEAK_AND_REF_PTRS(HdsiMaterialOverrideResolvingSceneIndex);

/*
HdsiMaterialOverrideResolvingSceneIndex applies material overrides in the form
of edits to a material's interface or directly to parameters of its shader nodes.

Overrides to a material's interface are only applied if their interface mappings 
are found.
The overrides for a particular 'publicUIName' are specified at path like this:

materialOverride.interfaceValues.<publicUIName>.value 
    -> overrideValueDataSource

The 'overrideValueDataSource' is copied over a network node parameter's original
'valueDataSource' at paths like this:

material.<renderContext>.nodes.<nodePath>.parameters.<inputName>.value
    -> valueDataSource

The scene index identifies which network node parameter to override by using
the material's interface mappings which are defined at paths like this:

material.<renderContext>.interface.parameters.<publicUIName>.mappings 
    -> [(nodePath, inputName), (nodePath, inputName), ...]

Edits of input parameters of shaders within the material are specified at a path 
like this:
materialOverride.parameterValues.<shaderNodeName>.<parameterName>.value ->
    paramEditValueDataSource

The 'paramEditValueDataSource' is copied over a network node parameter's original
'valueDataSource' at paths like this:

material.<renderContext>.nodes.<nodePath>.parameters.<inputName>.value
    -> valueDataSource

If the same input parameter is overridden both by an edit to the material
interface and by a direct parameter edit, the interface override will take 
precedence.

If a non-material location contains materialOverride data sources, a copy of
the material bound to that location will be generated, and the overrides will
only be applied to the generated material.
Any changes to the original material will be reflected in the generated material.
Non-material locations that contain materialOverride data sources are only
processed by this Scene Index if they contain a materialBindings data source
with material binds for one of the following purposes: full, allPurpose.

Below is a diagram of the expected attributes needed for material interface
and parameter edits on a scene index prim of type 'material':

MaterialPrim
|
+------materialOverride
|      |
|      +----interfaceValues
|      |    |
|      |    +-publicUIName
|      |    |   |
|      |    |   +---value -> overrideValueDataSource
|      |    |
|      |    +-publicUIName
|      |    |   |
|      |    |   +---value -> overrideValueDataSource
|      |    |
|      |    +-...
|      |
|      +----parameterValues  
|           |
|           +-nodePath
|           |   |
|           |   +---inputName
|           |   |   |
|           |   |   +---value -> paramEditValueDataSource
|           |   |
|           |   +---inputName
|           |   |   |
|           |   |   +---value -> paramEditValueDataSource
|           |   |
|           |   +---...
|           |  
|           +-...
|
+------material
       |
       +----ri
            |
            +--nodes
            |    |
            |    +-nodePath
            |    |     |
            |    |     +-parameters
            |    |           |
            |    |           +-inputName
            |    |           |   |
            |    |           |   +-value -> valueDataSource
            |    |           |
            |    |           +-inputName
            |    |               |
            |    |               +-value -> valueDataSource
            |    |
            |    +-nodePath  
            |          |
            |          +-parameters           
            |                |
            |                +-...       
            |
            +--interface
                 |
                 +-parameters
                 |     |
                 |     +-publicUIName
                 |     |     |
                 |     |     +-mappings
                 |     |         +-i0
                 |     |         |  |
                 |     |         |  +----nodePath  
                 |     |         |  |
                 |     |         |  +----inputName  
                 |     |         |
                 |     |         +-i1
                 |     |         |  |
                 |     |         |  +----nodePath  
                 |     |         |  |
                 |     |         |  +----inputName  
                 |     |         |
                 |     |         +-...
                 |     |
                 |     +-publicUIName 
                 |     |     |
                 |     |     +-mappings
                 |     |         +-i0
                 |     |         |  |
                 |     |         |  +----nodePath  
                 |     |         |  |
                 |     |         |  +----inputName  
                 |     |         |
                 |     |         +-i1
                 |     |         |  |
                 |     |         |  +----nodePath  
                 |     |         |  |
                 |     |         |  +----inputName  
                 |     |         |
                 |     |         +-...
                 |     +-...
                 |
                 +-parameterOrder = ...
*/

class HdsiMaterialOverrideResolvingSceneIndex final : 
    public HdSingleInputFilteringSceneIndexBase
{
    struct MaterialData;
    using PathMap = std::unordered_map<SdfPath, SdfPath, SdfPath::Hash>;
    using MaterialPathsMap = std::unordered_map<SdfPath, PathSet, SdfPath::Hash>;
    using MaterialDataMap = std::unordered_map<SdfPath, MaterialData, 
        SdfPath::Hash>;

public:
    static HdsiMaterialOverrideResolvingSceneIndexRefPtr New(
        const HdSceneIndexBaseRefPtr &inputScene)
    {
        return TfCreateRefPtr(
            new HdsiMaterialOverrideResolvingSceneIndex(inputScene));
    }

    // HdSceneIndexBase overrides
    HDSI_API 
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:

    HDSI_API
    HdsiMaterialOverrideResolvingSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene);

    // HdSingleInputFilteringSceneIndexBase overrides
    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;

    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;

    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

private:
    /// Returns true if the prim at location \p primPath has a materialOverrides
    /// data source. Returns false otherwise.
    bool _DoesPrimHaveMaterialOverrides(const SdfPath& primPath) const;

    /// Returns the MaterialBindings Schema for the prim at location \p primPath.
    /// The returned optional object will not have a value if the prim at 
    /// \p primPath does not have a materialBindings data source.
    std::optional<HdMaterialBindingSchema> _GetMaterialBindings(
        const SdfPath& primPath) const;

    /// Given a vector of AddedPrimEntry \p entries, decide whether new 
    /// materials need be generated to resolve material overrides.
    /// Returns a vector of AddedPrimEntry that includes all elements of
    /// \p entries plus any that were added to account for generated materials.
    HdSceneIndexObserver::AddedPrimEntries _AddGeneratedMaterials(
        const HdSceneIndexObserver::AddedPrimEntries& entries);

    /// Given a Hydra prim described by its type \p primType and its path 
    /// \p primPath decide whether a generated material is needed to 
    /// satisfy its material overrides.
    /// Returns the path to the generated material, or an empty path is no
    /// new generated material is required.
    SdfPath _AddGeneratedMaterial(
        const TfToken& primType, 
        const SdfPath& primPath);

    /// Given a vector of DirtiedPrimEntry \p entries, create extra entries
    /// for generated materials, as needed.
    /// Returns a vector of DirtiedPrimEntry that includes all elements of
    /// \p entries plus any that were added to account for generated materials.
    HdSceneIndexObserver::DirtiedPrimEntries _DirtyGeneratedMaterials(
        const HdSceneIndexObserver::DirtiedPrimEntries& entries);

    /// Given a path \p primPath to a material scope, return a set of 
    /// generated materials that are located under it 
    PathSet _GetGeneratedMaterials(const SdfPath& primPath) const;

    /// Returns true if the Hydra Material at \p primPath was generated by
    /// this scene index. Returns false otherwise
    bool _IsGeneratedMaterial(const SdfPath& primPath) const;

    /// Given a Hydra Material \p prim at location \p primPath which was 
    /// generated by this scene index, populate its data source.
    void _CreateGeneratedMaterialDataSource(
        HdSceneIndexPrim& prim,
        const SdfPath& primPath) const;

    /// Returns a hashed value for the \p materialOverrides schema
    uint64_t _GetHash(const HdMaterialOverrideSchema& materialOverrides) const;

    /// Struct describing data for materials generated in the process of
    /// resolving overrides set on non-material locations
    struct MaterialData
    {
        // Path to the material bound to materialOverridePrimPath
        // before any changes are made by this scene index
        SdfPath originalMaterialPath;

        // Path to the non-material location that contains material overrides
        SdfPath materialOverridePrimPath;
    };

    // Map linking a path of a material scope to a set of materials that are 
    // generated by this scene index as a result of resolving material
    // overrides at non-material locations.
    MaterialPathsMap _scopeToNewMaterialPaths;

    // Map linking a material to a set of materials generated from it.
    MaterialPathsMap _oldToNewMaterialPaths;

    // Map linking non-material prims to the path of the generated material that
    // will be bound to them to to resolve material overrides.
    // The keys in this map represent non-material prims that have
    // a material binding and a material override data source.
    PathMap _primToNewBindingMap;

    // Map linking materials generated in the process of resolving material
    // overrides to their data.
    MaterialDataMap _newMaterialData;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDSI_MATERIAL_OVERRIDE_RESOLVING_SCENE_INDEX_H