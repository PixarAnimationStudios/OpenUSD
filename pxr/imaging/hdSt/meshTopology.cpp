//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/imaging/hdSt/meshTopology.h"
#include "pxr/imaging/hdSt/quadrangulate.h"
#include "pxr/imaging/hdSt/subdivision.h"
#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/imaging/hdSt/triangulate.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"

#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/meshUtil.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

// Given an index builder buffer source and a face indices buffer source
// (containin face indices after triangulation/quadrangulation), this will 
// return a subset of the mesh indices that corresponds to those faces.
class HdSt_IndexSubsetComputation : public HdComputedBufferSource
{
public:
    HdSt_IndexSubsetComputation(
        HdBufferSourceSharedPtr indexBuilderSource,
        HdBufferSourceSharedPtr faceIndicesSource,
        HdBufferSourceSharedPtr baseFaceToRefinedFacesMapSource = nullptr);
    void GetBufferSpecs(HdBufferSpecVector *specs) const override;
    bool Resolve() override;

    bool HasChainedBuffer() const override;
    HdBufferSourceSharedPtrVector GetChainedBuffers() const override;

protected:
    bool _CheckValid() const override;
    VtIntArray _ComputeProcessedFaceIndices() const;
    void _ResolveIndices(VtIntArray const &faceIndices);
    void _PopulateChainedBuffers(VtIntArray const &faceIndices);
    HdBufferSourceSharedPtrVector _chainedBuffers;
    HdBufferSourceSharedPtr _indexBuilderSource;
    HdBufferSourceSharedPtr _faceIndicesSource;
    HdBufferSourceSharedPtr _baseFaceToRefinedFacesMapSource;
    HdSt_MeshTopology *_topology;
};

// Will map a geom subset's authored face indices to the appropriate 
// triangulated/quadrangulated face indices. This buffer source is also used in
// drawing as the unrefined fvar indices.
class HdSt_GeomSubsetFaceIndexBuilderComputation : 
    public HdComputedBufferSource
{
public:
    HdSt_GeomSubsetFaceIndexBuilderComputation(
        HdBufferSourceSharedPtr geomSubsetFaceIndexHelperSource,
        VtIntArray const &faceIndices);
    void GetBufferSpecs(HdBufferSpecVector *specs) const override;
    bool Resolve() override;

protected:
    bool _CheckValid() const override;
    HdBufferSourceSharedPtr _geomSubsetFaceIndexHelperSource;
    VtIntArray _faceIndices;
};

// Creates two buffer sources  to be used as input into 
// HdSt_GeomSubsetFaceIndexBuilderComputation. The first of these buffer sources
// contains the number of triangulated/quadrangulated faces created per base 
// face, as each base face can become multiple faces after 
// triangulation/quadrangulation. The second buffer source contains the 
// starting face index of the triangulated/quadrangulated faces for each base 
// face. 
class HdSt_GeomSubsetFaceIndexHelperComputation : 
    public HdComputedBufferSource
{
public:
    HdSt_GeomSubsetFaceIndexHelperComputation(
        HdSt_MeshTopology *topology,
        bool refined, 
        bool quadrangulated);
    void GetBufferSpecs(HdBufferSpecVector *specs) const override;
    bool Resolve() override;
    bool HasChainedBuffer() const override;
    HdBufferSourceSharedPtrVector GetChainedBuffers() const override;

protected:
    bool _CheckValid() const override;
    HdSt_MeshTopology *_topology;
    bool _refined;
    bool _quadrangulated;
    HdBufferSourceSharedPtr _processedFaceIndicesBuffer;
};

// static
HdSt_MeshTopologySharedPtr
HdSt_MeshTopology::New(
        const HdMeshTopology &src,
        int refineLevel,
        RefineMode refineMode,
        QuadsMode quadsMode)
{
    return HdSt_MeshTopologySharedPtr(
        new HdSt_MeshTopology(src, refineLevel, refineMode, quadsMode));
}

