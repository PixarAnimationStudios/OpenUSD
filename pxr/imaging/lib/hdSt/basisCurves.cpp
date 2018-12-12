//
// Copyright 2016 Pixar
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
#include "pxr/pxr.h"
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hdSt/basisCurves.h"
#include "pxr/imaging/hdSt/basisCurvesComputations.h"
#include "pxr/imaging/hdSt/basisCurvesShaderKey.h"
#include "pxr/imaging/hdSt/basisCurvesTopology.h"
#include "pxr/imaging/hdSt/bufferArrayRangeGL.h"
#include "pxr/imaging/hdSt/drawItem.h"
#include "pxr/imaging/hdSt/extCompGpuComputation.h"
#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/imaging/hdSt/instancer.h"
#include "pxr/imaging/hdSt/material.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/vec2i.h"

#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/computation.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vertexAdjacency.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/base/vt/value.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

HdStBasisCurves::HdStBasisCurves(SdfPath const& id,
                 SdfPath const& instancerId)
    : HdBasisCurves(id, instancerId)
    , _topology()
    , _topologyId(0)
    , _customDirtyBitsInUse(0)
    , _refineLevel(0)
{
    /*NOTHING*/
}


HdStBasisCurves::~HdStBasisCurves()
{
    /*NOTHING*/
}

void
HdStBasisCurves::Sync(HdSceneDelegate *delegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits,
                      TfToken const   &reprToken)
{
    TF_UNUSED(renderParam);

    if (*dirtyBits & HdChangeTracker::DirtyMaterialId) {
        _SetMaterialId(delegate->GetRenderIndex().GetChangeTracker(),
                       delegate->GetMaterialId(GetId()));
    }

    // Check if either the material or geometric shaders need updating for
    // draw items of all the reprs.
    bool updateMaterialShader = false;
    if (*dirtyBits & (HdChangeTracker::DirtyMaterialId |
                      HdChangeTracker::NewRepr)) {
        updateMaterialShader = true;
    }

    bool updateGeometricShader = false;
    if (*dirtyBits & (HdChangeTracker::DirtyDisplayStyle |
                      HdChangeTracker::DirtyMaterialId |
                      HdChangeTracker::NewRepr)) {
        updateGeometricShader = true;
    }

    _UpdateRepr(delegate, reprToken, dirtyBits);

    if (updateMaterialShader || updateGeometricShader) {
        _UpdateShadersForAllReprs(delegate,
                                  updateMaterialShader, updateGeometricShader);
    }

    // This clears all the non-custom dirty bits. This ensures that the rprim
    // doesn't have pending dirty bits that add it to the dirty list every
    // frame.
    // XXX: GetInitialDirtyBitsMask sets certain dirty bits that aren't
    // reset (e.g. DirtyExtent, DirtyPrimID) that make this necessary.
    *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

void
HdStBasisCurves::_UpdateDrawItem(HdSceneDelegate *sceneDelegate,
                                 HdStDrawItem *drawItem,
                                 HdDirtyBits *dirtyBits,
                                 const HdBasisCurvesReprDesc &desc)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();

    /* VISIBILITY */
    _UpdateVisibility(sceneDelegate, dirtyBits);

    /* CONSTANT PRIMVARS, TRANSFORM AND EXTENT */
    HdPrimvarDescriptorVector constantPrimvars =
        GetPrimvarDescriptors(sceneDelegate, HdInterpolationConstant);
    _PopulateConstantPrimvars(sceneDelegate, drawItem, dirtyBits,
            constantPrimvars);

    /* INSTANCE PRIMVARS */
    if (!GetInstancerId().IsEmpty()) {
        HdStInstancer *instancer = static_cast<HdStInstancer*>(
            sceneDelegate->GetRenderIndex().GetInstancer(GetInstancerId()));
        if (TF_VERIFY(instancer)) {
            instancer->PopulateDrawItem(drawItem, &_sharedData, *dirtyBits);
        }
    }

    /* TOPOLOGY */
    // XXX: _PopulateTopology should be split into two phase
    //      for scene dirtybits and for repr dirtybits.
    if (*dirtyBits & (HdChangeTracker::DirtyTopology
                    | HdChangeTracker::DirtyDisplayStyle
                    | DirtyIndices
                    | DirtyHullIndices)) {
        _PopulateTopology(sceneDelegate, drawItem, dirtyBits, desc);
    }

    /* PRIMVAR */
    if (HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id)) {
        // XXX: curves don't use refined vertex primvars, however,
        // the refined renderpass masks the dirtiness of non-refined vertex
        // primvars, so we need to see refined dirty for updating coarse
        // vertex primvars if there is only refined reprs being updated.
        // we'll fix the change tracking in order to address this craziness.
        _PopulateVertexPrimvars(sceneDelegate, drawItem, dirtyBits);
        _PopulateElementPrimvars(sceneDelegate, drawItem, dirtyBits);
    }

    // Topology and VertexPrimvar may be null, if the curve has zero line
    // segments.
    TF_VERIFY(drawItem->GetConstantPrimvarRange());
}

