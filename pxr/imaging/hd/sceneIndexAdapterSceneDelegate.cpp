//
// Copyright 2021 Pixar
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
#include "pxr/imaging/hd/sceneIndexAdapterSceneDelegate.h"
#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/coordSys.h"
#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/field.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/light.h"
#include "pxr/imaging/hd/renderBuffer.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/pxOsd/tokens.h"
#include "pxr/base/trace/trace.h"

#include "pxr/imaging/hd/dataSourceLegacyPrim.h"
#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/imaging/hd/dirtyBitsTranslator.h"

#include "pxr/imaging/hd/flatteningSceneIndex.h"
#include "pxr/imaging/hd/prefixingSceneIndex.h"

#include "pxr/imaging/hd/basisCurvesSchema.h"
#include "pxr/imaging/hd/basisCurvesTopologySchema.h"
#include "pxr/imaging/hd/cameraSchema.h"
#include "pxr/imaging/hd/categoriesSchema.h"
#include "pxr/imaging/hd/coordSysBindingSchema.h"
#include "pxr/imaging/hd/dataSharingSchema.h"
#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/dependencySchema.h"
#include "pxr/imaging/hd/extComputationInputComputationSchema.h"
#include "pxr/imaging/hd/extComputationOutputSchema.h"
#include "pxr/imaging/hd/extComputationPrimvarSchema.h"
#include "pxr/imaging/hd/extComputationPrimvarsSchema.h"
#include "pxr/imaging/hd/extComputationSchema.h"
#include "pxr/imaging/hd/extentSchema.h"
#include "pxr/imaging/hd/geomSubsetSchema.h"
#include "pxr/imaging/hd/geomSubsetsSchema.h"
#include "pxr/imaging/hd/instanceCategoriesSchema.h"
#include "pxr/imaging/hd/instancedBySchema.h"
#include "pxr/imaging/hd/instancerTopologySchema.h"
#include "pxr/imaging/hd/instanceSchema.h"
#include "pxr/imaging/hd/legacyDisplayStyleSchema.h"
#include "pxr/imaging/hd/lightSchema.h"
#include "pxr/imaging/hd/materialBindingSchema.h"
#include "pxr/imaging/hd/materialConnectionSchema.h"
#include "pxr/imaging/hd/materialNetworkSchema.h"
#include "pxr/imaging/hd/materialNodeSchema.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/meshSchema.h"
#include "pxr/imaging/hd/meshTopologySchema.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/purposeSchema.h"
#include "pxr/imaging/hd/renderBufferSchema.h"
#include "pxr/imaging/hd/subdivisionTagsSchema.h"
#include "pxr/imaging/hd/visibilitySchema.h"
#include "pxr/imaging/hd/volumeFieldBindingSchema.h"
#include "pxr/imaging/hd/volumeFieldSchema.h"
#include "pxr/imaging/hd/xformSchema.h"

#include "tbb/concurrent_unordered_set.h"

#include "pxr/imaging/hf/perfLog.h"

PXR_NAMESPACE_OPEN_SCOPE

// XXX: currently private and duplicated where used so as to not yet formally
//      define this convention.
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (prmanParams)
    ((prmanParamsNames, ""))
);


/* static */
HdSceneIndexBaseRefPtr
HdSceneIndexAdapterSceneDelegate::AppendDefaultSceneFilters(
    HdSceneIndexBaseRefPtr inputSceneIndex, SdfPath const &delegateID)
{
    HdSceneIndexBaseRefPtr result = inputSceneIndex;

    // if no prefix, don't add HdPrefixingSceneIndex
    if (!delegateID.IsEmpty() && delegateID != SdfPath::AbsoluteRootPath()) {
        result = HdPrefixingSceneIndex::New(result, delegateID);
    }

    // disabling flattening as it's not yet needed for pure emulation
    //result = HdFlatteningSceneIndex::New(result);

    return result;
}

// ----------------------------------------------------------------------------

HdSceneIndexAdapterSceneDelegate::HdSceneIndexAdapterSceneDelegate(
    HdSceneIndexBaseRefPtr inputSceneIndex,
    HdRenderIndex *parentIndex,
    SdfPath const &delegateID)
: HdSceneDelegate(parentIndex, delegateID)
, _inputSceneIndex(inputSceneIndex)
, _sceneDelegatesBuilt(false)
, _cachedLocatorSet()
, _cachedDirtyBits(0)
, _cachedPrimType()
{
    HdSceneIndexNameRegistry::GetInstance().RegisterNamedSceneIndex(
        "HdSceneIndexAdapterSceneDelegate scene: " + delegateID.GetString(),
            inputSceneIndex);

    // XXX: note that we will likely want to move this to the Has-A observer
    // pattern we're using now...
    _inputSceneIndex->AddObserver(HdSceneIndexObserverPtr(this));
}

HdSceneIndexAdapterSceneDelegate::~HdSceneIndexAdapterSceneDelegate()
{
    GetRenderIndex()._RemoveSubtree(GetDelegateID(), this);
}

// ----------------------------------------------------------------------------
// HdSceneIndexObserver interfaces

void
HdSceneIndexAdapterSceneDelegate::_PrimAdded(
    const SdfPath &primPath,
    const TfToken &primType)
{
    SdfPath indexPath = primPath;
    _PrimCacheTable::iterator it = _primCache.find(indexPath);

    bool insertIfNeeded = true;

    if (it != _primCache.end()) {
        _PrimCacheEntry &entry = (*it).second;
        const TfToken &existingType = entry.primType;
        if (primType != existingType) {
            if (GetRenderIndex().IsRprimTypeSupported(existingType)) {
                GetRenderIndex()._RemoveRprim(indexPath);
            } else if (GetRenderIndex().IsSprimTypeSupported(existingType)) {
                GetRenderIndex()._RemoveSprim(existingType, indexPath);
            } else if (GetRenderIndex().IsBprimTypeSupported(existingType)) {
                GetRenderIndex()._RemoveBprim(existingType, indexPath);
            } else if (existingType == HdPrimTypeTokens->instancer) {
                GetRenderIndex()._RemoveInstancer(indexPath);
            }
        } else {
            insertIfNeeded = false;
        }
    }

    if (insertIfNeeded) {
        enum PrimType {
            PrimType_None = 0, 
            PrimType_R, 
            PrimType_S,
            PrimType_B,
            PrimType_I,
        };

        PrimType hydraPrimType = PrimType_None;
        if (GetRenderIndex().IsRprimTypeSupported(primType)) {
            hydraPrimType = PrimType_R;
        } else if (GetRenderIndex().IsSprimTypeSupported(primType)) {
            hydraPrimType = PrimType_S;
        } else if (GetRenderIndex().IsBprimTypeSupported(primType)) {
            hydraPrimType = PrimType_B;
        } else if (primType == HdPrimTypeTokens->instancer) {
            hydraPrimType = PrimType_I;
        }

        if (hydraPrimType) {
            switch (hydraPrimType)
            {
            case PrimType_R:
                GetRenderIndex()._InsertRprim(primType, this, indexPath);
                break;
            case PrimType_S:
                GetRenderIndex()._InsertSprim(primType, this, indexPath);
                break;
            case PrimType_B:
                GetRenderIndex()._InsertBprim(primType, this, indexPath);
                break;
            case PrimType_I:
                GetRenderIndex()._InsertInstancer(this, indexPath);
                break;
            default:
                break;
            };
        }

        if (it != _primCache.end()) {
            _PrimCacheEntry & entry = (*it).second;
            entry.primType = primType;
        } else {
            _primCache[indexPath].primType = primType;
        }
    }
}

void
HdSceneIndexAdapterSceneDelegate::PrimsAdded(
    const HdSceneIndexBase &sender,
    const AddedPrimEntries &entries)
{
    TRACE_FUNCTION();

    for (const AddedPrimEntry &entry : entries) {
        _PrimAdded(entry.primPath, entry.primType);
    }
    if (!entries.empty()) {
        _sceneDelegatesBuilt = false;
    }
}

