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
HdStBasisCurves::Sync(HdSceneDelegate* delegate,
                      HdRenderParam*   renderParam,
                      HdDirtyBits*     dirtyBits,
                      TfToken const&   reprName,
                      bool             forcedRepr)
{
    TF_UNUSED(renderParam);

    HdRprim::_Sync(delegate,
                  reprName,
                  forcedRepr,
                  dirtyBits);

    TfToken calcReprName = _GetReprName(delegate, reprName,
                                        forcedRepr, dirtyBits);
    _GetRepr(delegate, calcReprName, dirtyBits);

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
    _PopulateConstantPrimVars(sceneDelegate, drawItem, dirtyBits);

    /* MATERIAL SHADER */
    drawItem->SetMaterialShaderFromRenderIndex(
        sceneDelegate->GetRenderIndex(), GetMaterialId());

    /* INSTANCE PRIMVARS */
    if (!GetInstancerId().IsEmpty()) {
        HdStInstancer *instancer = static_cast<HdStInstancer*>(
            sceneDelegate->GetRenderIndex().GetInstancer(GetInstancerId()));
        if (TF_VERIFY(instancer)) {
            instancer->PopulateDrawItem(drawItem, &_sharedData,
                dirtyBits, InstancePrimVar);
        }
    }

    /* TOPOLOGY */
    // XXX: _PopulateTopology should be split into two phase
    //      for scene dirtybits and for repr dirtybits.
    if (*dirtyBits & (HdChangeTracker::DirtyTopology
                    | HdChangeTracker::DirtyRefineLevel
                    | DirtyIndices
                    | DirtyHullIndices)) {
        _PopulateTopology(sceneDelegate, drawItem, dirtyBits, desc);
    }

    /* PRIMVAR */
    if (HdChangeTracker::IsAnyPrimVarDirty(*dirtyBits, id)) {
        // XXX: curves don't use refined vertex primvars, however,
        // the refined renderpass masks the dirtiness of non-refined vertex
        // primvars, so we need to see refined dirty for updating coarse
        // vertex primvars if there is only refined reprs being updated.
        // we'll fix the change tracking in order to address this craziness.
        _PopulateVertexPrimVars(sceneDelegate, drawItem, dirtyBits);
        _PopulateElementPrimVars(sceneDelegate, drawItem, dirtyBits);
    }

    // Topology and VertexPrimVar may be null, if the curve has zero line
    // segments.
    TF_VERIFY(drawItem->GetConstantPrimVarRange());
}

