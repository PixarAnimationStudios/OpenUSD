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

If a geometry prim contains materialOverride data sources, a copy of
the material bound to that location will be generated, and the overrides will
only be applied to the generated material.
Any changes to the original material will be reflected in the generated material.
Geometry prims that contain materialOverride data sources are only
processed by this Scene Index if they contain a materialBindings data source
with material binds for one of the following purposes: full, allPurpose.
A minimum set of materials will be generated to satisfy material overrides: two
geometry prims bound to the same material and containing the same set of
material overrides will share the same generated material.

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
    // Forward Declarations
    struct MaterialData;
    struct PrimData;

    // Type Definitions
    using PrimDataMap = std::unordered_map<SdfPath, PrimData, SdfPath::Hash>;
    using MaterialPathsMap = std::unordered_map<SdfPath, PathSet, SdfPath::Hash>;
    using MaterialDataMap = std::unordered_map<SdfPath, MaterialData, 
        SdfPath::Hash>;
    using HashToMaterialPathMap = std::unordered_map<uint64_t, SdfPath>;
    using MaterialHashMap = std::unordered_map<SdfPath, HashToMaterialPathMap, 
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
    /// Returns the Material Override Schema for the prim at \p primPath
    /// The returned schema can be invalid. 
    HdMaterialOverrideSchema _GetMaterialOverrides(
        const SdfPath& primPath) const;

    /// Returns the path of the material bound to the prim at location 
    /// \p primPath for the 'full' or 'allPurpose' purposes (whichever is found 
    /// first).
    /// If no material is bound to this prim for either of those purposes, an
    /// empty path is returned.
    SdfPath _GetBoundMaterial(const SdfPath& primPath) const;

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

    /// Process dirty notices for materials used to generated override
    /// materials in this scene index.
    /// This function is responsible for propagating changes from base materials
    /// to the materials generated from them.
    /// \p primPath is the path of the dirty material prim
    /// \p generatedMaterials is a list of materials generated from \p primPath
    /// \p dirtiedPaths which is returned to the caller, will contain a set
    /// of paths dirtied by this call. New elements are appended to any 
    /// already there.
    void _DirtyBaseMaterial(
        const SdfPath& primPath,
        const PathSet& generatedMaterials,
        PathSet* dirtiedPaths) const;

    /// Process dirty notices for geometry that has received material overrides.
    /// \p entry is the dirty entry to operate on
    /// \p inputScene is a pointer to this Scene Index's input Scene Index
    /// \p primData contains material override data for the dirtied prim
    /// \p processedPrimsSet which is returned to the caller, will contain a set
    /// of paths for prims that have been processed by this function in addition
    /// to the prim described by \p entry. New elements are appended to any 
    /// already there.
    /// \p addedPaths \p dirtiedPaths and \p removedPaths which are all returned
    /// to the caller will contain sets of paths that have been added, dirtied
    /// or removed by this call. New elements are appended to any 
    /// already there.
    void _DirtyGeometry(
        const HdSceneIndexObserver::DirtiedPrimEntry& entry,
        const HdSceneIndexBaseRefPtr inputScene,
        const PrimData& primData,
        PathSet* processedPrimsSet,
        PathSet* addedPaths,
        PathSet* dirtiedPaths,
        PathSet* removedPaths);

    /// Process dirty notices to the materialOverride locator for prims which
    /// are receiving a material override for the first time.
    /// \p primPath is the dirty prim
    /// \p inputScene is a pointer to this Scene Index's input Scene Index
    /// \p addedPaths and \p dirtiedPaths which are all returned
    /// to the caller will contain sets of paths that have been added or dirtied
    /// by this call. New elements are appended to any already there.
    void _DirtyMaterialOverrideLocator(
        const SdfPath& primPath,
        const HdSceneIndexBaseRefPtr inputScene,
        PathSet* addedPaths,
        PathSet* dirtiedPaths);

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

    /// Invalidate the portion of the bookkeeping maps of this scene index
    /// that contain data about the prim located at \p primPath
    void _InvalidateMaps(const SdfPath& primPath);

    /// Struct describing data for materials generated in the process of
    /// resolving overrides set on non-material locations
    struct MaterialData
    {
        // Path to the material bound to materialOverridePrimPath
        // before any changes are made by this scene index
        SdfPath originalMaterialPath;

        // Set of geometry prims using this generated material
        PathSet boundPrims;
    };

    /// Struct describing data for geometry prims using generated materials to
    /// express material overrides
    struct PrimData
    {
        // Path to the generated material bound to this geometry prim
        SdfPath generatedMaterialPath;

        // Hash of the material overrides found on this prim
        uint64_t materialOverrideHash;
    };

    // Map linking a path of a material scope to a set of materials that are 
    // generated by this scene index as a result of resolving material
    // overrides at non-material locations.
    MaterialPathsMap _scopeToNewMaterialPaths;

    // Map linking a material to a set of materials generated from it.
    MaterialPathsMap _oldToNewMaterialPaths;

    // Map linking geometry prims to the path of the generated material that
    // will be bound to them and to the hash for their material overrides.
    // The keys in this map represent geometry prims that have
    // a material binding and a material override data source.
    PrimDataMap _primData;

    // Map linking materials generated in the process of resolving material
    // overrides to their data.
    MaterialDataMap _materialData;

    // Map relating a geometry prim's bound material (before this scene index
    // executes) and the hash of its material override data source to the path
    // of a generated material.
    // Used to allow multiple geometry prims that share the same starting 
    // material and material overrides to use the same generated material.
    // For example:
    // PlasticMaterial --> ..0123 --> __MOR_PlasticMaterial_Mesh
    //                 --> ..4567 --> __MOR_PlasticMaterial_AnotherMesh
    // WoodMaterial    --> ..0123 --> __MOR_WoodMaterial_Mesh
    // MetalMaterial   --> ..4567 --> __MOR_MetalMaterial_Point
    MaterialHashMap _materialHashMap;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDSI_MATERIAL_OVERRIDE_RESOLVING_SCENE_INDEX_H