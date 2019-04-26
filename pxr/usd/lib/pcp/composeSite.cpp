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
#include "pxr/usd/pcp/composeSite.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/layerUtils.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/sdf/primSpec.h"

#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

// Implementation notes:
//
// These go directly to SdfLayer's field API, skipping
// indirection through SdfSpecHandle identities.
//
// For arcs that refer to asset paths, these functions additionally
// compute the absolute form of the asset path, using the layer that
// expressed the opinion as the basis for relative paths.

// SdfReference has custom data that is copied during composition, SdfPayload 
// does not.
static void 
_CopyCustomData(SdfReference *lhs, const SdfReference &rhs)
{
    lhs->SetCustomData(rhs.GetCustomData());
}

static void 
_CopyCustomData(SdfPayload *, const SdfPayload&)
{
    // do nothing
}

// Payload and reference lists are composed in the same way.
template <class RefOrPayloadType>
static void
_PcpComposeSiteReferencesOrPayloads(TfToken const &field,
                                    PcpLayerStackRefPtr const &layerStack,
                                    SdfPath const &path,
                                    std::vector<RefOrPayloadType> *result,
                                    PcpSourceReferenceInfoVector *info )
{
    // Sdf provides no convenient way to annotate each element of the result.
    // So we use a map from element value to its annotation, which in this
    // case is a PcpSourceReferenceInfo.
    std::map<RefOrPayloadType, PcpSourceReferenceInfo> infoMap;

    const SdfLayerRefPtrVector& layers = layerStack->GetLayers();
    SdfListOp<RefOrPayloadType> curListOp;

    result->clear();
    for (size_t i = layers.size(); i-- != 0; ) {
        const SdfLayerHandle& layer = layers[i];
        if (layer->HasField(path, field, &curListOp)) {
            const SdfLayerOffset* layerOffset =
                layerStack->GetLayerOffsetForLayer(i);

            // List-op composition callback computes absolute asset paths
            // relative to the layer where they were expressed and combines
            // layer offsets.
            curListOp.ApplyOperations(result,
                [&layer, layerOffset, &infoMap](
                    SdfListOpType opType, const RefOrPayloadType& refOrPayload)
                {
                    // Fill in the result reference of payload with the anchored
                    // asset path instead of the authored asset path. This 
                    // ensures that references or payloads with the same 
                    // relative asset path but anchored to different
                    // locations will not be considered duplicates.
                    const std::string &authoredAssetPath = 
                        refOrPayload.GetAssetPath();
                    const std::string assetPath = authoredAssetPath.empty() ? 
                        authoredAssetPath : 
                        SdfComputeAssetPathRelativeToLayer(
                            layer, authoredAssetPath);
                    SdfLayerOffset resolvedLayerOffset = layerOffset ?
                        *layerOffset * refOrPayload.GetLayerOffset() : 
                        refOrPayload.GetLayerOffset();
                    RefOrPayloadType result( assetPath, 
                                             refOrPayload.GetPrimPath(),
                                             resolvedLayerOffset);

                    _CopyCustomData(&result, refOrPayload);
                    PcpSourceReferenceInfo& info = infoMap[result];
                    info.layer = layer;
                    info.layerOffset = refOrPayload.GetLayerOffset();
                    info.authoredAssetPath = refOrPayload.GetAssetPath();
                    return result;
                });
        }
    }

    // Fill in info.
    info->clear();
    info->reserve(result->size());
    for (auto const &ref: *result) {
        info->push_back(infoMap[ref]);
    }
}

void
PcpComposeSiteReferences(PcpLayerStackRefPtr const &layerStack,
                         SdfPath const &path,
                         SdfReferenceVector *result,
                         PcpSourceReferenceInfoVector *info )
{
    _PcpComposeSiteReferencesOrPayloads(
        SdfFieldKeys->References, layerStack, path, result, info);
}

void
PcpComposeSitePayloads(PcpLayerStackRefPtr const &layerStack,
                       SdfPath const &path,
                       SdfPayloadVector *result,
                       PcpSourceReferenceInfoVector *info )
{
    _PcpComposeSiteReferencesOrPayloads(
        SdfFieldKeys->Payload, layerStack, path, result, info);
}

SdfPermission
PcpComposeSitePermission(PcpLayerStackRefPtr const &layerStack,
                         SdfPath const &path)
{
    SdfPermission perm = SdfPermissionPublic;
    for (auto const &layer: layerStack->GetLayers()) {
        if (layer->HasField(path, SdfFieldKeys->Permission, &perm))
            break;
    }
    return perm;
}

bool
PcpComposeSiteHasPrimSpecs(PcpLayerStackRefPtr const &layerStack,
                           SdfPath const &path)
{
    for (auto const &layer: layerStack->GetLayers()) {
        if (layer->HasSpec(path)) {
            return true;
        }
    }
    return false;
}

bool
PcpComposeSiteHasSymmetry(PcpLayerStackRefPtr const &layerStack,
                          SdfPath const &path)
{
    for (auto const &layer: layerStack->GetLayers()) {
        if (layer->HasField(path, SdfFieldKeys->SymmetryFunction) ||
            layer->HasField(path, SdfFieldKeys->SymmetryArguments)) {
            return true;
        }
    }
    return false;
}