static const char* HdSt_PrimTypeToString(HdSt_GeometricShader::PrimitiveType type){
    if (type == HdSt_GeometricShader::PrimitiveType::PRIM_BASIS_CURVES_LINES){
        return "lines";
    }
    if (type == HdSt_GeometricShader::PrimitiveType::PRIM_BASIS_CURVES_LINEAR_PATCHES){
        return "patches[linear]";
    }
    if (type == HdSt_GeometricShader::PrimitiveType::PRIM_BASIS_CURVES_CUBIC_PATCHES){
        return "patches[cubic]";
    }
    TF_WARN("Unknown type");
    return "unknown";
}

void
HdStBasisCurves::_UpdateDrawItemGeometricShader(
        HdSceneDelegate *sceneDelegate,
        HdStDrawItem *drawItem,
        const HdBasisCurvesReprDesc &desc)
{
    if (!TF_VERIFY(_topology)) return;

    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();

    HdSt_BasisCurvesShaderKey::DrawStyle drawStyle = 
        HdSt_BasisCurvesShaderKey::WIRE;
    HdSt_BasisCurvesShaderKey::NormalStyle normalStyle = 
        HdSt_BasisCurvesShaderKey::HAIR;

    TfToken curveType = _topology->GetCurveType();
    TfToken curveBasis = _topology->GetCurveBasis();

    if (_SupportsRefinement(_refineLevel)){
        bool supportsWidths = desc.geomStyle == HdBasisCurvesGeomStylePatch &&
            _SupportsUserWidths(drawItem);
        if (supportsWidths){
            bool supportsNormals = _SupportsUserNormals(drawItem);
            if (supportsNormals){
                drawStyle = HdSt_BasisCurvesShaderKey::RIBBON;
                normalStyle = HdSt_BasisCurvesShaderKey::ORIENTED;
            }
            else{
                if (_refineLevel > 2){
                    normalStyle = HdSt_BasisCurvesShaderKey::ROUND;
                    drawStyle = HdSt_BasisCurvesShaderKey::HALFTUBE;
                }
                else if (_refineLevel > 1){
                    normalStyle = HdSt_BasisCurvesShaderKey::ROUND;
                    drawStyle = HdSt_BasisCurvesShaderKey::RIBBON;
                }
                else{
                    drawStyle = HdSt_BasisCurvesShaderKey::RIBBON;
                    normalStyle = HdSt_BasisCurvesShaderKey::HAIR;
                }
            }
        }
    }
    else{
        TF_DEBUG(HD_RPRIM_UPDATED).
            Msg("HdStBasisCurves(%s) - Downcasting curve type to linear because refinement is disabled.\n",
                GetId().GetText());
        curveType = HdTokens->linear;
        curveBasis = TfToken();

    }

    TF_DEBUG(HD_RPRIM_UPDATED).
            Msg("HdStBasisCurves(%s) - Building shader with keys: %s, %s, %s, %s, %s, %s\n",
                GetId().GetText(), curveType.GetText(), 
                curveBasis.GetText(),
                TfEnum::GetName(drawStyle).c_str(), 
                TfEnum::GetName(normalStyle).c_str(),
                _basisWidthInterpolation ? "basis widths" : "linear widths",
                _basisNormalInterpolation ? "basis normals" : "linear normals");

    HdSt_BasisCurvesShaderKey shaderKey(curveType,
                                        curveBasis,
                                        drawStyle,
                                        normalStyle,
                                        _basisWidthInterpolation,
                                        _basisNormalInterpolation);

    TF_DEBUG(HD_RPRIM_UPDATED).
            Msg("HdStBasisCurves(%s) - Shader Key PrimType: %s\n ",
                GetId().GetText(), HdSt_PrimTypeToString(shaderKey.primType));

    HdStResourceRegistrySharedPtr resourceRegistry =
        boost::static_pointer_cast<HdStResourceRegistry>(
            renderIndex.GetResourceRegistry());

    HdSt_GeometricShaderSharedPtr geomShader =
        HdSt_GeometricShader::Create(shaderKey, resourceRegistry);

    TF_VERIFY(geomShader);

    drawItem->SetGeometricShader(geomShader);

    // The batches need to be validated and rebuilt if necessary.
    renderIndex.GetChangeTracker().MarkBatchesDirty();
}