void
HdStBasisCurves::_UpdateDrawItemGeometricShader(
        HdSceneDelegate *sceneDelegate,
        HdStDrawItem *drawItem,
        const HdBasisCurvesReprDesc &desc)
{
    if (!TF_VERIFY(_topology)) return;

    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();

    // Check for authored normals, we could leverage dirtyBits here as an
    // optimization, however the BAR is the ground truth, so until there is a
    // known peformance issue, we just check them explicitly.
    bool hasAuthoredNormals = false;

    if (!hasAuthoredNormals) {
        // Check if we picked up normals on a previous update.
        typedef HdBufferArrayRangeSharedPtr HdBarPtr;
        if (HdBarPtr const& bar = drawItem->GetConstantPrimVarRange()){
            HdStBufferArrayRangeGLSharedPtr bar_ =
                boost::static_pointer_cast<HdStBufferArrayRangeGL> (bar);
            hasAuthoredNormals |= bool(bar_->GetResource(HdTokens->normals));
        }
        if (HdBarPtr const& bar = drawItem->GetVertexPrimVarRange()) {
            HdStBufferArrayRangeGLSharedPtr bar_ =
                boost::static_pointer_cast<HdStBufferArrayRangeGL> (bar);
            hasAuthoredNormals |= bool(bar_->GetResource(HdTokens->normals));
        }
        if (HdBarPtr const& bar = drawItem->GetElementPrimVarRange()){
            HdStBufferArrayRangeGLSharedPtr bar_ =
                boost::static_pointer_cast<HdStBufferArrayRangeGL> (bar);

            hasAuthoredNormals |= bool(bar_->GetResource(HdTokens->normals));
        }
        int instanceNumLevels = drawItem->GetInstancePrimVarNumLevels();
        for (int i = 0; i < instanceNumLevels; ++i) {
            if (HdBarPtr const& bar = drawItem->GetInstancePrimVarRange(i)) {
                HdStBufferArrayRangeGLSharedPtr bar_ =
                    boost::static_pointer_cast<HdStBufferArrayRangeGL> (bar);

                hasAuthoredNormals |= bool(bar_->GetResource(HdTokens->normals));
            }
        }
    }

    HdSt_BasisCurvesShaderKey shaderKey(_topology->GetCurveBasis(),
                                        hasAuthoredNormals,
                                        (_SupportsSmoothCurves(desc,
                                                               _refineLevel)));
    HdStResourceRegistrySharedPtr resourceRegistry =
        boost::static_pointer_cast<HdStResourceRegistry>(
            renderIndex.GetResourceRegistry());

    HdSt_GeometricShaderSharedPtr geomShader =
        HdSt_GeometricShader::Create(shaderKey, resourceRegistry);

    TF_VERIFY(geomShader);

    drawItem->SetGeometricShader(geomShader);

    // The batches need to be validated and rebuilt if necessary.
    renderIndex.GetChangeTracker().MarkShaderBindingsDirty();
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
HdStBasisCurves::_InitRepr(TfToken const &reprName,
                        HdDirtyBits *dirtyBits)
{
    _ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(),
                                            _ReprComparator(reprName));
    bool isNew = it == _reprs.end();
    if (isNew) {
        _BasisCurvesReprConfig::DescArray descs = _GetReprDesc(reprName);

        // add new repr
        _reprs.emplace_back(reprName, boost::make_shared<HdRepr>());
        HdReprSharedPtr &repr = _reprs.back().second;

        *dirtyBits |= HdChangeTracker::NewRepr;

        // allocate all draw items
        for (size_t descIdx = 0; descIdx < descs.size(); ++descIdx) {
            const HdBasisCurvesReprDesc &desc = descs[descIdx];

            if (desc.geomStyle == HdBasisCurvesGeomStyleInvalid) {
                continue;
            }

            HdDrawItem *drawItem = new HdStDrawItem(&_sharedData);
            repr->AddDrawItem(drawItem);
            if (desc.geomStyle == HdBasisCurvesGeomStyleLine) {
                HdDrawingCoord *drawingCoord = drawItem->GetDrawingCoord();
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
        }
    }
}


HdReprSharedPtr const &
HdStBasisCurves::_GetRepr(HdSceneDelegate *sceneDelegate,
                          TfToken const &reprName,
                          HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _BasisCurvesReprConfig::DescArray const &reprDescs = _GetReprDesc(reprName);

    _ReprVector::iterator reprIt = std::find_if(_reprs.begin(), _reprs.end(),
                                            _ReprComparator(reprName));
    if (reprIt == _reprs.end()) {
        // Hydra should have called _InitRepr earlier in sync when
        // before sending dirty bits to the delegate.
        TF_CODING_ERROR("_InitRepr() should be called for repr %s.",
                        reprName.GetText());

        static const HdReprSharedPtr ERROR_RETURN;

        return ERROR_RETURN;
    }

    // _reprs holds a pair of (TfToken, HdReprSharedPtr)
    HdReprSharedPtr const &curRepr = reprIt->second;

    // Filter custom dirty bits to only those in use.
    *dirtyBits &= (_customDirtyBitsInUse |
                   HdChangeTracker::AllSceneDirtyBits |
                   HdChangeTracker::NewRepr);

    if (TfDebug::IsEnabled(HD_RPRIM_UPDATED)) {
        std::cout << "HdStBasisCurves::GetRepr " << GetId()
                  << " Repr = " << reprName << "\n";
        HdChangeTracker::DumpDirtyBits(*dirtyBits);
    }

    // For the bits geometric shader depends on, reset the geometric shaders
    // for all the draw items of all the reprs.
    bool needsSetGeometricShader = false;
    if (*dirtyBits & (HdChangeTracker::DirtyRefineLevel |
                      HdChangeTracker::DirtyMaterialId)) {
        needsSetGeometricShader = true;
    }

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

    if (needsSetGeometricShader) {
        // needsSetGeometricShader is set when all reprs need a refresh of their
        // geometric shader.

        TF_DEBUG(HD_RPRIM_UPDATED).
            Msg("HdStBasisCurves - Resetting the geometric shader for all draw"
                " items (currently only 1 per repr) of all reprs");

        TF_FOR_ALL (it, _reprs) {
            _BasisCurvesReprConfig::DescArray const &descs =
                _GetReprDesc(it->first);
            _UpdateReprGeometricShader(sceneDelegate, descs, it->second);
        }
    } else if (*dirtyBits & HdChangeTracker::NewRepr) {
        // If NewRepr is set, we haven't initialized the geometric shader yet.
        _UpdateReprGeometricShader(sceneDelegate, reprDescs, curRepr);
    }

    *dirtyBits &= ~HdChangeTracker::NewRepr;

    return curRepr;
}

void
HdStBasisCurves::_UpdateReprGeometricShader(HdSceneDelegate *sceneDelegate,
                                 _BasisCurvesReprConfig::DescArray const &descs,
                                 HdReprSharedPtr repr)
{
    int drawItemIndex = 0;
    for (size_t descIdx = 0; descIdx < descs.size(); ++descIdx) {
        if (descs[descIdx].geomStyle != HdBasisCurvesGeomStyleInvalid) {
            HdStDrawItem *drawItem = static_cast<HdStDrawItem*>(
                repr->GetDrawItem(drawItemIndex++));
            _UpdateDrawItemGeometricShader(sceneDelegate, drawItem,
                                           descs[descIdx]);
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

    if (*dirtyBits & HdChangeTracker::DirtyRefineLevel) {
        _refineLevel = GetRefineLevel(sceneDelegate);
    }

    // XXX: is it safe to get topology even if it's not dirty?
    if (HdChangeTracker::IsTopologyDirty(*dirtyBits, id) ||
        HdChangeTracker::IsRefineLevelDirty(*dirtyBits, id)) {


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

            bool refine = _SupportsSmoothCurves(desc, _refineLevel);

            sources.push_back(_topology->GetIndexBuilderComputation(refine));

            TF_FOR_ALL(it, sources) {
                (*it)->AddBufferSpecs(&bufferSpecs);
            }

            // allocate new range
            HdBufferArrayRangeSharedPtr range
                = resourceRegistry->AllocateNonUniformBufferArrayRange(
                    HdTokens->topology, bufferSpecs);

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
HdStBasisCurves::_PopulateVertexPrimVars(HdSceneDelegate *sceneDelegate,
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
    TfTokenVector primVarNames = GetPrimVarVertexNames(sceneDelegate);
    TfTokenVector const& vars = GetPrimVarVaryingNames(sceneDelegate);
    primVarNames.insert(primVarNames.end(), vars.begin(), vars.end());

    HdBufferSourceVector sources;
    HdComputationVector computations;
    sources.reserve(primVarNames.size());
    computations.reserve(primVarNames.size());

    TF_FOR_ALL(nameIt, primVarNames) {
        if (!HdChangeTracker::IsPrimVarDirty(*dirtyBits, id, *nameIt))
            continue;

        // TODO: We don't need to pull primvar metadata every time a value
        // changes, but we need support from the delegate.

        //assert name not in range.bufferArray.GetResources()
        VtValue value = GetPrimVar(sceneDelegate, *nameIt);
        if (!value.IsEmpty()) {
            if (*nameIt == HdTokens->points) {
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
            if (*nameIt == HdTokens->widths) {
                sources.push_back(HdBufferSourceSharedPtr(
                        new HdSt_BasisCurvesWidthsInterpolaterComputation(
                                      _topology.get(), value.Get<VtFloatArray>())));
            } else if (*nameIt == HdTokens->normals) {
                sources.push_back(HdBufferSourceSharedPtr(
                        new HdSt_BasisCurvesNormalsInterpolaterComputation(
                                      _topology.get(), value.Get<VtVec3fArray>())));
            } else {
                sources.push_back(HdBufferSourceSharedPtr(
                        new HdVtBufferSource(*nameIt, value)));
            }
        }
    }

    HdBufferSourceVector computationSources;
    HdSt_GetExtComputationPrimVarsComputations(
        id,
        sceneDelegate,
        HdInterpolationVertex,
        *dirtyBits,
        &sources,
        &computations,
        &computationSources);

    // XXX: To Do: Check primvar counts against Topology expected counts
    // XXX: To Do: Width / Normal Interpolation
    // XXX: To Do: Custom Primvar Interpolation
    // XXX: To Do: Varying Interpolation mode

    // return before allocation if it's empty.
    if (sources.empty() && computations.empty()) {
        return;
    }

    if (!drawItem->GetVertexPrimVarRange() ||
        !drawItem->GetVertexPrimVarRange()->IsValid()) {
        // initialize buffer array
        HdBufferSpecVector bufferSpecs;
        TF_FOR_ALL(it, sources) {
            (*it)->AddBufferSpecs(&bufferSpecs);
        }
        TF_FOR_ALL(it, computations) {
            (*it)->AddBufferSpecs(&bufferSpecs);
        }
        HdBufferArrayRangeSharedPtr range =
            resourceRegistry->AllocateNonUniformBufferArrayRange(
                HdTokens->primVar, bufferSpecs);
        _sharedData.barContainer.Set(
            drawItem->GetDrawingCoord()->GetVertexPrimVarIndex(), range);
    }

    // add sources to update queue
    if (!sources.empty()) {
        resourceRegistry->AddSources(drawItem->GetVertexPrimVarRange(),
                                     sources);
    }
    if (!computations.empty()) {
        TF_FOR_ALL(it, computations) {
            resourceRegistry->AddComputation(drawItem->GetVertexPrimVarRange(),
                                             *it);
        }
    }
    if (!computationSources.empty()) {
        TF_FOR_ALL(it, computationSources) {
            resourceRegistry->AddSource(*it);
        }
    }
}

void
HdStBasisCurves::_PopulateElementPrimVars(HdSceneDelegate *sceneDelegate,
                                          HdStDrawItem *drawItem,
                                          HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();
    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        boost::static_pointer_cast<HdStResourceRegistry>(
        sceneDelegate->GetRenderIndex().GetResourceRegistry());

    TfTokenVector primVarNames = GetPrimVarUniformNames(sceneDelegate);

    HdBufferSourceVector sources;
    sources.reserve(primVarNames.size());

    TF_FOR_ALL(nameIt, primVarNames) {
        if (!HdChangeTracker::IsPrimVarDirty(*dirtyBits, id, *nameIt))
            continue;

        VtValue value = GetPrimVar(sceneDelegate, *nameIt);
        if (!value.IsEmpty()) {
            sources.push_back(HdBufferSourceSharedPtr(
                              new HdVtBufferSource(*nameIt, value)));
        }
    }

    // return before allocation if it's empty.
    if (sources.empty())
        return;

    // element primvars exist.
    if (!drawItem->GetElementPrimVarRange() ||
        !drawItem->GetElementPrimVarRange()->IsValid()) {
        HdBufferSpecVector bufferSpecs;
        TF_FOR_ALL(it, sources) {
            (*it)->AddBufferSpecs(&bufferSpecs);
        }
        HdBufferArrayRangeSharedPtr range =
            resourceRegistry->AllocateNonUniformBufferArrayRange(
                HdTokens->primVar, bufferSpecs);
        _sharedData.barContainer.Set(
            drawItem->GetDrawingCoord()->GetElementPrimVarIndex(), range);
    }

    resourceRegistry->AddSources(drawItem->GetElementPrimVarRange(),
                                 sources);
}


bool
HdStBasisCurves::_SupportsSmoothCurves(const HdBasisCurvesReprDesc &desc,
                                       int refineLevel)
{
    if(!_topology) {
        TF_CODING_ERROR("Calling _SupportsSmoothCurves before topology is set");
        return false;
    }

    if (desc.geomStyle != HdBasisCurvesGeomStyleRefined) {
        return false;
    }

    TfToken curveType = _topology->GetCurveType(); 
    TfToken curveBasis = _topology->GetCurveBasis(); 

    if(curveType == HdTokens->cubic &&
       (curveBasis == HdTokens->bezier || 
        curveBasis == HdTokens->bSpline || 
        curveBasis == HdTokens->catmullRom)) {

        if (refineLevel > 0 || IsEnabledForceRefinedCurves()) {
            return true;
        }
    }

    return false;
}

HdDirtyBits
HdStBasisCurves::_GetInitialDirtyBits() const
{
    HdDirtyBits mask = HdChangeTracker::Clean
        | HdChangeTracker::InitRepr
        | HdChangeTracker::DirtyExtent
        | HdChangeTracker::DirtyInstanceIndex
        | HdChangeTracker::DirtyNormals
        | HdChangeTracker::DirtyPoints
        | HdChangeTracker::DirtyPrimID
        | HdChangeTracker::DirtyPrimVar
        | HdChangeTracker::DirtyRefineLevel
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

