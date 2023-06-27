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
#include "pxr/usd/pcp/expressionVariables.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/utils.h"
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

static const char*
_GetErrorContext(const SdfReference*)
{
    return "reference";
}

static const char*
_GetErrorContext(const SdfPayload*)
{
    return "payload";
}

// Payload and reference lists are composed in the same way.
template <class RefOrPayloadType>
static void
_PcpComposeSiteReferencesOrPayloads(
    TfToken const &field,
    PcpLayerStackRefPtr const &layerStack,
    SdfPath const &path,
    std::vector<RefOrPayloadType> *result,
    PcpSourceArcInfoVector *info,
    std::unordered_set<std::string> *exprVarDependencies,
    PcpErrorVector *errors)
{
    // Sdf provides no convenient way to annotate each element of the result.
    // So we use a map from element value to its annotation, which in this
    // case is a PcpSourceArcInfo.
    std::map<RefOrPayloadType, PcpSourceArcInfo> infoMap;

    const SdfLayerRefPtrVector& layers = layerStack->GetLayers();
    SdfListOp<RefOrPayloadType> curListOp;

    result->clear();
    for (size_t i = layers.size(); i-- != 0; ) {
        const SdfLayerHandle& layer = layers[i];
        if (!layer->HasField(path, field, &curListOp)) {
            continue;
        }

        const SdfLayerOffset* layerOffset =
            layerStack->GetLayerOffsetForLayer(i);

        // List-op composition callback computes absolute asset paths
        // relative to the layer where they were expressed.
        curListOp.ApplyOperations(result,
            [&](SdfListOpType opType, const RefOrPayloadType& refOrPayload)
            -> boost::optional<RefOrPayloadType>
            {
                // Fill in the result reference or payload with the anchored
                // asset path instead of the authored asset path. This 
                // ensures that references or payloads with the same 
                // relative asset path but anchored to different
                // locations will not be considered duplicates.
                std::string authoredAssetPath = refOrPayload.GetAssetPath();

                std::string assetPath;
                if (Pcp_IsVariableExpression(authoredAssetPath)) {
                    authoredAssetPath = Pcp_EvaluateVariableExpression(
                        authoredAssetPath, layerStack->GetExpressionVariables(),
                        _GetErrorContext((const RefOrPayloadType*)(nullptr)),
                        layer, path, exprVarDependencies, errors);

                    // Expressions that evaluate to an empty path are silently
                    // ignored to allow users to conditionally reference a
                    // layer. If the empty result was due to an error, that
                    // will have already been saved to the errors list above.
                    if (authoredAssetPath.empty()) {
                        return boost::none;
                    }

                    assetPath = SdfComputeAssetPathRelativeToLayer(
                        layer, authoredAssetPath);
                }
                else if (!authoredAssetPath.empty()) {
                    assetPath = SdfComputeAssetPathRelativeToLayer(
                        layer, authoredAssetPath);
                }

                RefOrPayloadType result( assetPath, 
                                         refOrPayload.GetPrimPath(),
                                         refOrPayload.GetLayerOffset());

                _CopyCustomData(&result, refOrPayload);
                infoMap[result] = {
                    layer,
                    layerOffset ? *layerOffset : SdfLayerOffset(),
                    std::move(authoredAssetPath)
                };
                return result;
            });
    }

    // Fill in info.
    info->clear();
    info->reserve(result->size());
    for (auto const &ref: *result) {
        info->push_back(infoMap[ref]);
    }
}

void
PcpComposeSiteReferences(
    PcpLayerStackRefPtr const &layerStack,
    SdfPath const &path,
    SdfReferenceVector *result,
    PcpSourceArcInfoVector *info,
    std::unordered_set<std::string> *stageVarDependencies,
    PcpErrorVector *errors)
{
    _PcpComposeSiteReferencesOrPayloads(
        SdfFieldKeys->References, layerStack, path, result, info,
        stageVarDependencies, errors);
}