void
HdSceneIndexAdapterSceneDelegate::PrimsRemoved(
    const HdSceneIndexBase &sender,
    const RemovedPrimEntries &entries)
{
    TRACE_FUNCTION();

    for (const RemovedPrimEntry &entry : entries) {
        // Special case Remove("/"), since this is a common shutdown operation.
        // Note: _Clear is faster than _RemoveSubtree here.
        if (entry.primPath.IsAbsoluteRootPath()) {
            GetRenderIndex()._Clear();
            _primCache.ClearInParallel();
            TfReset(_primCache);
            continue;
        }

        // RenderIndex::_RemoveSubtree can be expensive, so if we're
        // getting a remove message for a single prim it's better to
        // spend some time detecting that and calling the single-prim remove.
        _PrimCacheTable::iterator it = _primCache.find(entry.primPath);

        if (it == _primCache.end()) {
            continue;
        }

        const TfToken &primType = it->second.primType;

        _PrimCacheTable::iterator child = it;
        ++child;
        if (child == _primCache.end() ||
            child->first.GetParentPath() != it->first) {
            // The next item after entry.primPath is not a child, so we can
            // single-delete...
            if (GetRenderIndex().IsRprimTypeSupported(primType)) {
                GetRenderIndex()._RemoveRprim(entry.primPath);
            } else if (GetRenderIndex().IsSprimTypeSupported(primType)) {
                GetRenderIndex()._RemoveSprim(primType, entry.primPath);
            } else if (GetRenderIndex().IsBprimTypeSupported(primType)) {
                GetRenderIndex()._RemoveBprim(primType, entry.primPath);
            } else if (primType == HdPrimTypeTokens->instancer) {
                GetRenderIndex()._RemoveInstancer(entry.primPath);
            }
        } else {
            // Otherwise, there's a subtree and we need to call _RemoveSubtree.
            GetRenderIndex()._RemoveSubtree(entry.primPath, this);
        }
        _primCache.erase(it);
    }
    if (!entries.empty()) {
        _sceneDelegatesBuilt = false;
    }
}

void
HdSceneIndexAdapterSceneDelegate::PrimsDirtied(
    const HdSceneIndexBase &sender,
    const DirtiedPrimEntries &entries)
{
    TRACE_FUNCTION();

    for (const DirtiedPrimEntry &entry : entries) {
        const SdfPath &indexPath = entry.primPath;
        _PrimCacheTable::iterator it = _primCache.find(indexPath);
        if (it == _primCache.end()) {
            // no need to do anything if our prim doesn't correspond to a
            // renderIndex entry
            continue;
        }

        const TfToken & primType = (*it).second.primType;

        if (GetRenderIndex().IsRprimTypeSupported(primType)) {
            HdDirtyBits dirtyBits = 0;
            if (entry.dirtyLocators == _cachedLocatorSet &&
                primType == _cachedPrimType) {
                dirtyBits = _cachedDirtyBits;
            } else {
                dirtyBits = HdDirtyBitsTranslator::RprimLocatorSetToDirtyBits(
                    primType, entry.dirtyLocators);
                _cachedLocatorSet = entry.dirtyLocators;
                _cachedPrimType = primType;
                _cachedDirtyBits = dirtyBits;
            }
            if (dirtyBits != HdChangeTracker::Clean) {
                GetRenderIndex().GetChangeTracker()._MarkRprimDirty(
                    indexPath, dirtyBits);
            }
        } else if (GetRenderIndex().IsSprimTypeSupported(primType)) {
            HdDirtyBits dirtyBits =
                HdDirtyBitsTranslator::SprimLocatorSetToDirtyBits(
                        primType, entry.dirtyLocators);
            if (dirtyBits != HdChangeTracker::Clean) {
                GetRenderIndex().GetChangeTracker()._MarkSprimDirty(
                    indexPath, dirtyBits);
            }
        } else if (GetRenderIndex().IsBprimTypeSupported(primType)) {
            HdDirtyBits dirtyBits =
                HdDirtyBitsTranslator::BprimLocatorSetToDirtyBits(
                        primType, entry.dirtyLocators);
            if (dirtyBits != HdChangeTracker::Clean) {
                GetRenderIndex().GetChangeTracker()._MarkBprimDirty(
                    indexPath, dirtyBits);
            }
        } else if (primType == HdPrimTypeTokens->instancer) {
            HdDirtyBits dirtyBits =
                HdDirtyBitsTranslator::InstancerLocatorSetToDirtyBits(
                        primType, entry.dirtyLocators);
            if (dirtyBits != HdChangeTracker::Clean) {
                GetRenderIndex().GetChangeTracker()._MarkInstancerDirty(
                    indexPath, dirtyBits);
            }
        }

        if (entry.dirtyLocators.Intersects(
                HdPrimvarsSchema::GetDefaultLocator())) {
            it->second.primvarDescriptors.clear();
            it->second.primvarDescriptorsState.store(
                _PrimCacheEntry::ReadStateUnread);
        }

        if (entry.dirtyLocators.Intersects(
                HdExtComputationPrimvarsSchema::GetDefaultLocator())) {
            it->second.extCmpPrimvarDescriptors.clear();
            it->second.extCmpPrimvarDescriptorsState.store(
                _PrimCacheEntry::ReadStateUnread);
        }
    }
}

// ----------------------------------------------------------------------------

HdMeshTopology
HdSceneIndexAdapterSceneDelegate::GetMeshTopology(SdfPath const &id)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(id);


    HdMeshSchema meshSchema = HdMeshSchema::GetFromParent(prim.dataSource);


    HdMeshTopologySchema meshTopologySchema = meshSchema.GetTopology();
    if (!meshTopologySchema.IsDefined()) {
        return HdMeshTopology();
    }

    HdIntArrayDataSourceHandle faceVertexCountsDataSource = 
            meshTopologySchema.GetFaceVertexCounts();

    HdIntArrayDataSourceHandle faceVertexIndicesDataSource = 
            meshTopologySchema.GetFaceVertexIndices();

    if (!faceVertexCountsDataSource || !faceVertexIndicesDataSource) {
        return HdMeshTopology();
    }

    TfToken scheme = PxOsdOpenSubdivTokens->none;
    if (HdTokenDataSourceHandle schemeDs = 
            meshSchema.GetSubdivisionScheme()) {
        scheme = schemeDs->GetTypedValue(0.0f);
    }

    VtIntArray holeIndices;
    if (HdIntArrayDataSourceHandle holeDs =
            meshTopologySchema.GetHoleIndices()) {
        holeIndices = holeDs->GetTypedValue(0.0f);
    }

    TfToken orientation = PxOsdOpenSubdivTokens->rightHanded;
    if (HdTokenDataSourceHandle orientDs =
            meshTopologySchema.GetOrientation()) {
        orientation = orientDs->GetTypedValue(0.0f);
    }

    HdMeshTopology meshTopology(
        scheme,
        orientation,
        faceVertexCountsDataSource->GetTypedValue(0.0f),
        faceVertexIndicesDataSource->GetTypedValue(0.0f),
        holeIndices);

    HdGeomSubsetsSchema geomSubsets = meshSchema.GetGeomSubsets();
    if (geomSubsets.IsDefined()) {
        HdGeomSubsets geomSubsetsVec;
        for (const TfToken &id : geomSubsets.GetIds()) {
            HdGeomSubsetSchema gsSchema = geomSubsets.GetGeomSubset(id);
            if (!gsSchema.IsDefined()) {
                continue;
            }

            if (HdTokenDataSourceHandle typeDs = gsSchema.GetType()) {
                TfToken typeToken = typeDs->GetTypedValue(0.0f);

                HdIntArrayDataSourceHandle invisIndicesDs;

                if (HdVisibilitySchema visSchema =
                        HdVisibilitySchema::GetFromParent(
                            gsSchema.GetContainer())) {
                    if (HdBoolDataSourceHandle visDs =
                            visSchema.GetVisibility()) {
                        if (visDs->GetTypedValue(0.0f) == false) {
                            invisIndicesDs = gsSchema.GetIndices();
                        }
                    }
                }

                if (invisIndicesDs) {
                    // TODO, Combine possible multiple invisible element
                    //       arrays. Not relevant for front-end emulation.
                    if (typeToken == HdGeomSubsetSchemaTokens->typeFaceSet) {
                        meshTopology.SetInvisibleFaces(
                            invisIndicesDs->GetTypedValue(0.0f));
                    } else if (typeToken ==
                            HdGeomSubsetSchemaTokens->typePointSet) {
                        meshTopology.SetInvisiblePoints(
                            invisIndicesDs->GetTypedValue(0.0f));
                    }
                    // don't include invisible elements in the geom subset
                    // entries below.
                    continue;
                }

            } else {
                // no type? don't include
                continue;
            }

            SdfPath materialId = SdfPath();
            HdMaterialBindingSchema materialBinding = 
                HdMaterialBindingSchema::GetFromParent(gsSchema.GetContainer());
            if (materialBinding.IsDefined()) {
                if (HdPathDataSourceHandle materialIdDs = 
                    materialBinding.GetMaterialBinding()) {
                    materialId = materialIdDs->GetTypedValue(0.0f);
                }
            }

            VtIntArray indices = VtIntArray(0);
            if (HdIntArrayDataSourceHandle indicesDs = 
                gsSchema.GetIndices()) {
                indices = indicesDs->GetTypedValue(0.0f);
            }

            HdGeomSubset geomSubset = { HdGeomSubset::TypeFaceSet, 
                SdfPath(id.GetText()), materialId, indices };
            geomSubsetsVec.push_back(geomSubset);
        }
        meshTopology.SetGeomSubsets(geomSubsetsVec);
    }

    return meshTopology;
}