HdDirtyBits
HdStBasisCurves::_PropagateDirtyBits(HdDirtyBits bits) const
{
    // propagate scene-based dirtyBits into rprim-custom dirtyBits
    if (bits & HdChangeTracker::DirtyTopology) {
        bits |= _customDirtyBitsInUse &
            (DirtyIndices|DirtyHullIndices);
    }

    return bits;
}

void
HdStBasisCurves::_InitRepr(TfToken const &reprToken, HdDirtyBits *dirtyBits)
{
    _ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(),
                                            _ReprComparator(reprToken));
    bool isNew = it == _reprs.end();
    if (isNew) {
        _BasisCurvesReprConfig::DescArray descs = _GetReprDesc(reprToken);

        // add new repr
        _reprs.emplace_back(reprToken, boost::make_shared<HdRepr>());
        HdReprSharedPtr &repr = _reprs.back().second;

        *dirtyBits |= HdChangeTracker::NewRepr;

        // allocate all draw items
        for (size_t descIdx = 0; descIdx < descs.size(); ++descIdx) {
            const HdBasisCurvesReprDesc &desc = descs[descIdx];

            if (desc.geomStyle == HdBasisCurvesGeomStyleInvalid) {
                continue;
            }

            HdDrawItem *drawItem = new HdStDrawItem(&_sharedData);
            HdDrawingCoord *drawingCoord = drawItem->GetDrawingCoord();
            repr->AddDrawItem(drawItem);
            if (desc.geomStyle == HdBasisCurvesGeomStyleWire) {
                // Why does geom style require this change?
                drawingCoord->SetTopologyIndex(HdStBasisCurves::HullTopology);
                if (!(_customDirtyBitsInUse & DirtyHullIndices)) {
                    _customDirtyBitsInUse |= DirtyHullIndices;
                    *dirtyBits |= DirtyHullIndices;
                }
            } else {
                if (!(_customDirtyBitsInUse & DirtyIndices)) {
                    _customDirtyBitsInUse |= DirtyIndices;
                    *dirtyBits |= DirtyIndices;
                }
            }

            // Set up drawing coord instance primvars.
            drawingCoord->SetInstancePrimvarBaseIndex(
                HdStBasisCurves::InstancePrimvar);
        }
    }
}

void
HdStBasisCurves::_UpdateRepr(HdSceneDelegate *sceneDelegate,
                             TfToken const &reprToken,
                             HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdReprSharedPtr const &curRepr = _GetRepr(reprToken);
    if (!curRepr) {
        return;
    }

    // Filter custom dirty bits to only those in use.
    *dirtyBits &= (_customDirtyBitsInUse |
                   HdChangeTracker::AllSceneDirtyBits |
                   HdChangeTracker::NewRepr);

    if (TfDebug::IsEnabled(HD_RPRIM_UPDATED)) {
        std::cout << "HdStBasisCurves::_UpdateRepr " << GetId()
                  << " Repr = " << reprToken << "\n";
        HdChangeTracker::DumpDirtyBits(*dirtyBits);
    }

    _BasisCurvesReprConfig::DescArray const &reprDescs = 
        _GetReprDesc(reprToken);

    int drawItemIndex = 0;
    for (size_t descIdx = 0; descIdx < reprDescs.size(); ++descIdx) {
        // curves don't have multiple draw items (for now)
        const HdBasisCurvesReprDesc &desc = reprDescs[descIdx];

        if (desc.geomStyle != HdBasisCurvesGeomStyleInvalid) {
            HdStDrawItem *drawItem = static_cast<HdStDrawItem*>(
                curRepr->GetDrawItem(drawItemIndex++));

            if (HdChangeTracker::IsDirty(*dirtyBits)) {
                _UpdateDrawItem(sceneDelegate, drawItem, dirtyBits, desc);
            } 
        }
    }


    *dirtyBits &= ~HdChangeTracker::NewRepr;
}