// explicit
HdSt_MeshTopology::HdSt_MeshTopology(
        const HdMeshTopology& src,
        int refineLevel,
        RefineMode refineMode,
        QuadsMode quadsMode)
 : HdMeshTopology(src, refineLevel)
 , _quadsMode(quadsMode)
 , _quadInfo(nullptr)
 , _quadrangulateTableRange()
 , _quadInfoBuilder()
 , _refineMode(refineMode)
 , _subdivision(nullptr)
 , _osdTopologyBuilder()
 , _osdBaseFaceToRefinedFacesMap()
 , _nonSubsetFaces(nullptr)
{
    SanitizeGeomSubsets();
}

HdSt_MeshTopology::~HdSt_MeshTopology() = default;

bool
HdSt_MeshTopology::operator==(HdSt_MeshTopology const &other) const {

    TRACE_FUNCTION();

    // no need to compare _adajency and _quadInfo
    return HdMeshTopology::operator==(other);
}

void
HdSt_MeshTopology::SetQuadInfo(HdQuadInfo const *quadInfo)
{
    _quadInfo.reset(quadInfo);
}

HdBufferSourceSharedPtr
HdSt_MeshTopology::GetPointsIndexBuilderComputation()
{
    // this is simple enough to return the result right away.
    int numPoints = GetNumPoints();
    VtIntArray indices(numPoints);
    for (int i = 0; i < numPoints; ++i) indices[i] = i;

    return std::make_shared<HdVtBufferSource>(
        HdTokens->indices, VtValue(indices));
}

HdBufferSourceSharedPtr
HdSt_MeshTopology::GetTriangleIndexBuilderComputation(SdfPath const &id)
{
    return std::make_shared<HdSt_TriangleIndexBuilderComputation>(this, id);
}

HdSt_QuadInfoBuilderComputationSharedPtr
HdSt_MeshTopology::GetQuadInfoBuilderComputation(
    bool gpu, SdfPath const &id, HdStResourceRegistry *resourceRegistry)
{
    HdSt_QuadInfoBuilderComputationSharedPtr builder =
        std::make_shared<HdSt_QuadInfoBuilderComputation>(this, id);

    // store as a weak ptr.
    _quadInfoBuilder = builder;

    if (gpu) {
        if (!TF_VERIFY(resourceRegistry)) {
            TF_CODING_ERROR("resource registry must be non-null "
                            "if gpu quadinfo is requested.");
            return builder;
        }

        HdBufferSourceSharedPtr quadrangulateTable =
            std::make_shared<HdSt_QuadrangulateTableComputation>(
                this, builder);

        // allocate quadrangulation table on GPU
        HdBufferSpecVector bufferSpecs;
        quadrangulateTable->GetBufferSpecs(&bufferSpecs);

        _quadrangulateTableRange =
            resourceRegistry->AllocateNonUniformBufferArrayRange(
                HdTokens->topology, bufferSpecs,
                HdBufferArrayUsageHintBitsStorage);

        resourceRegistry->AddSource(_quadrangulateTableRange, quadrangulateTable);
    }
    return builder;
}

HdBufferSourceSharedPtr
HdSt_MeshTopology::GetQuadIndexBuilderComputation(SdfPath const &id)
{
    return std::make_shared<HdSt_QuadIndexBuilderComputation>(
        this, _quadInfoBuilder.lock(), id);
}

HdBufferSourceSharedPtr
HdSt_MeshTopology::GetQuadrangulateComputation(
    HdBufferSourceSharedPtr const &source, SdfPath const &id)
{
    // check if the quad table is already computed as all-quads.
    if (_quadInfo && _quadInfo->IsAllQuads()) {
        // no need of quadrangulation.
        return HdBufferSourceSharedPtr();
    }

    // Make a dependency to quad info, in case if the topology
    // is chaging and the quad info hasn't been populated.
    //
    // It can be null for the second or later primvar animation.
    // Don't call GetQuadInfoBuilderComputation instead. It may result
    // unregisterd computation.
    HdBufferSourceSharedPtr quadInfo = _quadInfoBuilder.lock();

    return std::make_shared<HdSt_QuadrangulateComputation>(
        this, source, quadInfo, id);
}