bool
HdSceneIndexAdapterSceneDelegate::GetDoubleSided(SdfPath const &id)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(id);

    HdMeshSchema meshSchema = 
        HdMeshSchema::GetFromParent(prim.dataSource);
    if (!meshSchema.IsDefined()) {
        return false;
    }

    HdBoolDataSourceHandle doubleSidedDs = meshSchema.GetDoubleSided();
    if (!doubleSidedDs) {
        return false;
    }

    return doubleSidedDs->GetTypedValue(0.0f);
}

GfRange3d
HdSceneIndexAdapterSceneDelegate::GetExtent(SdfPath const &id)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(id);

    HdExtentSchema extentSchema =
        HdExtentSchema::GetFromParent(prim.dataSource);
    if (!extentSchema.IsDefined()) {
        return GfRange3d();
    }

    GfVec3d min, max;
    if (HdVec3dDataSourceHandle minDs = extentSchema.GetMin()) {
        min = minDs->GetTypedValue(0);
    }
    if (HdVec3dDataSourceHandle maxDs = extentSchema.GetMax()) {
        max = maxDs->GetTypedValue(0);
    }

    return GfRange3d(min, max);
}

bool
HdSceneIndexAdapterSceneDelegate::GetVisible(SdfPath const &id)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(id);

    HdVisibilitySchema visibilitySchema =
        HdVisibilitySchema::GetFromParent(prim.dataSource);
    if (!visibilitySchema.IsDefined()) {
        return true; // default visible
    }

    HdBoolDataSourceHandle visDs = visibilitySchema.GetVisibility();
    if (!visDs) {
        return true;
    }
    return visDs->GetTypedValue(0);
}

TfToken
HdSceneIndexAdapterSceneDelegate::GetRenderTag(SdfPath const &id)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(id);

    HdPurposeSchema purposeSchema =
        HdPurposeSchema::GetFromParent(prim.dataSource);
    if (!purposeSchema.IsDefined()) {
        return HdRenderTagTokens->geometry; // default render tag.
    }

    HdTokenDataSourceHandle purposeDs = purposeSchema.GetPurpose();
    if (!purposeDs) {
        return HdRenderTagTokens->geometry;
    }
    return purposeDs->GetTypedValue(0);
}

PxOsdSubdivTags
HdSceneIndexAdapterSceneDelegate::GetSubdivTags(SdfPath const &id)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(id);

    PxOsdSubdivTags tags;

    HdMeshSchema meshSchema = HdMeshSchema::GetFromParent(prim.dataSource);
    if (!meshSchema.IsDefined()) {
        return tags;
    }

    HdSubdivisionTagsSchema subdivTagsSchema = meshSchema.GetSubdivisionTags();
    if (!subdivTagsSchema.IsDefined()) {
        return tags;
    }

    if (HdTokenDataSourceHandle fvliDs =
            subdivTagsSchema.GetFaceVaryingLinearInterpolation()) {
        tags.SetFaceVaryingInterpolationRule(fvliDs->GetTypedValue(0.0f));
    }

    if (HdTokenDataSourceHandle ibDs =
            subdivTagsSchema.GetInterpolateBoundary()) {
        tags.SetVertexInterpolationRule(ibDs->GetTypedValue(0.0f));
    }

    if (HdTokenDataSourceHandle tsrDs =
            subdivTagsSchema.GetTriangleSubdivisionRule()) {
        tags.SetTriangleSubdivision(tsrDs->GetTypedValue(0.0f));
    }

    if (HdIntArrayDataSourceHandle cniDs =
            subdivTagsSchema.GetCornerIndices()) {
        tags.SetCornerIndices(cniDs->GetTypedValue(0.0f));
    }

    if (HdFloatArrayDataSourceHandle cnsDs =
            subdivTagsSchema.GetCornerSharpnesses()) {
        tags.SetCornerWeights(cnsDs->GetTypedValue(0.0f));
    }

    if (HdIntArrayDataSourceHandle criDs =
            subdivTagsSchema.GetCreaseIndices()) {
        tags.SetCreaseIndices(criDs->GetTypedValue(0.0f));
    }

    if (HdIntArrayDataSourceHandle crlDs =
            subdivTagsSchema.GetCreaseLengths()) {
        tags.SetCreaseLengths(crlDs->GetTypedValue(0.0f));
    }

    if (HdFloatArrayDataSourceHandle crsDs =
            subdivTagsSchema.GetCreaseSharpnesses()) {
        tags.SetCreaseWeights(crsDs->GetTypedValue(0.0f));
    }

    return tags;
}

HdBasisCurvesTopology 
HdSceneIndexAdapterSceneDelegate::GetBasisCurvesTopology(SdfPath const &id)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(id);

    HdBasisCurvesSchema basisCurvesSchema =
        HdBasisCurvesSchema::GetFromParent(prim.dataSource);

    HdBasisCurvesTopologySchema bcTopologySchema =
        basisCurvesSchema.GetTopology();

    if (!bcTopologySchema.IsDefined()) {
        return HdBasisCurvesTopology();
    }

    HdIntArrayDataSourceHandle curveVertexCountsDataSource = 
            bcTopologySchema.GetCurveVertexCounts();

    if (!curveVertexCountsDataSource) {
        return HdBasisCurvesTopology();
    }

    VtIntArray curveIndices;
    HdIntArrayDataSourceHandle curveIndicesDataSource =
        bcTopologySchema.GetCurveIndices();
    if (curveIndicesDataSource) {
        curveIndices = curveIndicesDataSource->GetTypedValue(0.0f);
    }

    TfToken basis = HdTokens->bezier;
    HdTokenDataSourceHandle basisDataSource = bcTopologySchema.GetBasis();
    if (basisDataSource) {
        basis = basisDataSource->GetTypedValue(0.0f);
    }

    TfToken type = HdTokens->linear;
    HdTokenDataSourceHandle typeDataSource = bcTopologySchema.GetType();
    if (typeDataSource) {
        type = typeDataSource->GetTypedValue(0.0f);
    }

    TfToken wrap = HdTokens->nonperiodic;
    HdTokenDataSourceHandle wrapDataSource = bcTopologySchema.GetWrap();
    if (wrapDataSource) {
        wrap = wrapDataSource->GetTypedValue(0.0f);
    }

    HdBasisCurvesTopology result(
        type, basis, wrap,
        curveVertexCountsDataSource->GetTypedValue(0.0f),
        curveIndices);

    HdGeomSubsetsSchema geomSubsets = basisCurvesSchema.GetGeomSubsets();

    if (geomSubsets.IsDefined()) {

        HdGeomSubsets geomSubsetsVec;
        for (const TfToken &id : geomSubsets.GetIds()) {
            HdGeomSubsetSchema gsSchema = geomSubsets.GetGeomSubset(id);
            if (!gsSchema.IsDefined()) {
                continue;
            }

            if (HdTokenDataSourceHandle typeDs = gsSchema.GetType()) {
                TfToken typeToken = typeDs->GetTypedValue(0.0f);

                HdIntArrayDataSourceHandle invisIndicesDs;

                if (HdVisibilitySchema visSchema =
                        HdVisibilitySchema::GetFromParent(
                            gsSchema.GetContainer())) {
                    if (HdBoolDataSourceHandle visDs =
                            visSchema.GetVisibility()) {
                        if (visDs->GetTypedValue(0.0f) == false) {
                            invisIndicesDs = gsSchema.GetIndices();
                        }
                    }
                }

                if (invisIndicesDs) {
                    // TODO, Combine possible multiple invisible element
                    //       arrays. Not relevant for front-end emulation.
                    if (typeToken == HdGeomSubsetSchemaTokens->typeCurveSet) {
                        result.SetInvisibleCurves(
                            invisIndicesDs->GetTypedValue(0.0f));
                    } else if (
                        typeToken == HdGeomSubsetSchemaTokens->typePointSet) {
                        result.SetInvisiblePoints(
                            invisIndicesDs->GetTypedValue(0.0f));
                    }
                    // don't include invisible elements in the geom subset
                    // entries below.
                    continue;
                }
            }
        }
    }

    return result;
}

VtArray<TfToken>
HdSceneIndexAdapterSceneDelegate::GetCategories(SdfPath const &id)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(id);

    static const VtArray<TfToken> emptyResult;

    HdCategoriesSchema categoriesSchema = 
        HdCategoriesSchema::GetFromParent(
            prim.dataSource);

    if (!categoriesSchema.IsDefined()) {
        return emptyResult;
    }

    return categoriesSchema.GetIncludedCategoryNames();
}