void
HdStBasisCurves::_UpdateShadersForAllReprs(HdSceneDelegate *sceneDelegate,
                                           bool updateMaterialShader,
                                           bool updateGeometricShader)
{
    TF_DEBUG(HD_RPRIM_UPDATED).
        Msg("HdStMesh(%s) - Resetting shaders for draw items of all reprs.",
            GetId().GetText());

    SdfPath materialId;
    if (updateMaterialShader) {
        materialId = GetMaterialId();
    }

    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();

    for (auto const& reprPair : _reprs) {
        const TfToken &reprToken = reprPair.first;
        _BasisCurvesReprConfig::DescArray const &descs =
            _GetReprDesc(reprToken);
        HdReprSharedPtr repr = reprPair.second;
        int drawItemIndex = 0;
        for (size_t descIdx = 0; descIdx < descs.size(); ++descIdx) {
            if (descs[descIdx].geomStyle == HdBasisCurvesGeomStyleInvalid) {
                continue;
            }
            HdStDrawItem *drawItem = static_cast<HdStDrawItem*>(
                repr->GetDrawItem(drawItemIndex++));

            if (updateMaterialShader) {
                drawItem->SetMaterialShaderFromRenderIndex(
                    renderIndex, materialId);
            }
            if (updateGeometricShader) {
                _UpdateDrawItemGeometricShader(sceneDelegate, drawItem,
                    descs[descIdx]);
            }
        }
    }
}

void
HdStBasisCurves::_PopulateTopology(HdSceneDelegate *sceneDelegate,
                                   HdStDrawItem *drawItem,
                                   HdDirtyBits *dirtyBits,
                                   const HdBasisCurvesReprDesc &desc)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();
    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        boost::static_pointer_cast<HdStResourceRegistry>(
        sceneDelegate->GetRenderIndex().GetResourceRegistry());

    if (*dirtyBits & HdChangeTracker::DirtyDisplayStyle) {
        _refineLevel = GetDisplayStyle(sceneDelegate).refineLevel;
    }

    // XXX: is it safe to get topology even if it's not dirty?
    if (HdChangeTracker::IsTopologyDirty(*dirtyBits, id) ||
        HdChangeTracker::IsDisplayStyleDirty(*dirtyBits, id)) {


        const HdBasisCurvesTopology &srcTopology =
                                          GetBasisCurvesTopology(sceneDelegate);

        // compute id.
        _topologyId = srcTopology.ComputeHash();
        bool refined = (_refineLevel>0);
        _topologyId = ArchHash64((const char*)&refined, sizeof(refined),
            _topologyId);

        // XXX: Should be HdSt_BasisCurvesTopologySharedPtr
        HdInstance<HdTopology::ID, HdBasisCurvesTopologySharedPtr> topologyInstance;

        // ask registry if there's a sharable mesh topology
        std::unique_lock<std::mutex> regLock =
            resourceRegistry->RegisterBasisCurvesTopology(_topologyId, &topologyInstance);

        if (topologyInstance.IsFirstInstance()) {
            // if this is the first instance, create a new stream topology
            // representation and use that.
            HdSt_BasisCurvesTopologySharedPtr topology =
                                     HdSt_BasisCurvesTopology::New(srcTopology);

            // XXX: Registry is currently in core Hd, so doesn't have access
            // to the St version of the topology,
            HdBasisCurvesTopologySharedPtr baseTopology =
                    boost::static_pointer_cast<HdBasisCurvesTopology>(topology);

            topologyInstance.SetValue(baseTopology);
        }

        // XXX: Registry is currently in core Hd, so doesn't have access
        // to the St version of the topology,
        _topology = boost::static_pointer_cast<HdSt_BasisCurvesTopology>(
                                                   topologyInstance.GetValue());
        TF_VERIFY(_topology);

        // hash collision check
        if (TfDebug::IsEnabled(HD_SAFE_MODE)) {
            TF_VERIFY(srcTopology == *_topology);
        }
    }

    // bail out if the index bar is already synced
    TfToken indexToken;
    if (drawItem->GetDrawingCoord()->GetTopologyIndex()
        == HdStBasisCurves::HullTopology) {
        if ((*dirtyBits & DirtyHullIndices) == 0) return;
        *dirtyBits &= ~DirtyHullIndices;
        indexToken = HdTokens->hullIndices;
    } else {
        if ((*dirtyBits & DirtyIndices) == 0) return;
        *dirtyBits &= ~DirtyIndices;
        indexToken = HdTokens->indices;
    }

    {
        HdInstance<HdTopology::ID, HdBufferArrayRangeSharedPtr> rangeInstance;

        std::unique_lock<std::mutex> regLock =
            resourceRegistry->RegisterBasisCurvesIndexRange(
                _topologyId, indexToken, &rangeInstance);

        if(rangeInstance.IsFirstInstance()) {
            HdBufferSourceVector sources;
            HdBufferSpecVector bufferSpecs;

            sources.push_back(_topology->GetIndexBuilderComputation(
                !_SupportsRefinement(_refineLevel)));

            HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);

            // allocate new range
            HdBufferArrayRangeSharedPtr range
                = resourceRegistry->AllocateNonUniformBufferArrayRange(
                    HdTokens->topology, bufferSpecs, HdBufferArrayUsageHint());

            // add sources to update queue
            resourceRegistry->AddSources(range, sources);
            rangeInstance.SetValue(range);
        }

        _sharedData.barContainer.Set(
            drawItem->GetDrawingCoord()->GetTopologyIndex(),
            rangeInstance.GetValue());
    }
}