HdStComputationSharedPtr
HdSt_MeshTopology::GetQuadrangulateComputationGPU(
    TfToken const &name, HdType dataType, SdfPath const &id)
{
    // check if the quad table is already computed as all-quads.
    if (_quadInfo && _quadInfo->IsAllQuads()) {
        // no need of quadrangulation.
        return nullptr;
    }
    return std::make_shared<HdSt_QuadrangulateComputationGPU>(
        this, name, dataType, id);
}

HdBufferSourceSharedPtr
HdSt_MeshTopology::GetQuadrangulateFaceVaryingComputation(
    HdBufferSourceSharedPtr const &source, SdfPath const &id)
{
    return std::make_shared<HdSt_QuadrangulateFaceVaryingComputation>(
        this, source, id);
}

HdBufferSourceSharedPtr
HdSt_MeshTopology::GetTriangulateFaceVaryingComputation(
    HdBufferSourceSharedPtr const &source, SdfPath const &id)
{
    return std::make_shared<HdSt_TriangulateFaceVaryingComputation>(
        this, source, id);
}

bool
HdSt_MeshTopology::RefinesToTriangles() const
{
    return HdSt_Subdivision::RefinesToTriangles(_topology.GetScheme());
}

bool
HdSt_MeshTopology::RefinesToBSplinePatches() const
{
    return ((IsEnabledAdaptive() || (_refineMode == RefineModePatches)) &&
            HdSt_Subdivision::RefinesToBSplinePatches(_topology.GetScheme()));
}

bool
HdSt_MeshTopology::RefinesToBoxSplineTrianglePatches() const
{
    return ((IsEnabledAdaptive() || (_refineMode == RefineModePatches)) &&
    HdSt_Subdivision::RefinesToBoxSplineTrianglePatches(_topology.GetScheme()));
}

HdBufferSourceSharedPtr
HdSt_MeshTopology::GetOsdTopologyComputation(SdfPath const &id)
{
    if (HdBufferSourceSharedPtr builder = _osdTopologyBuilder.lock()) {
        return builder;
    }

    // this has to be the first instance.
    if (!TF_VERIFY(!_subdivision)) return HdBufferSourceSharedPtr();

    bool const adaptive = RefinesToBSplinePatches() ||
                          RefinesToBoxSplineTrianglePatches();

    // create HdSt_Subdivision
    _subdivision = std::make_unique<HdSt_Subdivision>(adaptive, _refineLevel);

    if (!TF_VERIFY(_subdivision)) return HdBufferSourceSharedPtr();

    // create a topology computation for HdSt_Subdivision
    HdBufferSourceSharedPtr builder =
        _subdivision->CreateTopologyComputation(this, id);
    _osdTopologyBuilder = builder; // retain weak ptr
    return builder;
}

void
HdSt_MeshTopology::SanitizeGeomSubsets()
{
    const HdGeomSubsets &geomSubsets = GetGeomSubsets();
    if (geomSubsets.empty()) {
        return;
    }
    const size_t numFaces = GetNumFaces();

    // Keep track of faces that are used within the geom subsets
    std::vector<bool> unusedFaces(numFaces, true);
    size_t numUnusedFaces = numFaces;

    HdGeomSubsets sanitizedGeomSubsets;
    for (const HdGeomSubset &geomSubset : geomSubsets) {
        HdGeomSubset sanitizedGeomSubset = geomSubset;

        // We only care about subsets that will with non-empty indices and
        // material id
        const VtIntArray faceIndices = geomSubset.indices;
        if (!faceIndices.empty() && !geomSubset.materialId.IsEmpty()) {                    
            VtIntArray sanitizedFaceIndices;
            for (size_t i = 0; i < faceIndices.size(); ++i) {
                const int index = faceIndices[i];
                // Skip out-of-bound face indices.
                if (index >= (int)numFaces) {
                    TF_WARN("Geom subset index %d is larger than number of "
                        "faces (%d), removing.", index, (int)numFaces);
                    continue;
                }
                sanitizedFaceIndices.push_back(index);
                if (unusedFaces[index]) {
                    unusedFaces[index] = false;
                    numUnusedFaces--;
                } else {
                    // Warn about duplicated face indices.
                    TF_WARN("Face index %d is repeated between geom subsets", 
                        index);;
                }
            }
            sanitizedGeomSubset.indices = sanitizedFaceIndices;
            sanitizedGeomSubsets.push_back(sanitizedGeomSubset);
        }
    }

    _nonSubsetFaces = std::make_unique<std::vector<int>>();
    _nonSubsetFaces->resize(numUnusedFaces);

    if (numUnusedFaces) {
        size_t count = 0;
        for (size_t i = 0; i < unusedFaces.size() && count < numUnusedFaces; 
             ++i) {
            if (unusedFaces[i]) {
                (*_nonSubsetFaces)[count] = i;
                count++;
            }
        }
    }
    
    SetGeomSubsets(sanitizedGeomSubsets);
}