HdVolumeFieldDescriptorVector
HdSceneIndexAdapterSceneDelegate::GetVolumeFieldDescriptors(
        SdfPath const &volumeId)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(volumeId);

    HdVolumeFieldDescriptorVector result;
    HdVolumeFieldBindingSchema bindingSchema =
        HdVolumeFieldBindingSchema::GetFromParent(prim.dataSource);
    if (!bindingSchema.IsDefined()) {
        return result;
    }

    TfTokenVector names = bindingSchema.GetContainer()->GetNames();
    for (const TfToken& name : names) {
        HdPathDataSourceHandle pathDs =
            bindingSchema.GetVolumeFieldBinding(name);
        if (!pathDs) {
            continue;
        }

        HdVolumeFieldDescriptor desc;
        desc.fieldName = name;
        desc.fieldId = pathDs->GetTypedValue(0);

        // XXX: Kind of a hacky way to get the prim type for the old API.
        HdSceneIndexPrim fieldPrim = _inputSceneIndex->GetPrim(desc.fieldId);
        if (!fieldPrim.dataSource) {
            continue;
        }
        desc.fieldPrimType = fieldPrim.primType;

        result.push_back(desc);
    }

    return result;
}

SdfPath 
HdSceneIndexAdapterSceneDelegate::GetMaterialId(SdfPath const & id)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(id);

    HdMaterialBindingSchema mat = HdMaterialBindingSchema::GetFromParent(
        prim.dataSource);
    if (!mat.IsDefined()) {
        return SdfPath();
    }

    HdPathDataSourceHandle bindingDs = mat.GetMaterialBinding();
    if (!bindingDs) {
        return SdfPath();
    }

    return bindingDs->GetTypedValue(0);
}

HdIdVectorSharedPtr
HdSceneIndexAdapterSceneDelegate::GetCoordSysBindings(SdfPath const &id)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(id);

    HdCoordSysBindingSchema coordSys = HdCoordSysBindingSchema::GetFromParent(
        prim.dataSource);
    if (!coordSys.IsDefined()) {
        return nullptr;
    }

    HdIdVectorSharedPtr idVec = HdIdVectorSharedPtr(new SdfPathVector());
    TfTokenVector names = coordSys.GetContainer()->GetNames();
    for (const TfToken& name : names) {
        HdPathDataSourceHandle pathDs =
            coordSys.GetCoordSysBinding(name);
        if (!pathDs) {
            continue;
        }

        idVec->push_back(pathDs->GetTypedValue(0));
    }

    return idVec;
}

HdRenderBufferDescriptor
HdSceneIndexAdapterSceneDelegate::GetRenderBufferDescriptor(SdfPath const &id)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(id);
    HdRenderBufferDescriptor desc;

    HdRenderBufferSchema rb = HdRenderBufferSchema::GetFromParent(
        prim.dataSource);
    if (!rb.IsDefined()) {
        return desc;
    }

    HdVec3iDataSourceHandle dim = rb.GetDimensions();
    if (dim) {
        desc.dimensions = dim->GetTypedValue(0);
    }

    HdFormatDataSourceHandle fmt = rb.GetFormat();
    if (fmt) {
        desc.format = fmt->GetTypedValue(0);
    }

    HdBoolDataSourceHandle ms = rb.GetMultiSampled();
    if (ms) {
        desc.multiSampled = ms->GetTypedValue(0);
    }

    return desc;
}

static
void 
_Walk(
    const SdfPath & nodePath, 
    const HdContainerDataSourceHandle & nodesDS,
    std::unordered_set<SdfPath, SdfPath::Hash> * visitedSet,
    HdMaterialNetwork * netHd)
{
    if (visitedSet->find(nodePath) != visitedSet->end()) {
        return;
    }

    visitedSet->insert(nodePath);

    TfToken nodePathTk(nodePath.GetToken());
    if (!nodesDS->Has(nodePathTk)){ 
        return;
    }

    HdDataSourceBaseHandle nodeDS = nodesDS->Get(nodePathTk);
    HdMaterialNodeSchema nodeSchema(HdContainerDataSource::Cast(nodeDS));
    if (!nodeSchema.IsDefined()) {
        return;
    }
    
    const TfToken nodeId = nodeSchema.GetNodeIdentifier()->GetTypedValue(0);
    HdContainerDataSourceHandle connsDS = nodeSchema.GetInputConnections();
    HdContainerDataSourceHandle paramsDS = nodeSchema.GetParameters();

    if (connsDS) {
        const TfTokenVector connsNames = connsDS->GetNames();
        for (const auto & connName : connsNames) {
            HdVectorDataSourceHandle allConnDS = 
                HdVectorDataSource::Cast(connsDS->Get(connName));
            
            if (!allConnDS) {
                continue;
            }
            
            for (size_t i = 0 ; i < allConnDS->GetNumElements() ; i++) {
                HdDataSourceBaseHandle connDS = allConnDS->GetElement(i);
                
                HdMaterialConnectionSchema connSchema(
                    HdContainerDataSource::Cast(connDS));
                if (!connSchema.IsDefined()) {
                    continue;
                }
            
                TfToken p = connSchema.GetUpstreamNodePath()->GetTypedValue(0);
                TfToken n = 
                    connSchema.GetUpstreamNodeOutputName()->GetTypedValue(0);
                _Walk(SdfPath(p.GetString()), nodesDS, visitedSet, netHd);

                HdMaterialRelationship r;
                r.inputId = SdfPath(p.GetString()); 
                r.inputName = n; 
                r.outputId = nodePath; 
                r.outputName=connName;
                netHd->relationships.push_back(r);
            }
        }
    }

    std::map<TfToken, VtValue> paramsHd;
    if (paramsDS) {
        const TfTokenVector pNames = paramsDS->GetNames();
        for (const auto & pName : pNames) {
            HdDataSourceBaseHandle paramDS = paramsDS->Get(pName);
            HdSampledDataSourceHandle paramSDS = 
                HdSampledDataSource::Cast(paramDS);
            if (paramSDS) {
                VtValue v = paramSDS->GetValue(0);
                paramsHd[pName] = v;
            }
        }
    }

    HdMaterialNode n;
    n.identifier = nodeId;
    n.path = nodePath;
    n.parameters = paramsHd;
    netHd->nodes.push_back(n);
}

VtValue 
HdSceneIndexAdapterSceneDelegate::GetMaterialResource(SdfPath const & id)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(id);

    HdMaterialSchema matSchema = HdMaterialSchema::GetFromParent(
            prim.dataSource);
    if (!matSchema.IsDefined()) {
        return VtValue();
    }

    TfToken networkSelector =
        GetRenderIndex().GetRenderDelegate()->GetMaterialNetworkSelector();
    HdContainerDataSourceHandle matDS =
        matSchema.GetMaterialNetwork(networkSelector);
    HdMaterialNetworkSchema netSchema = HdMaterialNetworkSchema(matDS);
    if (!netSchema.IsDefined()) {
        return VtValue();
    }

    // Convert HdDataSource with material data to HdMaterialNetworkMap
    HdMaterialNetworkMap matHd;

    // List of visited nodes to facilitate network traversal
    std::unordered_set<SdfPath, SdfPath::Hash> visitedNodes;

    HdContainerDataSourceHandle nodesDS = netSchema.GetNodes();
    HdContainerDataSourceHandle terminalsDS = netSchema.GetTerminals();
    const TfTokenVector names = terminalsDS->GetNames();
   
    for (const auto & name : names) {
        visitedNodes.clear();
        
        // Extract connections one by one
        HdDataSourceBaseHandle connDS = terminalsDS->Get(name);
        HdMaterialConnectionSchema connSchema(
            HdContainerDataSource::Cast(connDS));
        if (!connSchema.IsDefined()) {
            continue;
        }

        // Keep track of the terminals
        TfToken pathTk = connSchema.GetUpstreamNodePath()->GetTypedValue(0);
        SdfPath path(pathTk.GetString());
        matHd.terminals.push_back(path);

        // Continue walking the network
        HdMaterialNetwork & netHd = matHd.map[name];
        _Walk(path, nodesDS, &visitedNodes, &netHd);
    }
    return VtValue(matHd);
}