void
HdStBasisCurves::_PopulateVertexPrimvars(HdSceneDelegate *sceneDelegate,
                                         HdStDrawItem *drawItem,
                                         HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();
    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        boost::static_pointer_cast<HdStResourceRegistry>(
        sceneDelegate->GetRenderIndex().GetResourceRegistry());

    // The "points" attribute is expected to be in this list.
    HdPrimvarDescriptorVector primvars =
        GetPrimvarDescriptors(sceneDelegate, HdInterpolationVertex);

    // Analyze and append varying primvars.
    {
        HdPrimvarDescriptorVector varyingPvs =
            GetPrimvarDescriptors(sceneDelegate, HdInterpolationVarying);

        // XXX: It's sort of a waste to do basis width interpolation
        // by default, but in testImagingComputation there seems
        // to be a way to initialize this shader with cubic widths
        // where widths doesn't appear in the vertex primvar list. To
        // avoid a regression, I'm setting the behavior to default to
        // basis/cubic width interpolation until we understand the test
        // case a little better.
        _basisWidthInterpolation = true;
        // If we don't find varying normals, then we are assuming
        // implicit normals or prescribed basis normals. (For implicit
        // normals, varying might be the right fallback behavior, but
        // leaving as basis for now to preserve the current behavior
        // until we get can do a better pass on curve normals.)
        _basisNormalInterpolation = true;
        for (HdPrimvarDescriptor const& primvar: varyingPvs) {
            if (primvar.name == HdTokens->widths) {
                _basisWidthInterpolation = false;
            } else if (primvar.name == HdTokens->normals) {
                _basisNormalInterpolation = false;
            }
        }

        primvars.insert(primvars.end(), varyingPvs.begin(), varyingPvs.end());
    }

    HdBufferSourceVector sources;
    HdBufferSourceVector reserveOnlySources;
    HdBufferSourceVector separateComputationSources;
    HdComputationVector computations;
    sources.reserve(primvars.size());

    HdSt_GetExtComputationPrimvarsComputations(
        id,
        sceneDelegate,
        HdInterpolationVertex,
        *dirtyBits,
        &sources,
        &reserveOnlySources,
        &separateComputationSources,
        &computations);

    for (HdPrimvarDescriptor const& primvar: primvars) {
        if (!HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, primvar.name))
            continue;

        // TODO: We don't need to pull primvar metadata every time a value
        // changes, but we need support from the delegate.

        //assert name not in range.bufferArray.GetResources()
        VtValue value = GetPrimvar(sceneDelegate, primvar.name);
        if (!value.IsEmpty()) {
            if (primvar.name == HdTokens->points) {
                // We want to validate the topology by making sure the number of
                // verts is equal or greater than the number of verts the topology
                // references
                if (!_topology) {
                    TF_CODING_ERROR("No topology set for BasisCurve %s",
                                    id.GetName().c_str());
                }
                else if(!value.IsHolding<VtVec3fArray>() ||
                        (!_topology->HasIndices() &&
                        value.Get<VtVec3fArray>().size() != _topology->CalculateNeededNumberOfControlPoints())) {
                    TF_WARN("Topology and vertices do not match for "
                            "BasisCurve %s",id.GetName().c_str());
                }
            }

            // XXX: this really needs to happen for all primvars.
            if (primvar.name == HdTokens->widths) {
                sources.push_back(HdBufferSourceSharedPtr(
                        new HdSt_BasisCurvesWidthsInterpolaterComputation(
                                      _topology.get(), value.Get<VtFloatArray>())));
            } else if (primvar.name == HdTokens->normals) {
                sources.push_back(HdBufferSourceSharedPtr(
                        new HdSt_BasisCurvesNormalsInterpolaterComputation(
                                      _topology.get(), value.Get<VtVec3fArray>())));
            } else {
                sources.push_back(HdBufferSourceSharedPtr(
                        new HdVtBufferSource(primvar.name, value)));
            }
        }
    }

    // XXX: To Do: Check primvar counts against Topology expected counts
    // XXX: To Do: Width / Normal Interpolation
    // XXX: To Do: Custom Primvar Interpolation
    // XXX: To Do: Varying Interpolation mode

    // return before allocation if it's empty.
    if (sources.empty() && computations.empty()) {
        return;
    }

    if (!drawItem->GetVertexPrimvarRange() ||
        !drawItem->GetVertexPrimvarRange()->IsValid()) {

        // new buffer specs
        HdBufferSpecVector bufferSpecs;
        HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);
        HdBufferSpec::GetBufferSpecs(reserveOnlySources, &bufferSpecs);
        HdBufferSpec::GetBufferSpecs(computations, &bufferSpecs);

        HdBufferArrayRangeSharedPtr range =
            resourceRegistry->AllocateNonUniformBufferArrayRange(
                HdTokens->primvar, bufferSpecs, HdBufferArrayUsageHint());
        _sharedData.barContainer.Set(
            drawItem->GetDrawingCoord()->GetVertexPrimvarIndex(), range);
    }

    // add sources to update queue
    if (!sources.empty()) {
        resourceRegistry->AddSources(drawItem->GetVertexPrimvarRange(),
                                     sources);
    }
    if (!computations.empty()) {
        TF_FOR_ALL(it, computations) {
            resourceRegistry->AddComputation(drawItem->GetVertexPrimvarRange(),
                                             *it);
        }
    }
    if (!separateComputationSources.empty()) {
        TF_FOR_ALL(it, separateComputationSources) {
            resourceRegistry->AddSource(*it);
        }
    }
}