void
PcpComposeSitePayloads(
    PcpLayerStackRefPtr const &layerStack,
    SdfPath const &path,
    SdfPayloadVector *result,
    PcpSourceArcInfoVector *info,
    std::unordered_set<std::string> *stageVarDependencies,
    PcpErrorVector *errors)
{
    _PcpComposeSiteReferencesOrPayloads(
        SdfFieldKeys->Payload, layerStack, path, result, info,
        stageVarDependencies, errors);
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

// Helper for PcpComposeSiteInherits/Specializes/ overloads
// that want to provide source arc info with the layer that adds each result.
template <typename ResultType>
static void
_ComposeSiteListOpWithSourceInfo(const PcpLayerStackRefPtr &layerStack,
                                 const SdfPath &path,
                                 const TfToken &field,
                                 std::vector<ResultType> *result,
                                 PcpSourceArcInfoVector *info)
{
    // Map of result value to source arc info. The same value may appear in 
    // multiple layers' list ops. This lets us make sure we find the strongest
    // layer that added the value.
    std::map<ResultType, PcpSourceArcInfo> infoMap;

    SdfListOp<ResultType> listOp;
    TF_REVERSE_FOR_ALL(layer, layerStack->GetLayers()) {
        if ((*layer)->HasField(path, field, &listOp)) {
            listOp.ApplyOperations(result,
                [&layer, &infoMap](SdfListOpType opType, const ResultType &path)
                {
                    // Just store the layer in the source arc info for the 
                    // result. We don't need the other data.
                    infoMap[path].layer = *layer;
                    return path;
                });
        }
    }

    // Construct the parallel array of source info to results.
    info->reserve(result->size());
    for (const ResultType &path: *result) {
        info->push_back(infoMap[path]);
    }
}

void
PcpComposeSiteInherits( const PcpLayerStackRefPtr &layerStack,
                        const SdfPath &path, SdfPathVector *result,
                        PcpSourceArcInfoVector *info )
{
    static const TfToken field = SdfFieldKeys->InheritPaths;
    _ComposeSiteListOpWithSourceInfo(layerStack, path, field, result, info);
}

void
PcpComposeSiteInherits( const PcpLayerStackRefPtr &layerStack,
                        const SdfPath &path, SdfPathVector *result)
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
PcpComposeSiteSpecializes(const PcpLayerStackRefPtr &layerStack,
                          const SdfPath &path, SdfPathVector *result,
                          PcpSourceArcInfoVector *info )
{
    static const TfToken field = SdfFieldKeys->Specializes;
    _ComposeSiteListOpWithSourceInfo(layerStack, path, field, result, info);
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

PCP_API
void
PcpComposeSiteVariantSets(PcpLayerStackRefPtr const &layerStack,
                          SdfPath const &path,
                          std::vector<std::string> *result,
                          PcpSourceArcInfoVector *info)
{
    static const TfToken field = SdfFieldKeys->VariantSetNames;
    _ComposeSiteListOpWithSourceInfo(layerStack, path, field, result, info);
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
PcpComposeSiteVariantSelection(
    PcpLayerStackRefPtr const &layerStack,
    SdfPath const &path,
    std::string const &vsetName,
    std::string *result,
    std::unordered_set<std::string> *exprVarDependencies,
    PcpErrorVector *errors)
{
    static const TfToken field = SdfFieldKeys->VariantSelection;

    SdfVariantSelectionMap vselMap;

    for (auto const &layer: layerStack->GetLayers()) {
        if (!layer->HasField(path, field, &vselMap)) {
            continue;
        }

        std::string* vsel = TfMapLookupPtr(vselMap, vsetName);
        if (!vsel) {
            continue;
        }

        if (Pcp_IsVariableExpression(*vsel)) {
            PcpErrorVector exprErrors;
            *vsel = Pcp_EvaluateVariableExpression(
                *vsel, layerStack->GetExpressionVariables(),
                "variant", layer, path, exprVarDependencies, &exprErrors);

            // If an error occurred evaluating this expression, we ignore
            // this variant selection and look for the next weakest opinion.
            if (!exprErrors.empty()) {
                if (errors) {
                    errors->insert(errors->end(),
                        std::make_move_iterator(exprErrors.begin()),
                        std::make_move_iterator(exprErrors.end()));
                }
                continue;
            }
        }

        *result = std::move(*vsel);
        return true;
    }
    return false;
}

void 
PcpComposeSiteVariantSelections(
    PcpLayerStackRefPtr const &layerStack,
    SdfPath const &path,
    SdfVariantSelectionMap *result,
    std::unordered_set<std::string> *exprVarDependencies,
    PcpErrorVector *errors)
{
    static const TfToken field = SdfFieldKeys->VariantSelection;

    SdfVariantSelectionMap vselMap;

    for (auto const &layer: layerStack->GetLayers()) {
        if (!layer->HasField(path, field, &vselMap)) {
            continue;
        }

        for (auto it = vselMap.begin(); it != vselMap.end(); ) {
            std::string& vsel = it->second;
            
            if (Pcp_IsVariableExpression(vsel)) {
                PcpErrorVector exprErrors;
                vsel = Pcp_EvaluateVariableExpression(
                    vsel, layerStack->GetExpressionVariables(),
                    "variant", layer, path, exprVarDependencies, &exprErrors);

                // If an error occurred evaluating this expression, we ignore
                // this variant selection and look for the next weakest opinion.
                if (!exprErrors.empty()) {
                    if (errors) {
                        errors->insert(errors->end(),
                            std::make_move_iterator(exprErrors.begin()),
                            std::make_move_iterator(exprErrors.end()));
                    }
                    it = vselMap.erase(it);
                    continue;
                }
            }

            ++it;
        }

        result->insert(vselMap.begin(), vselMap.end());
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
            TfTokenVector names = namesVal.UncheckedRemove<TfTokenVector>();

            // Append names in order.  Skip names that are already in the
            // nameSet.

            auto doOneByOne = [&]() {
                for (TfToken &name: names) {
                    if (nameSet->insert(name).second) {
                        nameOrder->push_back(std::move(name));
                    }
                }
            };
            
            // Commonly, nameSet is empty.  In this case, insert everything
            // upfront, then check the size.  If it is the same size as names,
            // they were unique, and we can just append them all.
            if (nameSet->empty()) {
                nameSet->insert(names.begin(), names.end());
                if (nameSet->size() == names.size()) {
                    *nameOrder = std::move(names);
                }
                else {
                    // This case is really, really unlikely -- sdfdata semantics
                    // should disallow duplicates within a single names field.
                    // In this case we just pay the price and do them
                    // one-by-one.
                    nameSet->clear();
                    doOneByOne();
                }
            }
            else {
                doOneByOne();
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
