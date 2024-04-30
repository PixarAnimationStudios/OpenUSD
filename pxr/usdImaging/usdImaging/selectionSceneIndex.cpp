//
// Copyright 2022 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/usdImaging/usdImaging/selectionSceneIndex.h"

#include "pxr/usdImaging/usdImaging/debugCodes.h"
#include "pxr/usdImaging/usdImaging/usdPrimInfoSchema.h"
#include "pxr/imaging/hd/instanceSchema.h"
#include "pxr/imaging/hd/instancedBySchema.h"
#include "pxr/imaging/hd/instanceIndicesSchema.h"
#include "pxr/imaging/hd/instancerTopologySchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/selectionSchema.h"
#include "pxr/imaging/hd/selectionsSchema.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace UsdImagingSelectionSceneIndex_Impl
{

// The following struct's align with the selection Hydra schemata.
//
// Data for HdInstanceIndicesSchema.
//
// Consider a piece of geometry within a prototype which is instanced.
// In particular, it has a data source for "instancedBy".
//
// Note that the instancer instancing the geometry can have multiple
// prototypes, given as array.
//
// To select particular instances of this geometry, we give the following
// information:
//
struct _InstanceIndices
{
    // The instancer.
    SdfPath instancer;
    // The index of the prototype within the prototypes of the above instancer.
    int prototypeIndex;
    // The selected instances.
    VtIntArray instanceIndices;
};

// Data for HdInstanceIndicesVectorSchema
//
// Consider a piece of geometry within a prototype which is instanced
// by an instancer within a prototype which is instanced, ...
//
// To select a particular instance of this geometry, we need to store
// _InstanceIndices at each level.
// We do this starting with the outermost instancer.
//
using _InstanceIndicesVector = std::vector<_InstanceIndices>;

// Data for HdSelectionSchema
struct _Selection
{
    // In the future, this might contain further information such as which
    // highlighting mode is used or whether some subset of faces, ...
    // is selected.

    // Which nested instances of this piece of geometry are selected.
    _InstanceIndicesVector nestedInstanceIndices;
};

// Data for HdSelectionsSchema.
using _Selections = std::vector<_Selection>;

// Information shared between scene index and data sources.
struct _SelectionInfo
{
    // The selection state by path in the scene index.
    std::map<SdfPath, _Selections> primToSelections;
};

HdContainerDataSourceHandle
_ToDs(const _InstanceIndices &i)
{
    return
        HdInstanceIndicesSchema::Builder()
            .SetInstancer(
                HdRetainedTypedSampledDataSource<SdfPath>::New(
                    i.instancer))
            .SetPrototypeIndex(
                HdRetainedTypedSampledDataSource<int>::New(
                    i.prototypeIndex))
            .SetInstanceIndices(
                HdRetainedTypedSampledDataSource<VtIntArray>::New(
                    i.instanceIndices))
            .Build();
}

HdVectorDataSourceHandle
_ToDs(const _InstanceIndicesVector &v)
{
    if (v.empty()) {
        return nullptr;
    }

    std::vector<HdDataSourceBaseHandle> r;
    r.reserve(v.size());
    for (const auto &i : v) {
        r.push_back(_ToDs(i));
    }

    return HdRetainedSmallVectorDataSource::New(
        r.size(),
        r.data());
}

HdContainerDataSourceHandle
_ToDs(const _Selection &s)
{
    return
        HdSelectionSchema::Builder()
            .SetFullySelected(
                HdRetainedTypedSampledDataSource<bool>::New(true))
            .SetNestedInstanceIndices(
                _ToDs(s.nestedInstanceIndices))
            .Build();
}

HdVectorDataSourceHandle
_ToDs(const _Selections &s)
{
    if (s.empty()) {
        return nullptr;
    }

    std::vector<HdDataSourceBaseHandle> r;
    r.reserve(s.size());
    for (const auto &i : s) {
        r.push_back(_ToDs(i));
    }

    return HdRetainedSmallVectorDataSource::New(
        r.size(),
        r.data());
}

// Populate selections data source for prims using _SelectionInfo
//
class _PrimSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimSource);

    _PrimSource(HdContainerDataSourceHandle const &inputSource,
                _SelectionInfoSharedPtr const &selectionInfo,
                const SdfPath &primPath)
        : _inputSource(inputSource)
        , _selectionInfo(selectionInfo)
        , _primPath(primPath)
    {
    }

    TfTokenVector GetNames() override
    {
        TfTokenVector names = _inputSource->GetNames();
        auto it = _selectionInfo->primToSelections.find(_primPath);
        if (it != _selectionInfo->primToSelections.end()) {
            names.push_back(HdSelectionsSchema::GetSchemaToken());
        }
        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (name == HdSelectionsSchema::GetSchemaToken()) {
            auto it = _selectionInfo->primToSelections.find(_primPath);
            if (it != _selectionInfo->primToSelections.end()) {
                return _ToDs(it->second);
            } else {
                return nullptr;
            }
        }

        return _inputSource->Get(name);
    }