void
HdStBasisCurves::_PopulateElementPrimvars(HdSceneDelegate *sceneDelegate,
                                          HdStDrawItem *drawItem,
                                          HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();
    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        boost::static_pointer_cast<HdStResourceRegistry>(
        sceneDelegate->GetRenderIndex().GetResourceRegistry());

    HdPrimvarDescriptorVector uniformPrimvars =
        GetPrimvarDescriptors(sceneDelegate, HdInterpolationUniform);

    HdBufferSourceVector sources;
    sources.reserve(uniformPrimvars.size());

    for (HdPrimvarDescriptor const& primvar: uniformPrimvars) {
        if (!HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, primvar.name))
            continue;

        VtValue value = GetPrimvar(sceneDelegate, primvar.name);
        if (!value.IsEmpty()) {
            sources.push_back(HdBufferSourceSharedPtr(
                              new HdVtBufferSource(primvar.name, value)));
        }
    }

    // return before allocation if it's empty.
    if (sources.empty())
        return;

    // element primvars exist.
    if (!drawItem->GetElementPrimvarRange() ||
        !drawItem->GetElementPrimvarRange()->IsValid()) {
        HdBufferSpecVector bufferSpecs;
        HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);

        HdBufferArrayRangeSharedPtr range =
            resourceRegistry->AllocateNonUniformBufferArrayRange(
                HdTokens->primvar, bufferSpecs, HdBufferArrayUsageHint());
        _sharedData.barContainer.Set(
            drawItem->GetDrawingCoord()->GetElementPrimvarIndex(), range);
    }

    resourceRegistry->AddSources(drawItem->GetElementPrimvarRange(),
                                 sources);
}

