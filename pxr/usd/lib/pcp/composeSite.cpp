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
#include "pxr/usd/pcp/site.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/sdf/primSpec.h"

PXR_NAMESPACE_OPEN_SCOPE

// Implementation notes:
//
// These go directly to SdfLayer's field API, skipping
// indirection through SdfSpecHandle identities.
//
// For arcs that refer to asset paths, these functions additionally
// compute the absolute form of the asset path, using the layer that
// expressed the opinion as the basis for relative paths.


// List-op composition callback that computes absolute asset paths
// relative to the layer where they were expressed.
static boost::optional<SdfReference>
_ResolveReference( const SdfLayerHandle& layer, 
                   const SdfLayerOffset& layerOffset,
                   std::map<SdfReference, PcpSourceReferenceInfo>* infoMap,
                   SdfListOpType opType,
                   const SdfReference& ref)
{
    SdfReference result( ref.GetAssetPath(),
                        ref.GetPrimPath(), 
                        layerOffset * ref.GetLayerOffset(),
                        ref.GetCustomData() );
    PcpSourceReferenceInfo& info = (*infoMap)[result];
    info.layer       = layer;
    info.layerOffset = ref.GetLayerOffset();
    return result;
}

void
PcpComposeSiteReferences(PcpLayerStackRefPtr const &layerStack,
                         SdfPath const &path,
                         SdfReferenceVector *result,
                         PcpSourceReferenceInfoVector *info )
{
    static const TfToken field = SdfFieldKeys->References;

    // Sd provides no convenient way to annotate each element of the result.
    // So we use a map from element value to its annotation, which in this
    // case is a PcpSourceReferenceInfo.
    std::map<SdfReference, PcpSourceReferenceInfo> infoMap;

    const SdfLayerRefPtrVector& layers = layerStack->GetLayers();
    SdfReferenceListOp curListOp;

    result->clear();
    for (size_t i = layers.size(); i-- != 0; ) {
        const SdfLayerHandle& layer = layers[i];
        if (layer->HasField(path, field, &curListOp)) {
            const SdfLayerOffset* layerOffset =
                layerStack->GetLayerOffsetForLayer(i);
            curListOp.ApplyOperations(result,
                boost::bind( &_ResolveReference, boost::ref(layer),
                             layerOffset ? *layerOffset : SdfLayerOffset(),
                             &infoMap, _1, _2));
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
PcpComposeSitePayload(PcpLayerStackRefPtr const &layerStack,
                      SdfPath const &path,
                      SdfPayload *result,
                      SdfLayerHandle *sourceLayer)
{
    static const TfToken field = SdfFieldKeys->Payload;

    for (auto const &layer: layerStack->GetLayers()) {
        if (layer->HasField(path, field, result) && *result) {
            *sourceLayer = layer;
            return;
        }
    }
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

PXR_NAMESPACE_CLOSE_SCOPE