VtValue
HdSceneIndexAdapterSceneDelegate::GetCameraParamValue(
        SdfPath const &cameraId, TfToken const &paramName)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(cameraId);
    if (!prim.dataSource) {
        return VtValue();
    }

    HdContainerDataSourceHandle camera =
        HdContainerDataSource::Cast(
            prim.dataSource->Get(HdCameraSchemaTokens->camera));
    if (!camera) {
        return VtValue();
    }

    HdSampledDataSourceHandle valueDs =
        HdSampledDataSource::Cast(
            camera->Get(paramName));
    if (!valueDs) {
        return VtValue();
    }

    VtValue value = valueDs->GetValue(0);
    // Smooth out some incompatibilities between scene delegate and
    // datasource schemas...
    if (paramName == HdCameraSchemaTokens->projection) {
        TfToken proj = HdCameraSchemaTokens->perspective;
        if (value.IsHolding<TfToken>()) {
            proj = value.UncheckedGet<TfToken>();
        }
        return VtValue(proj == HdCameraSchemaTokens->perspective ?
                HdCamera::Perspective :
                HdCamera::Orthographic);
    } else if (paramName == HdCameraSchemaTokens->clippingRange) {
        GfVec2f range(0);
        if (value.IsHolding<GfVec2f>()) {
            range = value.UncheckedGet<GfVec2f>();
        }
        return VtValue(GfRange1f(range[0], range[1]));
    } else {
        return value;
    }
}

VtValue
HdSceneIndexAdapterSceneDelegate::GetLightParamValue(
        SdfPath const &id, TfToken const &paramName)
{
    TRACE_FUNCTION();

    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(id);
    if (!prim.dataSource) {
        return VtValue();
    }

    HdContainerDataSourceHandle light =
        HdContainerDataSource::Cast(
            prim.dataSource->Get(HdLightSchemaTokens->light));
    if (!light) {
        return VtValue();
    }

    HdSampledDataSourceHandle valueDs =
        HdSampledDataSource::Cast(
            light->Get(paramName));
    if (!valueDs) {
        return VtValue();
    }

    return valueDs->GetValue(0);
}


static
HdInterpolation
Hd_InterpolationAsEnum(const TfToken &interpolationToken)
{
    if (interpolationToken == HdPrimvarSchemaTokens->constant) {
        return HdInterpolationConstant;
    } else if (interpolationToken == HdPrimvarSchemaTokens->uniform) {
        return HdInterpolationUniform;
    } else if (interpolationToken == HdPrimvarSchemaTokens->varying) {
        return HdInterpolationVarying;
    } else if (interpolationToken == HdPrimvarSchemaTokens->vertex) {
        return HdInterpolationVertex;
    } else if (interpolationToken == HdPrimvarSchemaTokens->faceVarying) {
        return HdInterpolationFaceVarying;
    } else if (interpolationToken == HdPrimvarSchemaTokens->instance) {
        return HdInterpolationInstance;
    }

    return HdInterpolation(-1);
}


HdPrimvarDescriptorVector
HdSceneIndexAdapterSceneDelegate::GetPrimvarDescriptors(
    SdfPath const &id, HdInterpolation interpolation)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    HdPrimvarDescriptorVector result;

    _PrimCacheTable::iterator it = _primCache.find(id);
    if (it == _primCache.end()) {
        return result;
    }

    if (it->second.primvarDescriptorsState.load() ==
            _PrimCacheEntry::ReadStateRead) {
        return it->second.primvarDescriptors[interpolation];
    }

    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(id);
    if (!prim.dataSource) {
        it->second.primvarDescriptorsState.store(
            _PrimCacheEntry::ReadStateRead);
        return result;
    }

    std::map<HdInterpolation, HdPrimvarDescriptorVector> descriptors;
    if (HdPrimvarsSchema primvars =
        HdPrimvarsSchema::GetFromParent(prim.dataSource)) {
        for (const TfToken &name : primvars.GetPrimvarNames()) {
            HdPrimvarSchema primvar = primvars.GetPrimvar(name);
            if (!primvar) {
                continue;
            }

            HdTokenDataSourceHandle interpolationDataSource =
                primvar.GetInterpolation();

            if (!interpolationDataSource) {
                continue;
            }

            TfToken interpolationToken =
                interpolationDataSource->GetTypedValue(0.0f);
            HdInterpolation interpolation =
                Hd_InterpolationAsEnum(interpolationToken);

            TfToken roleToken;
            if (HdTokenDataSourceHandle roleDataSource =
                    primvar.GetRole()) {
                roleToken = roleDataSource->GetTypedValue(0.0f);
            }

            bool indexed = primvar.IsIndexed();

            descriptors[interpolation].push_back(
                {name, interpolation, roleToken, indexed});
        }
    }

    _PrimCacheEntry::ReadState current = _PrimCacheEntry::ReadStateUnread;
    if (it->second.primvarDescriptorsState.compare_exchange_strong(
            current, _PrimCacheEntry::ReadStateReading)) {
        it->second.primvarDescriptors = std::move(descriptors);
        it->second.primvarDescriptorsState.store(
            _PrimCacheEntry::ReadStateRead);

        return it->second.primvarDescriptors[interpolation];
    }

    return descriptors[interpolation];
}

HdExtComputationPrimvarDescriptorVector
HdSceneIndexAdapterSceneDelegate::GetExtComputationPrimvarDescriptors(
    SdfPath const &id, HdInterpolation interpolation)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    HdExtComputationPrimvarDescriptorVector result;

    _PrimCacheTable::iterator it = _primCache.find(id);
    if (it == _primCache.end()) {
        return result;
    }

    if (it->second.extCmpPrimvarDescriptorsState.load() ==
            _PrimCacheEntry::ReadStateRead) {
        return it->second.extCmpPrimvarDescriptors[interpolation];
    }

    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(id);
    if (!prim.dataSource) {
        it->second.extCmpPrimvarDescriptorsState.store(
            _PrimCacheEntry::ReadStateRead);
        return result;
    }

    std::map<HdInterpolation, HdExtComputationPrimvarDescriptorVector>
        descriptors;
    if (HdExtComputationPrimvarsSchema primvars =
        HdExtComputationPrimvarsSchema::GetFromParent(prim.dataSource)) {
        for (const TfToken &name : primvars.GetExtComputationPrimvarNames()) {
            HdExtComputationPrimvarSchema primvar = primvars.GetPrimvar(name);
            if (!primvar) {
                continue;
            }

            HdTokenDataSourceHandle interpolationDataSource =
                primvar.GetInterpolation();
            if (!interpolationDataSource) {
                continue;
            }

            TfToken interpolationToken =
                interpolationDataSource->GetTypedValue(0.0f);
            HdInterpolation interpolation =
                Hd_InterpolationAsEnum(interpolationToken);

            TfToken roleToken;
            if (HdTokenDataSourceHandle roleDataSource =
                    primvar.GetRole()) {
                roleToken = roleDataSource->GetTypedValue(0.0f);
            }

            SdfPath sourceComputation;
            if (HdPathDataSourceHandle sourceComputationDs =
                    primvar.GetSourceComputation()) {
                sourceComputation = sourceComputationDs->GetTypedValue(0.0f);
            }

            TfToken sourceComputationOutputName;
            if (HdTokenDataSourceHandle sourceComputationOutputDs =
                    primvar.GetSourceComputationOutputName()) {
                sourceComputationOutputName =
                    sourceComputationOutputDs->GetTypedValue(0.0f);
            }

            HdTupleType valueType;
            if (HdTupleTypeDataSourceHandle valueTypeDs =
                    primvar.GetValueType()) {
                valueType = valueTypeDs->GetTypedValue(0.0f);
            }

            descriptors[interpolation].push_back(
                    {name, interpolation, roleToken, sourceComputation,
                     sourceComputationOutputName, valueType});
        }
    }

    if (it->second.extCmpPrimvarDescriptorsState.load() ==
            _PrimCacheEntry::ReadStateUnread) {
        it->second.extCmpPrimvarDescriptorsState.store(
            _PrimCacheEntry::ReadStateReading);
        it->second.extCmpPrimvarDescriptors = std::move(descriptors);
        it->second.extCmpPrimvarDescriptorsState.store(
            _PrimCacheEntry::ReadStateRead);
    } else {
        // if someone is in the process of filling the entry, just
        // return our value instead of trying to assign
        return descriptors[interpolation];
    }
    
    return it->second.extCmpPrimvarDescriptors[interpolation];


}