HdBufferSourceSharedPtr
HdSt_MeshTopology::GetOsdBaseFaceToRefinedFacesMapComputation(
    HdStResourceRegistry *resourceRegistry)
{   
    if (HdBufferSourceSharedPtr map = _osdBaseFaceToRefinedFacesMap.lock()) {
        return map;
    }

    if (!TF_VERIFY(_subdivision)) {
        return HdBufferSourceSharedPtr();
    }

    HdBufferSourceSharedPtr topologyBuilder = _osdTopologyBuilder.lock();
    HdBufferSourceSharedPtr map =
        _subdivision->CreateBaseFaceToRefinedFacesMapComputation(
            topologyBuilder);
    // Add to resource registry when created
    resourceRegistry->AddSource(map);

    _osdBaseFaceToRefinedFacesMap = map; // retain weak ptr
    return map;
}

HdBufferSourceSharedPtr
HdSt_MeshTopology::GetIndexSubsetComputation(
    HdBufferSourceSharedPtr indexBuilderSource,
    HdBufferSourceSharedPtr faceIndicesSource)
{
    return std::make_shared<HdSt_IndexSubsetComputation>(
        indexBuilderSource, faceIndicesSource);
}

HdBufferSourceSharedPtr
HdSt_MeshTopology::GetRefinedIndexSubsetComputation(
    HdBufferSourceSharedPtr indexBuilderSource, 
    HdBufferSourceSharedPtr faceIndicesSource)
{
    HdBufferSourceSharedPtr baseFaceToRefinedFacesMapSource = 
        _osdBaseFaceToRefinedFacesMap.lock();

    return std::make_shared<HdSt_IndexSubsetComputation>(
        indexBuilderSource, faceIndicesSource, baseFaceToRefinedFacesMapSource);
}

HdBufferSourceSharedPtr
HdSt_MeshTopology::GetGeomSubsetFaceIndexHelperComputation(
    bool refined, bool quadrangulated)
{
    return std::make_shared<HdSt_GeomSubsetFaceIndexHelperComputation>(
        this, refined, quadrangulated);
}

HdBufferSourceSharedPtr
HdSt_MeshTopology::GetGeomSubsetFaceIndexBuilderComputation(
    HdBufferSourceSharedPtr geomSubsetFaceIndexHelperSource, 
    VtIntArray const &faceIndices) 
{
    return std::make_shared<HdSt_GeomSubsetFaceIndexBuilderComputation>(
        geomSubsetFaceIndexHelperSource, faceIndices);
}

HdBufferSourceSharedPtr
HdSt_MeshTopology::GetOsdIndexBuilderComputation()
{
    HdBufferSourceSharedPtr topologyBuilder = _osdTopologyBuilder.lock();
    return _subdivision->CreateIndexComputation(this, topologyBuilder);
}

HdBufferSourceSharedPtr 
HdSt_MeshTopology::GetOsdFvarIndexBuilderComputation(int channel)
{
    HdBufferSourceSharedPtr topologyBuilder = _osdTopologyBuilder.lock();
    return _subdivision->CreateFvarIndexComputation(
        this, topologyBuilder, channel);
}