void
PcpComposeSitePrimSites(PcpLayerStackRefPtr const &layerStack,
                        SdfPath const &path,
                        SdfSiteVector *result)
{
    for (auto const &layer: layerStack->GetLayers()) {
        if (layer->HasSpec(path))
            result->push_back(SdfSite(layer, path));
    }
}

void
PcpComposeSiteRelocates(PcpLayerStackRefPtr const &layerStack,
                        SdfPath const &path,
                        SdfRelocatesMap *result)
{
    static const TfToken field = SdfFieldKeys->Relocates;

    SdfRelocatesMap relocMap;
    TF_REVERSE_FOR_ALL(layer, layerStack->GetLayers()) {
        if ((*layer)->HasField(path, field, &relocMap)) {
            TF_FOR_ALL(reloc, relocMap) {
                SdfPath source = reloc->first .MakeAbsolutePath(path);
                SdfPath target = reloc->second.MakeAbsolutePath(path);
                (*result)[source] = target;
            }
        }
    }
}

void
PcpComposeSiteInherits( const PcpLayerStackRefPtr &layerStack,
                        const SdfPath &path, SdfPathVector *result )
{
    static const TfToken field = SdfFieldKeys->InheritPaths;

    SdfPathListOp inheritListOp;
    TF_REVERSE_FOR_ALL(layer, layerStack->GetLayers()) {
        if ((*layer)->HasField(path, field, &inheritListOp)) {
            inheritListOp.ApplyOperations(result);
        }
    }
}

void
PcpComposeSiteSpecializes(PcpLayerStackRefPtr const &layerStack,
                          SdfPath const &path, SdfPathVector *result)
{
    static const TfToken field = SdfFieldKeys->Specializes;

    SdfPathListOp specializesListOp;
    TF_REVERSE_FOR_ALL(layer, layerStack->GetLayers()) {
        if ((*layer)->HasField(path, field, &specializesListOp)) {
            specializesListOp.ApplyOperations(result);
        }
    }
}

void
PcpComposeSiteVariantSets(PcpLayerStackRefPtr const &layerStack,
                          SdfPath const &path,
                          std::vector<std::string> *result)
{
    static const TfToken field = SdfFieldKeys->VariantSetNames;

    SdfStringListOp vsetListOp;
    TF_REVERSE_FOR_ALL(layer, layerStack->GetLayers()) {
        if ((*layer)->HasField(path, field, &vsetListOp)) {
            vsetListOp.ApplyOperations(result);
        }
    }
}

void
PcpComposeSiteVariantSetOptions(PcpLayerStackRefPtr const &layerStack,
                                SdfPath const &path,
                                std::string const &vsetName,
                                std::set<std::string> *result)
{
    static const TfToken field = SdfChildrenKeys->VariantChildren;

    const SdfPath vsetPath = path.AppendVariantSelection(vsetName, "");
    TfTokenVector vsetNames;
    for (auto const &layer: layerStack->GetLayers()) {
        if (layer->HasField(vsetPath, field, &vsetNames)) {
            TF_FOR_ALL(name, vsetNames) {
                result->insert(name->GetString());
            }
        }
    }
}

bool
PcpComposeSiteVariantSelection(PcpLayerStackRefPtr const &layerStack,
                               SdfPath const &path,
                               std::string const &vsetName,
                               std::string *result)
{
    static const TfToken field = SdfFieldKeys->VariantSelection;

    SdfVariantSelectionMap vselMap;
    for (auto const &layer: layerStack->GetLayers()) {
        if (layer->HasField(path, field, &vselMap)) {
            SdfVariantSelectionMap::const_iterator i = vselMap.find(vsetName);
            if (i != vselMap.end()) {
                *result = i->second;
                return true;
            } 
        }
    }
    return false;
}

void 
PcpComposeSiteVariantSelections(PcpLayerStackRefPtr const &layerStack,
                                SdfPath const &path,
                                SdfVariantSelectionMap *result)
{
    static const TfToken field = SdfFieldKeys->VariantSelection;

    SdfVariantSelectionMap vselMap;
    for (auto const &layer: layerStack->GetLayers()) {
        if (layer->HasField(path, field, &vselMap)) {
            result->insert(vselMap.begin(), vselMap.end());
        }
    }
}

void
PcpComposeSiteChildNames(SdfLayerRefPtrVector const &layers,
                         SdfPath const &path,
                         const TfToken & namesField,
                         TfTokenVector *nameOrder,
                         PcpTokenSet *nameSet,
                         const TfToken *orderField)
{
    TF_REVERSE_FOR_ALL(layer, layers) {
        VtValue namesVal = (*layer)->GetField(path, namesField);
        if (namesVal.IsHolding<TfTokenVector>()) {
            const TfTokenVector & names =
                namesVal.UncheckedGet<TfTokenVector>();
            // Append names in order.  Skip names that are 
            // already in the nameSet.
            TF_FOR_ALL(name, names) {
                if (nameSet->insert(*name).second) {
                    nameOrder->push_back(*name);
                }
            }
        }
        if (orderField) {
            VtValue orderVal = (*layer)->GetField(path, *orderField);
            if (orderVal.IsHolding<TfTokenVector>()) {
                SdfApplyListOrdering(nameOrder,
                                     orderVal.UncheckedGet<TfTokenVector>());
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