VtValue 
HdSceneIndexAdapterSceneDelegate::Get(SdfPath const &id, TfToken const &key)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(id);
    if (!prim.dataSource) {
        return VtValue();
    }

    // simpleLight use of Get().
    if (prim.primType == HdPrimTypeTokens->simpleLight) {
        return GetLightParamValue(id, key);
    }

    // camera use of Get().
    if (prim.primType == HdPrimTypeTokens->camera) {
        return GetCameraParamValue(id, key);
    }

    // Temporary backdoor for getting arbitrary data to render delegates
    // Currently supported for setting Options and active integrator parameters
    // in hdPrman.
    if (prim.primType == _tokens->prmanParams) {
         HdContainerDataSourceHandle prmanParamsDs = HdContainerDataSource::Cast(
                prim.dataSource->Get(_tokens->prmanParams));

         if (!prmanParamsDs) {
            return VtValue();
         }

         if (key == _tokens->prmanParamsNames) {
            return VtValue(prmanParamsDs->GetNames());
         } else {
            if (HdContainerDataSourceHandle paramsDs =
                    HdContainerDataSource::Cast(
                        prmanParamsDs->Get(key))) {

                std::map<TfToken, VtValue> valueDict;
                for (const TfToken &name : paramsDs->GetNames()) {
                    if (HdSampledDataSourceHandle sampledDs =
                            HdSampledDataSource::Cast(paramsDs->Get(name))){
                        valueDict[name] = sampledDs->GetValue(0.0f);
                    }
                }
                return VtValue(valueDict);
            }
        }

        return VtValue();
    }

    // drawTarget use of Get().
    if (prim.primType == HdPrimTypeTokens->drawTarget) {
        if (HdContainerDataSourceHandle drawTarget =
                HdContainerDataSource::Cast(
                    prim.dataSource->Get(HdPrimTypeTokens->drawTarget))) {
            if (drawTarget->Has(key)) {
                if (HdSampledDataSourceHandle valueDs =
                        HdSampledDataSource::Cast(drawTarget->Get(key))) {
                    return valueDs->GetValue(0.0f);
                } 
            }
        }

        return VtValue();
    }

    // volume field use of Get().
    if (HdLegacyPrimTypeIsVolumeField(prim.primType)) {
        HdContainerDataSourceHandle volumeField =
            HdContainerDataSource::Cast(
                prim.dataSource->Get(HdVolumeFieldSchemaTokens->volumeField));
        if (!volumeField) {
            return VtValue();
        }

        HdSampledDataSourceHandle valueDs =
            HdSampledDataSource::Cast(
                volumeField->Get(key));
        if (!valueDs) {
            return VtValue();
        }

        return valueDs->GetValue(0);
    }

    // renderbuffer use of Get().
    if (prim.primType == HdPrimTypeTokens->renderBuffer) {
        if (HdContainerDataSourceHandle renderBuffer =
                HdContainerDataSource::Cast(
                    prim.dataSource->Get(
                        HdRenderBufferSchemaTokens->renderBuffer))) {
            if (renderBuffer->Has(key)) {
                if (HdSampledDataSourceHandle valueDs =
                        HdSampledDataSource::Cast(renderBuffer->Get(key))) {
                    return valueDs->GetValue(0);
                }
            }
        }

        return VtValue();
    }

    // "primvars" use of Get()
    if (HdContainerDataSource::Cast(prim.dataSource)->Has(
            HdPrimvarsSchemaTokens->primvars)) {
        return _GetPrimvar(id, key, nullptr);
    }

    // Fallback for unknown prim conventions provided by emulated scene
    // delegate.
    if (HdTypedSampledDataSource<HdSceneDelegate*>::Handle sdDs =
            HdTypedSampledDataSource<HdSceneDelegate*>::Cast(
                prim.dataSource->Get(
                    HdSceneIndexEmulationTokens->sceneDelegate))) {
        if (HdSceneDelegate *delegate = sdDs->GetTypedValue(0.0f)) {
            return delegate->Get(id, key);
        }
    }

    return VtValue();
}

VtValue 
HdSceneIndexAdapterSceneDelegate::GetIndexedPrimvar(SdfPath const &id, 
    TfToken const &key, VtIntArray *outIndices)
{
    return _GetPrimvar(id, key, outIndices);
}

VtValue 
HdSceneIndexAdapterSceneDelegate::_GetPrimvar(SdfPath const &id, 
    TfToken const &key, VtIntArray *outIndices)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    if (outIndices) {
        outIndices->clear();
    }
    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(id);
    if (!prim.dataSource) {
        return VtValue();
    }

    if (HdPrimvarsSchema primvars = HdPrimvarsSchema::GetFromParent(
            prim.dataSource)) {
        if (HdPrimvarSchema primvar = primvars.GetPrimvar(key)) {
            
            if (outIndices) {
                if (HdSampledDataSourceHandle valueDataSource =
                    primvar.GetIndexedPrimvarValue()) {
                    if (HdIntArrayDataSourceHandle 
                        indicesDataSource = primvar.GetIndices()) {
                        *outIndices = indicesDataSource->GetTypedValue(0.f);
                    }
                    return valueDataSource->GetValue(0.0f);
                }
            } else {
                if (HdSampledDataSourceHandle valueDataSource =
                    primvar.GetPrimvarValue()) {
                    return valueDataSource->GetValue(0.0f);
                }
            }
        }
    }

    return VtValue();
}

size_t
HdSceneIndexAdapterSceneDelegate::SamplePrimvar(
        SdfPath const &id, TfToken const &key,
        size_t maxSampleCount, float *sampleTimes, 
        VtValue *sampleValues)
{
    return _SamplePrimvar(id, key, maxSampleCount, sampleTimes, sampleValues,
        nullptr);
}

size_t
HdSceneIndexAdapterSceneDelegate::SampleIndexedPrimvar(
        SdfPath const &id, TfToken const &key,
        size_t maxSampleCount, float *sampleTimes, 
        VtValue *sampleValues, VtIntArray *sampleIndices)
{
    return _SamplePrimvar(id, key, maxSampleCount, sampleTimes, sampleValues,
        sampleIndices);
}

size_t
HdSceneIndexAdapterSceneDelegate::_SamplePrimvar(
        SdfPath const &id, TfToken const &key,
        size_t maxSampleCount, float *sampleTimes, 
        VtValue *sampleValues, VtIntArray *sampleIndices)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(id);

    HdSampledDataSourceHandle valueSource = nullptr;
    HdIntArrayDataSourceHandle indicesSource = nullptr;
    
    HdPrimvarsSchema primvars =
        HdPrimvarsSchema::GetFromParent(prim.dataSource);
    if (primvars) {
        HdPrimvarSchema primvar = primvars.GetPrimvar(key);
        if (primvar) {
            if (sampleIndices) {
                valueSource = primvar.GetIndexedPrimvarValue();
                indicesSource = primvar.GetIndices();
            } else {
                valueSource = primvar.GetPrimvarValue();
            }
        }
    }

    // NOTE: SamplePrimvar is used by some render delegates to get multiple
    //       samples from camera parameters. While this works from UsdImaging,
    //       it's not due to intentional scene delegate specification but
    //       by UsdImaging fallback behavior which goes directly to USD attrs
    //       in absence of a matching primvar.
    //       In order to support legacy uses of this, we will also check
    //       camera parameter datasources
    if (!valueSource && prim.primType == HdPrimTypeTokens->camera) {
        if (HdCameraSchema cameraSchema =
                HdCameraSchema::GetFromParent(prim.dataSource)) {
            // Ask for the key directly from the schema's container data source
            // as immediate child data source names match the legacy camera
            // parameter names (e.g. focalLength)
            // For a native data source, this will naturally have time samples.
            // For a emulated data source, we are accounting for the possibly
            // that it needs to call SamplePrimvar
            valueSource = HdSampledDataSource::Cast(
                cameraSchema.GetContainer()->Get(key));
        }
    }

    if (!valueSource) {
        return 0;
    }

    std::vector<HdSampledDataSource::Time> times;
    // XXX: If the input prim is a legacy prim, the scene delegate is
    // responsible for setting the shutter window.  We can't query it, but
    // we pass the infinite window to accept all time samples from the
    // scene delegate.
    //
    // If the input prim is a datasource prim, we need some sensible default
    // here...  For now, we pass [0,0] to turn off multisampling.
    if (prim.dataSource->Has(HdSceneIndexEmulationTokens->sceneDelegate)) {
        valueSource->GetContributingSampleTimesForInterval(
                std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::max(), &times);
    } else {
        valueSource->GetContributingSampleTimesForInterval(
                0, 0, &times);
    }

    size_t authoredSamples = times.size();
    if (authoredSamples > maxSampleCount) {
        times.resize(maxSampleCount);
    }

    // XXX fallback to include a single sample
    if (times.empty()) {
        times.push_back(0.0f);
    }

    for (size_t i = 0; i < times.size(); ++i) {
        sampleTimes[i] = times[i];
        sampleValues[i] = valueSource->GetValue(times[i]);
        if (sampleIndices) {
            if (indicesSource) {
                // Can assume indices source has same sample times as primvar 
                // value source.
                sampleIndices[i] = indicesSource->GetTypedValue(times[i]);
            } else {
                sampleIndices[i].clear();
            }
        }
        
    }

    return authoredSamples;
}

GfMatrix4d
HdSceneIndexAdapterSceneDelegate::GetTransform(SdfPath const & id)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    GfMatrix4d m;
    m.SetIdentity();

    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(id);

    if (HdXformSchema xformSchema = HdXformSchema::GetFromParent(
            prim.dataSource)) {
        if (HdMatrixDataSourceHandle matrixSource =
                xformSchema.GetMatrix()) {

            m = matrixSource->GetTypedValue(0.0f);
        }
    }

    return m;
}