HdBufferSourceSharedPtr
HdSt_MeshTopology::GetOsdRefineComputation(HdBufferSourceSharedPtr const &source,
                                           Interpolation interpolation,
                                           int fvarChannel)
{
    // Make a dependency to osd topology builder computation.
    // (see comment on GetQuadrangulateComputation)
    //
    // It can be null for the second or later primvar animation.
    // Don't call GetOsdTopologyComputation instead. It may result
    // unregisterd computation.

    // for empty topology, we don't need to refine anything.
    // source will be scheduled at the caller
    if (_topology.GetFaceVertexCounts().size() == 0) return source;

    if (!TF_VERIFY(_subdivision)) {
        TF_CODING_ERROR("GetOsdTopologyComputation should be called before "
                        "GetOsdRefineComputation.");
        return source;
    }

    HdBufferSourceSharedPtr topologyBuilder = _osdTopologyBuilder.lock();

    return _subdivision->CreateRefineComputationCPU(this, source,
                                                    topologyBuilder, 
                                                    interpolation);
}

HdStComputationSharedPtr
HdSt_MeshTopology::GetOsdRefineComputationGPU(
    TfToken const &name,
    HdType dataType,
    HdStResourceRegistry *resourceRegistry,
    Interpolation interpolation,
    int fvarChannel)
{
    // Make a dependency to osd topology builder computation.
    // (see comment on GetOsdRefineComputation)

    // for empty topology, we don't need to refine anything.
    if (_topology.GetFaceVertexCounts().size() == 0) return nullptr;

    if (!TF_VERIFY(_subdivision)) {
        TF_CODING_ERROR("GetOsdTopologyComputation should be called before "
                        "GetOsdRefineComputationGPU.");
        return nullptr;
    }

    HdBufferSourceSharedPtr topologyBuilder = _osdTopologyBuilder.lock();
    
    return _subdivision->CreateRefineComputationGPU(this, topologyBuilder,
                                                    name, dataType,
                                                    resourceRegistry,
                                                    interpolation, fvarChannel);
}

HdSt_IndexSubsetComputation::HdSt_IndexSubsetComputation(
    HdBufferSourceSharedPtr indexBuilderSource,
    HdBufferSourceSharedPtr faceIndicesSource,
    HdBufferSourceSharedPtr baseFaceToRefinedFacesMapSource) : 
    _indexBuilderSource(indexBuilderSource),
    _faceIndicesSource(faceIndicesSource),
    _baseFaceToRefinedFacesMapSource(baseFaceToRefinedFacesMapSource)
{
}
    
void
HdSt_IndexSubsetComputation::GetBufferSpecs(HdBufferSpecVector *specs) const 
{
    return _indexBuilderSource->GetBufferSpecs(specs);
}

bool
HdSt_IndexSubsetComputation::Resolve() 
{
    if (_indexBuilderSource && !_indexBuilderSource->IsResolved()) return false;
    if (_faceIndicesSource && !_faceIndicesSource->IsResolved()) return false;
    if (_baseFaceToRefinedFacesMapSource && 
        !_baseFaceToRefinedFacesMapSource->IsResolved()) {
        return false;
    }

    if (!_TryLock()) return false;

    HD_TRACE_FUNCTION();

    VtIntArray faceIndices;
    if (_faceIndicesSource) {
        const int32_t * const processedFaceIndices = 
            reinterpret_cast<const int32_t*>(_faceIndicesSource->GetData());
        const size_t numElements = _faceIndicesSource->GetNumElements();
        faceIndices.resize(numElements);
        memcpy(faceIndices.data(), processedFaceIndices, 
            numElements * sizeof(int32_t));
    }

    // Refined indices need extra step to map the quadrangulated/triangulated
    // face indices to the refined face indices.
    if (_baseFaceToRefinedFacesMapSource) {
        const int32_t * const baseFaceToRefinedFacesMap = 
            reinterpret_cast<const int32_t*>(
                _baseFaceToRefinedFacesMapSource->GetData());
        const int32_t * const refinedFaceCounts = 
            reinterpret_cast<const int32_t*>(
                _baseFaceToRefinedFacesMapSource->GetChainedBuffers().front()
                    ->GetData());

        VtIntArray refinedFaceIndices;
        for (size_t i = 0; i < faceIndices.size(); ++i) {
            const size_t start = faceIndices[i] == 0 ? 
                0 : refinedFaceCounts[faceIndices[i] - 1];
            const size_t end = refinedFaceCounts[faceIndices[i]];

            for (size_t j = start; j < end; ++j) {
                refinedFaceIndices.push_back(baseFaceToRefinedFacesMap[j]);
            }
        }
        faceIndices = refinedFaceIndices;
    }

    _ResolveIndices(faceIndices);
    _PopulateChainedBuffers(faceIndices);

    _SetResolved();
    return true;
}