private:
    HdContainerDataSourceHandle const _inputSource;
    _SelectionInfoSharedPtr const _selectionInfo;
    const SdfPath _primPath;
};

///////////////////////////////////////////////////////////////////////////////
//
// Code to convert USD paths to scene index paths and nested instance
// indices.

struct _PrimAndNestedInstanceIndices
{
    SdfPath prim;
    _InstanceIndicesVector nestedInstanceIndices;
};

// Safe accessor for vector data source.
SdfPath
_GetPath(HdPathArrayDataSourceHandle const dataSource,
         const int index)
{
    if (index < 0) {
        return SdfPath();
    }

    if (!dataSource) {
        return SdfPath();
    }

    const VtArray<SdfPath> paths = dataSource->GetTypedValue(0.0f);

    if (!(static_cast<size_t>(index) < paths.size())) {
        return SdfPath();
    }

    return paths[index];
}

// An instancer can have multiple prototypes stored in an array.
//
// Get the index of the given prototype in that array.
int
_GetIndexOfPrototype(HdInstancerTopologySchema &schema,
                     const SdfPath &prototype)
{
    HdPathArrayDataSourceHandle const prototypesDs = schema.GetPrototypes();
    if (!prototypesDs) {
        return 0;
    }
    const VtArray<SdfPath> &prototypes = prototypesDs->GetTypedValue(0.0f);
    for (size_t i = 0; i < prototypes.size(); i++) {
        if (prototypes[i] == prototype) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

// Get how often the prototype identified by prototypeIndex is
// instantiated by the instancer.
size_t
_NumInstances(HdInstancerTopologySchema &schema,
              const size_t prototypeIndex)
{
    HdIntArrayDataSourceHandle const indicesDs =
        schema.GetInstanceIndices().GetElement(prototypeIndex);
    if (!indicesDs) {
        return 0;
    }

    const VtIntArray &indices = indicesDs->GetTypedValue(0.0f);
    return indices.size();
}

// [0, 1, 2, ..., n - 1]
//
VtIntArray
_Range(const size_t n)
{
    VtIntArray result(n);
    for (size_t i = 0; i < n; i++) {
        result[i] = i;
    }
    return result;
}

// If the prim in the scene index at prototypePath is a prototype,
// return _InstanceIndices to select all instances of the prototype.
// If the prim is not a prototype, _InstanceIndices::instancer will
// be empty.
//
// A prim is considered a prototype if its instancedBy data source
// identifies an instancer (that is a prim with an instancer topology
// data source) and the instancer's prototype paths includes the prim
// itself.
// 
_InstanceIndices
_ComputeAllInstanceIndicesForPrototype(
    const SdfPath &prototypePath,
    HdSceneIndexBaseRefPtr const &sceneIndex)
{
    HdContainerDataSourceHandle const primSource =
        sceneIndex->GetPrim(prototypePath).dataSource;
    HdInstancedBySchema instancedBySchema =
        HdInstancedBySchema::GetFromParent(primSource);
    HdPathArrayDataSourceHandle instancersDs = instancedBySchema.GetPaths();
    if (!instancersDs) {
        return { SdfPath(), 0, {}};
    }

    const VtArray<SdfPath> &instancers = instancersDs->GetTypedValue(0.0f);
    if (instancers.empty()) {
        return { SdfPath(), 0, {}};
    }

    if (instancers.size() > 1) {
        TF_CODING_ERROR("Expected at most one instancer");
    }

    const SdfPath &instancer = instancers[0];
    HdInstancerTopologySchema instancerTopologySchema =
        HdInstancerTopologySchema::GetFromParent(
            sceneIndex->GetPrim(instancer).dataSource);
    const int prototypeIndex =
        _GetIndexOfPrototype(instancerTopologySchema, prototypePath);

    const VtIntArray instanceIndices =
        _Range(_NumInstances(instancerTopologySchema, prototypeIndex));

    return { instancer, prototypeIndex, instanceIndices};
}

// If a prim in the scene index at prototypePath is a prototype (see above),
// returns the nested instance indices to select all instances of the
// prototype.
//
// That is, if the instancer instancing the prototype is itself in a
// prototype that is instances, and so on... include the instance indices
// for each level of nesting.
//
_InstanceIndicesVector
_ComputeAllNestedInstanceIndicesForPrototype(
    const SdfPath &prototypePath,
    HdSceneIndexBaseRefPtr const &sceneIndex)
{
    std::vector<_InstanceIndices> result;

    SdfPath path = prototypePath;

    while (true) {
        _InstanceIndices instanceIndices =
            _ComputeAllInstanceIndicesForPrototype(
                path, sceneIndex);

        if (TfDebug::IsEnabled(USDIMAGING_SELECTION)) {
            if (!instanceIndices.instancer.IsEmpty()) {
                TF_DEBUG(USDIMAGING_SELECTION).Msg(
                    "                Protoype %s is instanced by instancer %s "
                    "(as its %d. prototype)\n"
                    "                    Instance indices: %s\n",
                    path.GetText(),
                    instanceIndices.instancer.GetText(),
                    instanceIndices.prototypeIndex,
                    TfStringify(instanceIndices.instanceIndices).c_str());
            }
        }

        path = instanceIndices.instancer;

        if (path.IsEmpty()) {
            std::reverse(result.begin(), result.end());
            return result;
        }
        result.push_back(std::move(instanceIndices));
    }
    // Make compiler happy.
    return {};
}

std::vector<_PrimAndNestedInstanceIndices>
_ComputePiPropagatedPathsAndNestedInstanceIndices(
    HdContainerDataSourceHandle const &primSource,
    HdSceneIndexBaseRefPtr const &sceneIndex)
{
    std::vector<_PrimAndNestedInstanceIndices> result;

    UsdImagingUsdPrimInfoSchema schema =
        UsdImagingUsdPrimInfoSchema::GetFromParent(primSource);
    HdContainerDataSourceHandle propagatedPrototypesDs =
        schema.GetPiPropagatedPrototypes();
    if (!propagatedPrototypesDs) {
        return result;
    }

    for (const TfToken &name : propagatedPrototypesDs->GetNames()) {
        HdPathDataSourceHandle const propagatedPrototypeDs =
            HdPathDataSource::Cast(propagatedPrototypesDs->Get(name));
        if (!propagatedPrototypeDs) {
            continue;
        }

        const SdfPath &propagatedPrototype =
            propagatedPrototypeDs->GetTypedValue(0.0f);

        TF_DEBUG(USDIMAGING_SELECTION).Msg(
            "            Prim is PI prototype with propagated copy at %s\n",
            propagatedPrototype.GetText());

        result.push_back(
            { propagatedPrototype,
              _ComputeAllNestedInstanceIndicesForPrototype(
                  propagatedPrototype, sceneIndex)});
    }

    return result;
}

// Compute prototype path and a container data source conforming to
// HdInstanceIndicesSchema given the instance schema of an instance prim
// and the instancer topology schema of the corresponding instancer
// prim.
std::pair<SdfPath, _InstanceIndices>
_ComputeNiPrototypePathAndInstanceIndices(
    HdInstanceSchema &instance,
    HdInstancerTopologySchema &instancerTopology)
{
    SdfPath prototypePath;

    // Set instancer path.
    _InstanceIndices result;
    if (HdPathDataSourceHandle const instancerDs = instance.GetInstancer()) {
        result.instancer = instancerDs->GetTypedValue(0.0f);
    }

    if (HdIntDataSourceHandle const prototypeIndexDs =
                    instance.GetPrototypeIndex()) {

        // Set prototype id, the index into the prototypes of the instancer.
        result.prototypeIndex = prototypeIndexDs->GetTypedValue(0.0f);

        // Use the index to get the prototype path from the instancer.
        prototypePath = _GetPath(
            instancerTopology.GetPrototypes(),
            result.prototypeIndex);
    }

    if (HdIntDataSourceHandle const instanceIndexDs =
                    instance.GetInstanceIndex()) {
        // Note that an instance has a unique instance index, but
        // HdInstanceIndicesSchema can have a list of indices.
        // So we need to wrap it.
        result.instanceIndices = { instanceIndexDs->GetTypedValue(0.0f) };
    }

    return { prototypePath, result };
}

// Check whether the prim at the given path is an instance.
// If yes, return the prototype path and a container data source
// conforming to HdInstanceIndicesSchema.
std::pair<SdfPath, _InstanceIndices>
_ComputeNiPrototypePathAndInstanceIndices(
    HdContainerDataSourceHandle const &primSource,
    HdSceneIndexBaseRefPtr const &sceneIndex)
{
    // Extract instance information.
    HdInstanceSchema instanceSchema =
      HdInstanceSchema::GetFromParent(primSource);

    HdPathDataSourceHandle const instancerPathDs = instanceSchema.GetInstancer();
    if (!instancerPathDs) {
        return { SdfPath(), { SdfPath(), 0, {}} };
    }

    // Extract information of instancer realizing this instance.
    const SdfPath instancerPath = instancerPathDs->GetTypedValue(0.0f);
    HdInstancerTopologySchema instancerTopologySchema =
        HdInstancerTopologySchema::GetFromParent(
            sceneIndex->GetPrim(instancerPath).dataSource);

    return _ComputeNiPrototypePathAndInstanceIndices(
        instanceSchema, instancerTopologySchema);
}

bool
_TranslatePathToNiPrototype(
    HdContainerDataSourceHandle const &primSource,
    HdSceneIndexBaseRefPtr const &sceneIndex,
    _PrimAndNestedInstanceIndices * const primAndInstanceIndices)
{
    SdfPath prototypePath;
    _InstanceIndices instanceIndices;
    std::tie(prototypePath, instanceIndices) =
        _ComputeNiPrototypePathAndInstanceIndices(
            primSource,
            sceneIndex);
    
    if (prototypePath.IsEmpty()) {
        return false;
    }

    TF_DEBUG(USDIMAGING_SELECTION).Msg(
        "            Prim is USD native instance of prototype %s\n",
        prototypePath.GetText());
    
    // If we hit an instance, we need to replace the path to
    // the prototype (in the scene index) that this instance
    // is instancing. More precisely, the prototype that
    // was added by the prototype propagating scene index after
    // instancing aggregation.
    primAndInstanceIndices->prim = std::move(prototypePath);

    if (!instanceIndices.instancer.IsEmpty()) {
        TF_DEBUG(USDIMAGING_SELECTION).Msg(
            "                Instancer: %s\n"
            "                Instance indices: %s\n",
            instanceIndices.instancer.GetText(),
            TfStringify(instanceIndices.instanceIndices).c_str());

        // If we hit an instance, record the instancing info such
        // as what instancer was added by instance aggregation to
        // realize this instance and what the instance index within
        // that instancer is.
        primAndInstanceIndices->nestedInstanceIndices.push_back(
            std::move(instanceIndices));
    }

    TF_DEBUG(USDIMAGING_SELECTION).Msg(
        "            New path: %s\n",
        primAndInstanceIndices->prim.GetText());
 
    return true;
}

_InstanceIndicesVector
_Concat(const _InstanceIndicesVector &a,
        const _InstanceIndicesVector &b)
{
    _InstanceIndicesVector result = a;
    result.insert(result.end(), b.begin(), b.end());
    return result;
}

// If we hit a prototype instanced by a point instancer, the propagating
// scene index will have made a copy of the prototype. We need to select
// this copy as well. Add the copy to the entries.
void
_AddPiPropagatedPrototypes(
    HdContainerDataSourceHandle const &primSource,
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const _PrimAndNestedInstanceIndices &primAndInstanceIndices,
    std::vector<_PrimAndNestedInstanceIndices> * const newPrimAndInstanceIndices)
{
    const _InstanceIndicesVector oldIndices =
        primAndInstanceIndices.nestedInstanceIndices;

    const std::vector<_PrimAndNestedInstanceIndices> propagatedPathsAndIndices =
        _ComputePiPropagatedPathsAndNestedInstanceIndices(
            primSource, sceneIndex);

    for (auto &propagatedPathAndIndices : propagatedPathsAndIndices) {

        TF_DEBUG(USDIMAGING_SELECTION).Msg(
            "            New path: %s\n",
            propagatedPathAndIndices.prim.GetText());

        newPrimAndInstanceIndices->push_back(
            { propagatedPathAndIndices.prim,
              _Concat(oldIndices,
                      propagatedPathAndIndices.nestedInstanceIndices)});
    }             
}       

// Given the instancing information to select a prim at
// primAndInstanceIndices.prim, compute the information to select
// the child at primName.
void
_AppendNameToSceneIndexPrimsAndInstanceIndices(
    _PrimAndNestedInstanceIndices primAndInstanceIndices,
    const TfToken &primName,
    HdSceneIndexBaseRefPtr const &sceneIndex,
    std::vector<_PrimAndNestedInstanceIndices> * const result)
{
    SdfPath &prim = primAndInstanceIndices.prim;
    prim = prim.AppendChild(primName);

    HdContainerDataSourceHandle const primSource =
        sceneIndex->GetPrim(prim).dataSource;

    TF_DEBUG(USDIMAGING_SELECTION).Msg(
        "            Scene index prim to process: %s\n",
        prim.GetText());
    
    if (!_TranslatePathToNiPrototype(
            primSource,
            sceneIndex,
            &primAndInstanceIndices)) {
        _AddPiPropagatedPrototypes(
            primSource,
            sceneIndex,
            primAndInstanceIndices,
            result);
    }
    result->push_back(std::move(primAndInstanceIndices));
}

// Given a usd proxy path, computes the corresponding paths in the scene index
// and the necessary instancing information.
//
// Example 1:
//
// def PointInstancer "MyInstancer"
// {
//     point3f[] positions = [(3, 0, 0), (-3, 0, 0)]
//     int[] protoIndices = [0, 0]
//     rel prototypes = [
//         </MyPrototype>
//     ]
// }
//
// def Xform "MyPrototype" {
//     def Cube "Cube"
//     {
//     }
// }
//
// The NI and PI prototype propagating scene indices turn this into:
//
// /MyInstancer
//     primType: instancer
//     dataSource:
//         instancerTopology:
//             prototypes: [ /MyPrototypes/ForInstancer52...82 ]
//             ...
//         ...
// /MyPrototype
//     primType: ""
//     dataSource:
//         __usdPrimInfo:
//             piPropagatedPrototypes:
//                 ForInstacner52...82: /MyPrototype/ForInstancer52...82
//             ...
//         ...
// /MyPrototype/Cube
//     primType: cube
// /MyPrototype/ForInstancer52...82
//     primType: ""
//     dataSource:
//         instancedBy:
//             paths: [/MyInstancer]
//             prototypeRoots: [/MyPrototype/ForInstancer52...82]
//         ...
// /MyPrototype/ForInstancer52...82/Cube
//     primType: cube
//     dataSource:
//         instancedBy:
//             paths: [/MyInstancer]
//             prototypeRoots: [/MyPrototype/ForInstancer52...82]
//         ...
//
// Assume that the UsdPrim /MyPrototype/Cube was selected.
//
// We start with /MyPrototype. Note that it has piPropagatedPrototypes
// information. Thus, at this point,
// std::vector<_PrimAndNestedInstanceIndice> contains the following selection
// information:
// prim: /MyPrototype:
//     nestedInstanceIndices: []
// prim: /MyPrototype/ForInstancer52...82:
//     nestedInstanceIndices: [
//         instancer: /MyInstancer
//         prototypeIndex: 0
//         instanceIndices: [ 0, 1 ]
//       ]
//
// We then process Cube from /MyPrototype/Cube and append it to the above
// selection information. Note that neither /MyPrototype/Cube nor
// /MyPrototype/ForInstancer52...82/Cube has any piPropagatedPrototypes
// information, so this just becomes:
//
// prim: /MyPrototype/Cube:
//     nestedInstanceIndices: []
// prim: /MyPrototype/ForInstancer52...82/Cube:
//     nestedInstanceIndices: [
//         instancer: /MyInstancer
//         prototypeIndex: 0
//         instance indices: [ 0, 1 ]
//       ]
// 
//
// Example 2:
//
// def Xform "MyPrototype"
// {
//    def Cube "cube"
//    {
//    }
// }
//
// def Xform "Instance1"
// (
//    instanceable = true
//    references = </MyPrototype>
// )
// {
//    double3 xformOp:translate = (0, 4, 0)
//    uniform token[] xformOpOrder = ["xformOp:translate"]
// }
//
// The NI and PI prototype propagating scene indices turn this into:
//
// /Instance1
//     primType: ""
//     dataSource:
//         instance:
//             instancer: /UsdNiPropagatedPrototypes/NoPrimvars_NoMaterialBindings/__Prototype_1/UsdNiInstancer
//             prototypeIndex: 0
//             instanceIndex: 0
//         ...
// /UsdNiPropagatedPrototypes
// /UsdNiPropagatedPrototypes/NoPrimvars_NoMaterialBindings
// /UsdNiPropagatedPrototypes/NoPrimvars_NoMaterialBindings/__Prototype_1
//     primType: ""
// /UsdNiPropagatedPrototypes/NoPrimvars_NoMaterialBindings/__Prototype_1/UsdNiInstancer
//     primType: instancer
//     dataSource:
//         instancerTopology:
//             instanceLocations: [ /Instance1 ]
//             prototypes: [ /UsdNiPropagatedPrototypes/NoPrimvars_NoMaterialBindings/__Prototype_1/UsdNiInstancer/UsdNiPrototype ]
//             ...
//         ...
// /UsdNiPropagatedPrototypes/NoPrimvars_NoMaterialBindings/__Prototype_1/UsdNiInstancer/UsdNiPrototype
//     primType: ""
// /UsdNiPropagatedPrototypes/NoPrimvars_NoMaterialBindings/__Prototype_1/UsdNiInstancer/UsdNiPrototype/cube
//     primType: cube
//
// Assume we select /Instance1/Cube in USD.
//
// We start with /Instance1. The prim at that path has an instance data source.
// Thus, /Instance1 is translated into the following
// std::vector<_PrimAndNestedInstanceIndice>:
//
// prim: /UsdNiPropagatedPrototypes/NoPrimvars_NoMaterialBindings/__Prototype_1/UsdNiInstancer/UsdNiPrototype
//     nestedInstanceIndices: [
//         instancer: /UsdNiPropagatedPrototypes/NoPrimvars_NoMaterialBindings/__Prototype_1/UsdNiInstancer
//         prototypeIndex: 0
//         instanceIndices: [ 0 ]
//
// Next, we process Cube from /Instance1/Cube. Since
// /UsdNiPropagatedPrototypes/NoPrimvars_NoMaterialBindings/__Prototype_1/UsdNiInstancer/UsdNiPrototype/cube
// has no pi propagated or instance data source, we simply append "Cube":
//
// prim: /UsdNiPropagatedPrototypes/NoPrimvars_NoMaterialBindings/__Prototype_1/UsdNiInstancer/UsdNiPrototype/Cube
//     nestedInstanceIndices: [
//         instancer: /UsdNiPropagatedPrototypes/NoPrimvars_NoMaterialBindings/__Prototype_1/UsdNiInstancer
//         prototypeIndex: 0
//         instanceIndices: [ 0 ]
//
std::vector<_PrimAndNestedInstanceIndices>
_ComputeSceneIndexPrimsAndInstanceIndices(
    const SdfPath &usdPath, HdSceneIndexBaseRefPtr const &sceneIndex)
{
    static const _PrimAndNestedInstanceIndices root{
        SdfPath::AbsoluteRootPath(), {}};

    std::vector<_PrimAndNestedInstanceIndices> result = { root };

    // Iterate through elements of path and build up path in scene index,
    // replacing the path if we hit a native instance.
    for (const SdfPath &usdPrefix : usdPath.GetPrefixes()) {
        const TfToken primName = usdPrefix.GetNameToken();

        TF_DEBUG(USDIMAGING_SELECTION).Msg(
            "    Processing next path element of usdPath: %s\n",
            primName.GetText());

        std::vector<_PrimAndNestedInstanceIndices> newResult;

        for (const _PrimAndNestedInstanceIndices &primAndIndices : result) {
            _AppendNameToSceneIndexPrimsAndInstanceIndices(
                primAndIndices, primName, sceneIndex, &newResult);
        }

        result = std::move(newResult);
    }

    TF_DEBUG(USDIMAGING_SELECTION).Msg(
        "    Traversing descendants of usdPath %s\n", usdPath.GetText());

    // Now add all namespace descendants of the paths we determined.
    size_t i = 0;
    while (i < result.size()) {
        TF_DEBUG(USDIMAGING_SELECTION).Msg(
            "        Adding children of scene index prim %s\n",
            result[i].prim.GetText());
        for (const SdfPath &child :
                 sceneIndex->GetChildPrimPaths(result[i].prim)) {
            _AppendNameToSceneIndexPrimsAndInstanceIndices(
                result[i], child.GetNameToken(), sceneIndex, &result);
        }
        ++i;
    }

    return result;
}

}

using namespace UsdImagingSelectionSceneIndex_Impl;

UsdImagingSelectionSceneIndexRefPtr
UsdImagingSelectionSceneIndex::New(
    HdSceneIndexBaseRefPtr const &inputSceneIndex)
{
    return TfCreateRefPtr(
        new UsdImagingSelectionSceneIndex(
            inputSceneIndex));
}

UsdImagingSelectionSceneIndex::
UsdImagingSelectionSceneIndex(
        HdSceneIndexBaseRefPtr const &inputSceneIndex)
  : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
  , _selectionInfo(std::make_shared<_SelectionInfo>())
{
}

UsdImagingSelectionSceneIndex::~UsdImagingSelectionSceneIndex() = default;

HdSceneIndexPrim
UsdImagingSelectionSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    HdSceneIndexPrim result = _GetInputSceneIndex()->GetPrim(primPath);
    if (!result.dataSource) {
        return result;
    }

    result.dataSource = _PrimSource::New(
        result.dataSource, _selectionInfo, primPath);

    return result;
}