GfMatrix4d
HdSceneIndexAdapterSceneDelegate::GetInstancerTransform(SdfPath const & id)
{
    return GetTransform(id);
}

size_t
HdSceneIndexAdapterSceneDelegate::SampleTransform(
        SdfPath const &id, size_t maxSampleCount,
        float *sampleTimes, GfMatrix4d *sampleValues)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(id);

    HdXformSchema xformSchema =
        HdXformSchema::GetFromParent(prim.dataSource);
    if (!xformSchema) {
        return 0;
    }
    HdMatrixDataSourceHandle matrixSource = xformSchema.GetMatrix();
    if (!matrixSource) {
        return 0;
    }

    std::vector<HdSampledDataSource::Time> times;
    // XXX: If the input prim is a legacy prim, the scene delegate is
    // responsible for setting the shutter window.  We can't query it, but
    // we pass the infinite window to accept all time samples from the
    // scene delegate.
    //
    // If the input prim is a datasource prim, we need some sensible default
    // here...  For now, we pass [0,0] to turn off multisampling.
    if (prim.dataSource->Has(HdSceneIndexEmulationTokens->sceneDelegate)) {
        matrixSource->GetContributingSampleTimesForInterval(
                std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::max(), &times);
    } else {
        matrixSource->GetContributingSampleTimesForInterval(
                0, 0, &times);
    }

    // XXX fallback to include a single sample
    if (times.empty()) {
        times.push_back(0.0f);
    }

    size_t authoredSamples = times.size();
    if (authoredSamples > maxSampleCount) {
        times.resize(maxSampleCount);
    }

    for (size_t i = 0; i < times.size(); ++i) {
        sampleTimes[i] = times[i];
        sampleValues[i] = matrixSource->GetTypedValue(times[i]);
    }

    return authoredSamples;
}

size_t
HdSceneIndexAdapterSceneDelegate::SampleInstancerTransform(
        SdfPath const &id, size_t maxSampleCount,
        float *sampleTimes, GfMatrix4d *sampleValues)
{
    return SampleTransform(id, maxSampleCount, sampleTimes, sampleValues);
}


std::vector<VtArray<TfToken>>
HdSceneIndexAdapterSceneDelegate::GetInstanceCategories(
    SdfPath const &instancerId) {
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    std::vector<VtArray<TfToken>> result;

    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(instancerId);

    if (HdInstanceCategoriesSchema instanceCategories =
            HdInstanceCategoriesSchema::GetFromParent(prim.dataSource)) {

        if (HdVectorDataSourceHandle values =
                instanceCategories.GetCategoriesValues())
        {
            static const VtArray<TfToken> emptyValue;
            if (values) {
                result.reserve(values->GetNumElements());
                for (size_t i = 0, e = values->GetNumElements(); i != e; ++i) {

                    if (HdCategoriesSchema value = HdContainerDataSource::Cast(
                            values->GetElement(i))) {
                        // TODO, deduplicate by address
                        result.push_back(value.GetIncludedCategoryNames());
                    } else {
                        result.push_back(emptyValue);
                    }
                }
            }
        }
    }

    return result;
}


VtIntArray
HdSceneIndexAdapterSceneDelegate::GetInstanceIndices(
    SdfPath const &instancerId,
    SdfPath const &prototypeId)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    VtIntArray indices;

    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(instancerId);

    if (HdInstancerTopologySchema instancerTopology =
            HdInstancerTopologySchema::GetFromParent(prim.dataSource)) {
        indices = instancerTopology.ComputeInstanceIndicesForProto(prototypeId);
    }

    return indices;
}

SdfPathVector
HdSceneIndexAdapterSceneDelegate::GetInstancerPrototypes(
        SdfPath const &instancerId)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    SdfPathVector prototypes;

    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(instancerId);

    if (HdInstancerTopologySchema instancerTopology =
            HdInstancerTopologySchema::GetFromParent(prim.dataSource)) {
        HdPathArrayDataSourceHandle protoDs =
            instancerTopology.GetPrototypes();
        if (protoDs) {
            VtArray<SdfPath> protoArray = protoDs->GetTypedValue(0);
            prototypes.assign(protoArray.begin(), protoArray.end());
        }
    }

    return prototypes;
}

SdfPath
HdSceneIndexAdapterSceneDelegate::GetInstancerId(SdfPath const &id)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath instancerId;

    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(id);

    if (HdInstancedBySchema instancedBy =
            HdInstancedBySchema::GetFromParent(prim.dataSource)) {
        VtArray<SdfPath> instancerIds;
        if (HdPathArrayDataSourceHandle instancerIdsDs =
                instancedBy.GetPaths()) {
            instancerIds = instancerIdsDs->GetTypedValue(0);
        }

        // XXX: Right now the scene delegate can't handle multiple
        // instancers, so we rely on upstream ops to make the size <= 1.
        if (instancerIds.size() > 1) {
            TF_CODING_ERROR("Prim <%s> has multiple instancer ids, using first.",
                id.GetText());
        }

        if (instancerIds.size() > 0) {
            instancerId = instancerIds[0];
        }
    }

    return instancerId;
}

SdfPath
HdSceneIndexAdapterSceneDelegate::GetDataSharingId(SdfPath const& primId)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath sharingId;

    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(primId);

    if (HdDataSharingSchema dataSharing =
            HdDataSharingSchema::GetFromParent(prim.dataSource)) {
        if (HdPathDataSourceHandle sharingIdDs =
                dataSharing.GetSharingId()) {
            sharingId = sharingIdDs->GetTypedValue(0);
        }
    }

    return sharingId;
}

TfTokenVector
HdSceneIndexAdapterSceneDelegate::GetExtComputationSceneInputNames(
        SdfPath const &computationId)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(computationId);
    if (HdExtComputationSchema extComputation =
            HdExtComputationSchema::GetFromParent(prim.dataSource)) {
        if (HdContainerDataSourceHandle inputDs =
                extComputation.GetInputValues()) {
            return inputDs->GetNames();
        }
    }

    return TfTokenVector();
}

VtValue
HdSceneIndexAdapterSceneDelegate::GetExtComputationInput(
        SdfPath const &computationId, TfToken const &input)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(computationId);
    if (HdExtComputationSchema extComputation =
            HdExtComputationSchema::GetFromParent(prim.dataSource)) {

        if (input == HdTokens->dispatchCount) {
            if (HdSizetDataSourceHandle dispatchDs =
                    extComputation.GetDispatchCount()) {
                return dispatchDs->GetValue(0);
            }
        } else if (input == HdTokens->elementCount) {
            if (HdSizetDataSourceHandle elementDs =
                    extComputation.GetElementCount()) {
                return elementDs->GetValue(0);
            }
        } else {
            if (HdContainerDataSourceHandle inputDs =
                    extComputation.GetInputValues()) {
                if (HdSampledDataSourceHandle valueDs =
                        HdSampledDataSource::Cast(inputDs->Get(input))) {
                    return valueDs->GetValue(0);
                }
            }
        }
    }

    return VtValue();
}

size_t
HdSceneIndexAdapterSceneDelegate::SampleExtComputationInput(
        SdfPath const &computationId,
        TfToken const &input, size_t maxSampleCount,
        float *sampleTimes, VtValue *sampleValues)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(computationId);
    HdExtComputationSchema extComputation =
        HdExtComputationSchema::GetFromParent(prim.dataSource);
    if (!extComputation) {
        return 0;
    }
    HdContainerDataSourceHandle inputDs = extComputation.GetInputValues();
    if (!inputDs) {
        return 0;
    }
    HdSampledDataSourceHandle valueDs =
        HdSampledDataSource::Cast(inputDs->Get(input));
    if (!valueDs) {
        return 0;
    }

    std::vector<HdSampledDataSource::Time> times;
    // XXX: If the input prim is a legacy prim, the scene delegate is
    // responsible for setting the shutter window.  We can't query it, but
    // we pass the infinite window to accept all time samples from the
    // scene delegate.
    //
    // If the input prim is a datasource prim, we need some sensible default
    // here...  For now, we pass [0,0] to turn off multisampling.
    if (prim.dataSource->Has(HdSceneIndexEmulationTokens->sceneDelegate)) {
        valueDs->GetContributingSampleTimesForInterval(
                std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::max(), &times);
    } else {
        valueDs->GetContributingSampleTimesForInterval(
                0, 0, &times);
    }

    size_t authoredSamples = times.size();
    if (authoredSamples > maxSampleCount) {
        times.resize(maxSampleCount);
    }

    // XXX fallback to include a single sample
    if (times.empty()) {
        times.push_back(0.0f);
    }

    for (size_t i = 0; i < times.size(); ++i) {
        sampleTimes[i] = times[i];
        sampleValues[i] = valueDs->GetValue(times[i]);
    }

    return authoredSamples;
}