bool 
HdSt_IndexSubsetComputation::HasChainedBuffer() const 
{
    return !_chainedBuffers.empty();
}

HdBufferSourceSharedPtrVector 
HdSt_IndexSubsetComputation::GetChainedBuffers() const 
{
    return _chainedBuffers;
}

bool 
HdSt_IndexSubsetComputation::_CheckValid() const 
{
    return true;
}

void 
HdSt_IndexSubsetComputation::_ResolveIndices(VtIntArray const &faceIndices) 
{
    const size_t numFaces = faceIndices.size();
    const int32_t * const indices = reinterpret_cast<const int32_t*>(
        _indexBuilderSource->GetData());
    const HdTupleType tupleType = _indexBuilderSource->GetTupleType();
    const size_t arraySize = tupleType.count;

    // We assume indices of type HdTypeInt32 can come in arrays, while the other 
    // types do not.
    VtValue subsetIndices;
    switch (tupleType.type) {
        case HdTypeInt32: 
        {
            VtIntArray typedSubsetIndices;
            typedSubsetIndices.reserve(arraySize * numFaces);
            for (size_t i = 0; i < numFaces; ++i) {
                size_t index = arraySize * faceIndices[i];
                for (size_t j = 0; j < arraySize; ++j) {
                    typedSubsetIndices.push_back(indices[index + j]);
                }
            }
            subsetIndices = VtValue(typedSubsetIndices);
            break;
        }
        case HdTypeInt32Vec3: 
        {
            VtVec3iArray typedSubsetIndices(numFaces);
            for (size_t i = 0; i < numFaces; ++i) {
                size_t index = 3 * faceIndices[i];
                typedSubsetIndices[i] = GfVec3i(
                    indices[index], indices[index+1], indices[index+2]);
            }
            subsetIndices = VtValue(typedSubsetIndices);
            break;
        }
        case HdTypeInt32Vec4:
        {
            VtVec4iArray typedSubsetIndices(numFaces);
            for (size_t i = 0; i < numFaces; ++i) {
                size_t index = 4 * faceIndices[i];
                typedSubsetIndices[i] = GfVec4i(
                    indices[index], indices[index+1], indices[index+2], 
                        indices[index+3]);
            }
            subsetIndices = VtValue(typedSubsetIndices);
            break;
        }
        default:
            TF_WARN("%s indices type not supported",
                _indexBuilderSource->GetName().GetText());
    }

    _SetResult(std::make_shared<HdVtBufferSource>(
        _indexBuilderSource->GetName(), subsetIndices, arraySize));
}