SdfPathVector
UsdImagingSelectionSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
UsdImagingSelectionSceneIndex::AddSelection(
    const SdfPath &usdPath)
{
    TRACE_FUNCTION();

    TF_DEBUG(USDIMAGING_SELECTION).Msg(
        "UsdImagingSelectionSceneInedx::AddSelection(usdPath = %s)\n",
        usdPath.GetText());

    // From the USD path, compute the corresponding selections for 
    // the paths in the scene index - taking into account, e.g.,
    // that selecting a native instance in USD should translate into
    // selecting the prototype and its descendants and add to the
    // selections schema the index of the native instance.
    const std::vector<_PrimAndNestedInstanceIndices> primsAndIndices =
        _ComputeSceneIndexPrimsAndInstanceIndices(
            usdPath, _GetInputSceneIndex());

    SdfPathSet dirtiedPrims;
    
    for (const _PrimAndNestedInstanceIndices &primAndIndices : primsAndIndices) {
        const SdfPath &prim = primAndIndices.prim;
        _Selections &selections = _selectionInfo->primToSelections[prim];
        selections.push_back({ primAndIndices.nestedInstanceIndices });
        dirtiedPrims.insert(prim);
    }

    if (dirtiedPrims.empty()) {
        return;
    }
    
    HdSceneIndexObserver::DirtiedPrimEntries entries;
    entries.reserve(dirtiedPrims.size());
    for (const SdfPath &dirtiedPrim : dirtiedPrims) {
        static const HdDataSourceLocatorSet locators{
            HdSelectionsSchema::GetDefaultLocator()};
        entries.push_back({dirtiedPrim, locators});
    }
    _SendPrimsDirtied(entries);
}

void
UsdImagingSelectionSceneIndex::ClearSelection()
{
    TRACE_FUNCTION();

    TF_DEBUG(USDIMAGING_SELECTION).Msg(
        "UsdImagingSelectionSceneIndex::ClearSelection()\n");

    if (_selectionInfo->primToSelections.empty()) {
        return;
    }

    HdSceneIndexObserver::DirtiedPrimEntries entries;
    if (_IsObserved()) {
        entries.reserve(_selectionInfo->primToSelections.size());
        for (const auto &primAndSelections : _selectionInfo->primToSelections) {
            static const HdDataSourceLocatorSet locators{
                HdSelectionsSchema::GetDefaultLocator()};
            entries.emplace_back(primAndSelections.first, locators);
        }
    }

    _selectionInfo->primToSelections.clear();

    if (!entries.empty()) {
        _SendPrimsDirtied(entries);
    }
}

void
UsdImagingSelectionSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    _SendPrimsAdded(entries);
}

void
UsdImagingSelectionSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    _SendPrimsDirtied(entries);
}

static
void
_DeletePrefix(const SdfPath &prefix,
              std::map<SdfPath, _Selections> * const m)
{
    auto it = m->lower_bound(prefix);
    while (it != m->end() && it->first.HasPrefix(prefix)) {
        it = m->erase(it);
    }
}

void
UsdImagingSelectionSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    TRACE_FUNCTION();

    if (!_selectionInfo->primToSelections.empty()) {
        TRACE_SCOPE("Deleting prefixes");
        for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
            _DeletePrefix(
                entry.primPath,
                std::addressof(_selectionInfo->primToSelections));
        }
    }

    _SendPrimsRemoved(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