HdExtComputationInputDescriptorVector
HdSceneIndexAdapterSceneDelegate::GetExtComputationInputDescriptors(
        SdfPath const &computationId)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdExtComputationInputDescriptorVector result;

    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(computationId);
    if (HdExtComputationSchema extComputation =
            HdExtComputationSchema::GetFromParent(prim.dataSource)) {
        if (HdVectorDataSourceHandle vecDs =
                extComputation.GetInputComputations()) {
            size_t count = vecDs->GetNumElements();
            result.reserve(count);
            for (size_t i = 0; i < count; ++i) {
                HdExtComputationInputComputationSchema input(
                    HdContainerDataSource::Cast(vecDs->GetElement(i)));
                if (!input) {
                    continue;
                }

                HdExtComputationInputDescriptor desc;
                if (HdTokenDataSourceHandle nameDs = input.GetName()) {
                    desc.name = nameDs->GetTypedValue(0);
                }
                if (HdPathDataSourceHandle srcDs =
                        input.GetSourceComputation()) {
                    desc.sourceComputationId = srcDs->GetTypedValue(0);
                }
                if (HdTokenDataSourceHandle srcNameDs =
                        input.GetSourceComputationOutputName()) {
                    desc.sourceComputationOutputName =
                        srcNameDs->GetTypedValue(0);
                }
                result.push_back(desc);
            }
        }
    }

    return result;
}

HdExtComputationOutputDescriptorVector
HdSceneIndexAdapterSceneDelegate::GetExtComputationOutputDescriptors(
        SdfPath const &computationId)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdExtComputationOutputDescriptorVector result;

    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(computationId);
    if (HdExtComputationSchema extComputation =
            HdExtComputationSchema::GetFromParent(prim.dataSource)) {
        if (HdVectorDataSourceHandle vecDs = extComputation.GetOutputs()) {
            size_t count = vecDs->GetNumElements();
            result.reserve(count);
            for (size_t i = 0; i < count; ++i) {
                HdExtComputationOutputSchema output(
                    HdContainerDataSource::Cast(vecDs->GetElement(i)));
                if (!output) {
                    continue;
                }

                HdExtComputationOutputDescriptor desc;
                if (HdTokenDataSourceHandle nameDs = output.GetName()) {
                    desc.name = nameDs->GetTypedValue(0);
                }
                if (HdTupleTypeDataSourceHandle typeDs =
                        output.GetValueType()) {
                    desc.valueType = typeDs->GetTypedValue(0);
                }
                result.push_back(desc);
            }
        }
    }

    return result;
}

std::string
HdSceneIndexAdapterSceneDelegate::GetExtComputationKernel(
        SdfPath const &computationId)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(computationId);
    if (HdExtComputationSchema extComputation =
            HdExtComputationSchema::GetFromParent(prim.dataSource)) {
        HdStringDataSourceHandle ds = extComputation.GetGlslKernel();
        if (ds) {
            return ds->GetTypedValue(0);
        }
    }
    return std::string();
}

void
HdSceneIndexAdapterSceneDelegate::InvokeExtComputation(
        SdfPath const &computationId, HdExtComputationContext *context)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(computationId);
    if (HdExtComputationSchema extComputation =
            HdExtComputationSchema::GetFromParent(prim.dataSource)) {
        HdExtComputationCallbackDataSourceHandle ds =
            HdExtComputationCallbackDataSource::Cast(
                extComputation.GetCpuCallback());
        if (ds) {
            ds->Invoke(context);
        }
    }
}

void 
HdSceneIndexAdapterSceneDelegate::Sync(HdSyncRequestVector* request)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!request || request->IDs.size() == 0) {
        return;
    }

    if (!_sceneDelegatesBuilt) {
        tbb::concurrent_unordered_set<HdSceneDelegate*> sds;
        _primCache.ParallelForEach(
            [this, &sds](const SdfPath &k, const _PrimCacheEntry &v) {
                HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(k);
                if (!prim.dataSource) {
                    return;
                }

                HdTypedSampledDataSource<HdSceneDelegate*>::Handle ds = 
                    HdTypedSampledDataSource<HdSceneDelegate*>::Cast(
                        prim.dataSource->Get(
                            HdSceneIndexEmulationTokens->sceneDelegate));
                if (!ds) {
                    return;
                }

                sds.insert(ds->GetTypedValue(0));
            });
        _sceneDelegates.assign(sds.begin(), sds.end());
        _sceneDelegatesBuilt = true;
    }

    for (auto sd : _sceneDelegates) {
        if (TF_VERIFY(sd != nullptr)) {
            sd->Sync(request);
        } 
    }
}

void
HdSceneIndexAdapterSceneDelegate::PostSyncCleanup()
{
    if (!_sceneDelegatesBuilt) {
        return;
    }

    for (auto sd : _sceneDelegates) {
        if (TF_VERIFY(sd != nullptr)) {
            sd->PostSyncCleanup();
        }
    }
}

// ----------------------------------------------------------------------------

HdDisplayStyle
HdSceneIndexAdapterSceneDelegate::GetDisplayStyle(SdfPath const &id)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdDisplayStyle result;
    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(id);
    if (HdLegacyDisplayStyleSchema styleSchema =
            HdLegacyDisplayStyleSchema::GetFromParent(prim.dataSource)) {

        if (HdIntDataSourceHandle ds =
                styleSchema.GetRefineLevel()) {
            result.refineLevel = ds->GetTypedValue(0.0f);
        }

        if (HdBoolDataSourceHandle ds =
                styleSchema.GetFlatShadingEnabled()) {
            result.flatShadingEnabled = ds->GetTypedValue(0.0f);
        }

        if (HdBoolDataSourceHandle ds =
                styleSchema.GetDisplacementEnabled()) {
            result.displacementEnabled = ds->GetTypedValue(0.0f);
        }

        if (HdBoolDataSourceHandle ds =
                styleSchema.GetOccludedSelectionShowsThrough()) {
            result.occludedSelectionShowsThrough = ds->GetTypedValue(0.0f);
        }

        if (HdBoolDataSourceHandle ds =
                styleSchema.GetPointsShadingEnabled()) {
            result.pointsShadingEnabled = ds->GetTypedValue(0.0f);
        }

        if (HdBoolDataSourceHandle ds =
                styleSchema.GetMaterialIsFinal()) {
            result.materialIsFinal = ds->GetTypedValue(0.0f);
        }
    }

    return result;
}

VtValue
HdSceneIndexAdapterSceneDelegate::GetShadingStyle(SdfPath const &id)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    VtValue result;
    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(id);
    if (HdLegacyDisplayStyleSchema styleSchema =
            HdLegacyDisplayStyleSchema::GetFromParent(prim.dataSource)) {

        if (HdTokenDataSourceHandle ds =
                styleSchema.GetShadingStyle()) {
            TfToken st = ds->GetTypedValue(0.0f);
            result = VtValue(st);
        }
    }

    return result;
}

HdReprSelector
HdSceneIndexAdapterSceneDelegate::GetReprSelector(SdfPath const &id)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdReprSelector result;
    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(id);
    if (HdLegacyDisplayStyleSchema styleSchema =
            HdLegacyDisplayStyleSchema::GetFromParent(prim.dataSource)) {

        if (HdTokenArrayDataSourceHandle ds =
                styleSchema.GetReprSelector()) {
            VtArray<TfToken> ar = ds->GetTypedValue(0.0f);
            ar.resize(HdReprSelector::MAX_TOPOLOGY_REPRS);
            result = HdReprSelector(ar[0], ar[1], ar[2]);
        }
    }

    return result;
}

HdCullStyle
HdSceneIndexAdapterSceneDelegate::GetCullStyle(SdfPath const &id)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdCullStyle result = HdCullStyleDontCare;
    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(id);
    if (HdLegacyDisplayStyleSchema styleSchema =
            HdLegacyDisplayStyleSchema::GetFromParent(prim.dataSource)) {

        if (HdTokenDataSourceHandle ds =
                styleSchema.GetCullStyle()) {
            TfToken ct = ds->GetTypedValue(0.0f);
            if (ct == HdCullStyleTokens->nothing) {
                result = HdCullStyleNothing;
            } else if (ct == HdCullStyleTokens->back) {
                result = HdCullStyleBack;
            } else if (ct == HdCullStyleTokens->front) {
                result = HdCullStyleFront;
            } else if (ct == HdCullStyleTokens->backUnlessDoubleSided) {
                result = HdCullStyleBackUnlessDoubleSided;
            } else if (ct == HdCullStyleTokens->frontUnlessDoubleSided) {
                result = HdCullStyleFrontUnlessDoubleSided;
            } else {
                result = HdCullStyleDontCare;
            }
        }
    }

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