void 
HdSt_IndexSubsetComputation::_PopulateChainedBuffers(
    VtIntArray const &faceIndices) 
{
    if (_indexBuilderSource->HasChainedBuffer()) {
        const size_t numFaces = faceIndices.size();

        HdBufferSourceSharedPtrVector chainedBuffers = 
            _indexBuilderSource->GetChainedBuffers();

        for (HdBufferSourceSharedPtr chainedBuffer : chainedBuffers) {
            const int32_t * const chainedBufferData = 
                reinterpret_cast<const int32_t*>(chainedBuffer->GetData());
            const HdTupleType tupleType = chainedBuffer->GetTupleType();
            
            // We assume the chained buffers of the index builder comps all
            // have an array size of 1.
            VtValue subsetChainedBuffer;
            switch (tupleType.type) {
                case HdTypeInt32: 
                {
                    VtIntArray typedSubsetChainedBuffer(numFaces);
                    for (size_t i = 0; i < numFaces; ++i) {
                        size_t index = faceIndices[i];
                        typedSubsetChainedBuffer[i] = chainedBufferData[index];
                    }
                    subsetChainedBuffer = VtValue(typedSubsetChainedBuffer);
                    break;
                } 
                case HdTypeInt32Vec2:
                {
                    VtVec2iArray typedSubsetChainedBuffer(numFaces);
                    for (size_t i = 0; i < numFaces; ++i) {
                        size_t index = 2 * faceIndices[i];
                        typedSubsetChainedBuffer[i] = GfVec2i(
                            chainedBufferData[index], 
                            chainedBufferData[index + 1]);
                    }
                    subsetChainedBuffer = VtValue(typedSubsetChainedBuffer);
                    break;
                }
                case HdTypeInt32Vec3:
                {
                    VtVec3iArray typedSubsetChainedBuffer(numFaces);
                    for (size_t i = 0; i < numFaces; ++i) {
                        size_t index = 3 * faceIndices[i];
                        typedSubsetChainedBuffer[i] = GfVec3i(
                            chainedBufferData[index], 
                            chainedBufferData[index + 1], 
                            chainedBufferData[index + 2]);
                    }
                    subsetChainedBuffer = VtValue(typedSubsetChainedBuffer);
                    break;
                }
                case HdTypeInt32Vec4:
                {
                    VtVec4iArray typedSubsetChainedBuffer(numFaces);
                    for (size_t i = 0; i < numFaces; ++i) {
                        size_t index = 4 * faceIndices[i];
                        typedSubsetChainedBuffer[i] = GfVec4i(
                            chainedBufferData[index], 
                            chainedBufferData[index + 1], 
                            chainedBufferData[index + 2], 
                            chainedBufferData[index + 3]);
                    }
                    subsetChainedBuffer = VtValue(typedSubsetChainedBuffer);
                    break;
                }
                default:
                    TF_WARN("Chained buffer %s type not supported", 
                        chainedBuffer->GetName().GetText());
            }

            HdBufferSourceSharedPtr subsetChainedBufferSource = 
                std::make_shared<HdVtBufferSource>(
                    chainedBuffer->GetName(),
                    subsetChainedBuffer);

            _chainedBuffers.push_back(subsetChainedBufferSource);
        }
    }
}

HdSt_GeomSubsetFaceIndexBuilderComputation::
    HdSt_GeomSubsetFaceIndexBuilderComputation(
        HdBufferSourceSharedPtr geomSubsetFaceIndexHelperSource, 
        VtIntArray const& faceIndices) : 
    _geomSubsetFaceIndexHelperSource(geomSubsetFaceIndexHelperSource), 
    _faceIndices(faceIndices)
{
}

void
HdSt_GeomSubsetFaceIndexBuilderComputation::GetBufferSpecs(
    HdBufferSpecVector *specs) const
{
    // Though this computation is used as the face indices input into the
    // subset indices computations, it is also used in drawing as the coarse
    // face index (in place of gl_PrimitiveID)
    specs->emplace_back(HdStTokens->coarseFaceIndex, 
        HdTupleType {HdTypeInt32, 1});
}

bool
HdSt_GeomSubsetFaceIndexBuilderComputation::Resolve()
{
    if (_geomSubsetFaceIndexHelperSource && 
        !_geomSubsetFaceIndexHelperSource->IsResolved()) {
        return false;
    }

    if (!_TryLock()) return false;

    HD_TRACE_FUNCTION();

    VtIntArray faceIndices;
    if (_geomSubsetFaceIndexHelperSource) {
        const int32_t * const processedFaceCounts = 
            reinterpret_cast<const int32_t*>(
                _geomSubsetFaceIndexHelperSource->GetData());
        const int32_t * const processedFaceIndices = 
            reinterpret_cast<const int32_t*>(
                _geomSubsetFaceIndexHelperSource->GetChainedBuffers().front()->
                    GetData());

        for (size_t i = 0; i < _faceIndices.size(); ++i) {
            const int baseFaceIndex = _faceIndices[i];
            for (int j = 0; j < processedFaceCounts[baseFaceIndex]; ++j) {
                faceIndices.push_back(processedFaceIndices[baseFaceIndex] + j);
            }
        }
    } 

    _SetResult(std::make_shared<HdVtBufferSource>(
        HdStTokens->coarseFaceIndex, VtValue(faceIndices)));

    _SetResolved();

    return true;
}