static bool 
HdSt_HasResource(HdStDrawItem* drawItem, const TfToken& resourceToken){
    // Check for authored resource, we could leverage dirtyBits here as an
    // optimization, however the BAR is the ground truth, so until there is a
    // known peformance issue, we just check them explicitly.
    bool hasAuthoredResouce = false;

    typedef HdBufferArrayRangeSharedPtr HdBarPtr;
    if (HdBarPtr const& bar = drawItem->GetConstantPrimvarRange()){
        HdStBufferArrayRangeGLSharedPtr bar_ =
            boost::static_pointer_cast<HdStBufferArrayRangeGL> (bar);
        hasAuthoredResouce |= bool(bar_->GetResource(resourceToken));
    }
    if (HdBarPtr const& bar = drawItem->GetVertexPrimvarRange()) {
        HdStBufferArrayRangeGLSharedPtr bar_ =
            boost::static_pointer_cast<HdStBufferArrayRangeGL> (bar);
        hasAuthoredResouce |= bool(bar_->GetResource(resourceToken));
    }
    if (HdBarPtr const& bar = drawItem->GetElementPrimvarRange()){
        HdStBufferArrayRangeGLSharedPtr bar_ =
            boost::static_pointer_cast<HdStBufferArrayRangeGL> (bar);

        hasAuthoredResouce |= bool(bar_->GetResource(resourceToken));
    }
    int instanceNumLevels = drawItem->GetInstancePrimvarNumLevels();
    for (int i = 0; i < instanceNumLevels; ++i) {
        if (HdBarPtr const& bar = drawItem->GetInstancePrimvarRange(i)) {
            HdStBufferArrayRangeGLSharedPtr bar_ =
                boost::static_pointer_cast<HdStBufferArrayRangeGL> (bar);

            hasAuthoredResouce |= bool(bar_->GetResource(resourceToken));
        }
    }
    return hasAuthoredResouce;  
}

bool
HdStBasisCurves::_SupportsRefinement(int refineLevel)
{
    if(!_topology) {
        TF_CODING_ERROR("Calling _SupportsRefinement before topology is set");
        return false;
    }

    return refineLevel > 0 || IsEnabledForceRefinedCurves();
}

bool 
HdStBasisCurves::_SupportsUserWidths(HdStDrawItem* drawItem){
    return HdSt_HasResource(drawItem, HdTokens->widths);
}
bool 
HdStBasisCurves::_SupportsUserNormals(HdStDrawItem* drawItem){
    return HdSt_HasResource(drawItem, HdTokens->normals);
}

HdDirtyBits
HdStBasisCurves::GetInitialDirtyBitsMask() const
{
    HdDirtyBits mask = HdChangeTracker::Clean
        | HdChangeTracker::InitRepr
        | HdChangeTracker::DirtyExtent
        | HdChangeTracker::DirtyInstanceIndex
        | HdChangeTracker::DirtyNormals
        | HdChangeTracker::DirtyPoints
        | HdChangeTracker::DirtyPrimID
        | HdChangeTracker::DirtyPrimvar
        | HdChangeTracker::DirtyDisplayStyle
        | HdChangeTracker::DirtyRepr
        | HdChangeTracker::DirtyMaterialId
        | HdChangeTracker::DirtyTopology
        | HdChangeTracker::DirtyTransform 
        | HdChangeTracker::DirtyVisibility 
        | HdChangeTracker::DirtyWidths
        | HdChangeTracker::DirtyComputationPrimvarDesc
        ;

    return mask;
}

PXR_NAMESPACE_CLOSE_SCOPE