bool
HdSt_GeomSubsetFaceIndexBuilderComputation::_CheckValid() const
{
    return true;
}

HdSt_GeomSubsetFaceIndexHelperComputation::
    HdSt_GeomSubsetFaceIndexHelperComputation(
        HdSt_MeshTopology *topology,
        bool refined, 
        bool quadrangulated) :
    _topology(topology), 
    _refined(refined), 
    _quadrangulated(quadrangulated)
{
}

void 
HdSt_GeomSubsetFaceIndexHelperComputation::GetBufferSpecs(
    HdBufferSpecVector *specs) const 
{
    specs->emplace_back(
        HdStTokens->processedFaceCounts, HdTupleType {HdTypeInt32, 1});
    specs->emplace_back(
        HdStTokens->processedFaceIndices, HdTupleType {HdTypeInt32, 1});
}

bool 
HdSt_GeomSubsetFaceIndexHelperComputation::Resolve() 
{        
    if (!_TryLock()) return false;

    HD_TRACE_FUNCTION();

    const VtIntArray &faceVertexCounts = _topology->GetFaceVertexCounts();
    VtIntArray processedFaceCounts = VtIntArray(_topology->GetNumFaces());

    // Based on whether the mesh underwent a triangulation or quadrangulation
    // step, determine how many faces each base face becomes.
    if (_quadrangulated) {
        const VtIntArray &holeIndices = _topology->GetHoleIndices();
        size_t holeIndex = 0;
        for (int i = 0; i < _topology->GetNumFaces(); ++i) {
            if (holeIndex < holeIndices.size() && holeIndices[holeIndex] == i) {
                processedFaceCounts[i] = 0;
                holeIndex++;
            } else if (faceVertexCounts[i] == 4) {
                // Quad faces do not get quadrangulated
                processedFaceCounts[i] = 1;
            } else {
                processedFaceCounts[i] = faceVertexCounts[i];
            }
        }
    } else {
        const VtIntArray &holeIndices = _topology->GetHoleIndices();
        size_t holeIndex = 0;
        for (int i = 0; i < _topology->GetNumFaces(); ++i) {
            if (holeIndex < holeIndices.size() && holeIndices[holeIndex] == i) {
                processedFaceCounts[i] = 0;
                holeIndex++;
            } else {
                processedFaceCounts[i] = faceVertexCounts[i] - 2;
            }
        }
    }

    _SetResult(std::make_shared<HdVtBufferSource>(
        HdStTokens->processedFaceCounts, VtValue(processedFaceCounts)));

    // Using the number of processed faces per base face, determine the new face
    // index that each base face index maps to.
    // Each base face can potentially map to multiple processed faces, but this
    // value gives us the new starting index for those processed faces.
    VtIntArray processedFaceIndices = VtIntArray(_topology->GetNumFaces());
    int processedFaceIndex = 0;
    for (int i = 0; i < _topology->GetNumFaces(); ++i) {
        processedFaceIndices[i] = processedFaceIndex;

        // If current face is hole 
        if (_refined && processedFaceCounts[i] == 0) {
            if (_quadrangulated) {
                if (faceVertexCounts[i] == 4) {
                    processedFaceIndex += 1;
                } else {
                    processedFaceIndex += faceVertexCounts[i];
                }   
            } else {
                processedFaceIndex += faceVertexCounts[i] - 2;
            }
        } else {
            processedFaceIndex += processedFaceCounts[i];
        }      
    }

    _processedFaceIndicesBuffer.reset(new HdVtBufferSource(
        HdStTokens->processedFaceIndices, VtValue(processedFaceIndices)));

    _SetResolved();
    return true;
}

bool 
HdSt_GeomSubsetFaceIndexHelperComputation::HasChainedBuffer() const 
{
    return true;
}

HdBufferSourceSharedPtrVector 
HdSt_GeomSubsetFaceIndexHelperComputation::GetChainedBuffers() const 
{
    return { _processedFaceIndicesBuffer };    
}

bool
HdSt_GeomSubsetFaceIndexHelperComputation::_CheckValid() const
{
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE

