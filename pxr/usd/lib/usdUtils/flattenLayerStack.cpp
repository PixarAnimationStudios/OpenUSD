//
// Copyright 2017 Pixar
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
#include "pxr/usd/usdUtils/flattenLayerStack.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/layerUtils.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/variantSetSpec.h"
#include "pxr/usd/sdf/variantSpec.h"
#include "pxr/usd/sdf/pseudoRootSpec.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/pcp/composeSite.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/clipsAPI.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/tokens.h"
#include "pxr/usd/ar/resolverContextBinder.h"
#include "pxr/base/tf/staticData.h"

#include <algorithm>
#include <functional>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////
// UsdUtilsFlattenLayerStack
//
// The approach to merge layer stacks into a single layer is as follows:
//
// - _FlattenSpecs() recurses down the typed hierarchy of specs,
//   using PcpComposeSiteChildNames() to discover child names
//   of each type of spec, and creating them in the output layer.
//
// - At each output site, _FlattenFields() flattens field data
//   using a _Reduce() helper to apply composition rules for
//   particular value types and fields.  It uses _ApplyLayerOffset()
//   to handle time-remapping needed, depending on the field.

template <typename T>
VtValue
_Reduce(const T &lhs, const T &rhs)
{
    // Generic base case: take stronger opinion.
    return VtValue(lhs);
}

// "Fix" a list op to only use composable features.
template <typename T>
SdfListOp<T>
_FixListOp(SdfListOp<T> op)
{
    std::vector<T> items;
    items = op.GetAppendedItems();
    for (const T& item: op.GetAddedItems()) {
        if (std::find(items.begin(), items.end(), item) == items.end()) {
            items.push_back(item);
        }
    }
    op.SetAppendedItems(items);
    op.SetAddedItems(std::vector<T>());
    op.SetOrderedItems(std::vector<T>());
    return op;
}

template <typename T>
VtValue
_Reduce(const SdfListOp<T> &lhs, const SdfListOp<T> &rhs)
{
    boost::optional<SdfListOp<T>> r = lhs.ApplyOperations(rhs);
    if (r) {
        return VtValue(*r);
    }
    // List ops that use added or reordered items cannot, in general, be
    // composed into another listop. In those cases, we fall back to a
    // best-effort approximation by discarding reorders and converting
    // adds to appends.
    r = _FixListOp(lhs).ApplyOperations(_FixListOp(rhs));
    if (r) {
        return VtValue(*r);
    }
    // The approximation used should always be composable,
    // so error if that didn't work.
    TF_CODING_ERROR("Could not reduce listOp %s over %s",
                    TfStringify(lhs).c_str(), TfStringify(rhs).c_str());
    return VtValue();
}

template <>
VtValue
_Reduce(const VtDictionary &lhs, const VtDictionary &rhs)
{
    // Dictionaries compose keys recursively.
    return VtValue(VtDictionaryOverRecursive(lhs, rhs));
}

template <>
VtValue
_Reduce(const SdfSpecifier &lhs, const SdfSpecifier &rhs)
{
    // SdfSpecifierOver is the equivalent of "no opinion"
    //
    // Note that, in general, specifiers do not simply compose as
    // "strongest wins" -- see UsdStage::_GetPrimSpecifierImpl() for
    // details.  However, in the case of composing strictly within a
    // layer stack, they can be considered as strongest wins.
    return VtValue(lhs == SdfSpecifierOver ? rhs : lhs);
}

// This function is an overload that also accepts a field name.
VtValue
_Reduce(const VtValue &lhs, const VtValue &rhs, const TfToken &field)
{
    // Handle easy generic cases first.
    if (lhs.IsEmpty()) {
        return rhs;
    }
    if (rhs.IsEmpty()) {
        return lhs;
    }
    if (lhs.GetType() != rhs.GetType()) {
        // As long as the caller observes the SdfLayer schema, this
        // should never happen.
        TF_CODING_ERROR("UsdUtilsUsdUtilsMergeLayerStack: Cannot reduce "
                        "type '%s' with type '%s'",
                        lhs.GetType().GetTypeName().c_str(),
                        rhs.GetType().GetTypeName().c_str());
        return VtValue();
    }

    // Dispatch to type-specific reduce / compose rules.
    //
    // XXX WBN to have more generic (i.e. automatically extended)
    // way to handle listop types in case we add more in the future.
    // Maybe sdf/types.h could provide a type-list we could iterate over?
#define TYPE_DISPATCH(T) \
    if (lhs.IsHolding<T>()) { \
        return _Reduce(lhs.UncheckedGet<T>(),  \
                       rhs.UncheckedGet<T>()); \
    }
    TYPE_DISPATCH(SdfSpecifier);
    TYPE_DISPATCH(SdfIntListOp);
    TYPE_DISPATCH(SdfUIntListOp);
    TYPE_DISPATCH(SdfInt64ListOp);
    TYPE_DISPATCH(SdfUInt64ListOp);
    TYPE_DISPATCH(SdfTokenListOp);
    TYPE_DISPATCH(SdfStringListOp);
    TYPE_DISPATCH(SdfPathListOp);
    TYPE_DISPATCH(SdfReferenceListOp);
    TYPE_DISPATCH(SdfUnregisteredValueListOp);
    TYPE_DISPATCH(VtDictionary);
    TYPE_DISPATCH(SdfTimeSampleMap);
#undef TYPE_DISPATCH

    // TypeName is a special case: empty token represents "no opinion".
    // (That is not true of token-valued fields in general.)
    if (field == SdfFieldKeys->TypeName && lhs.IsHolding<TfToken>()) {
        return lhs.UncheckedGet<TfToken>().IsEmpty() ? rhs : lhs;
    }

    // Generic base case: take stronger opinion.
    return lhs;
}

static void
_ApplyLayerOffsetToClipInfo(
    const SdfLayerOffset &offset,
    const TfToken& infoKey, VtDictionary* clipInfo)
{
    VtValue* v = TfMapLookupPtr(*clipInfo, infoKey);
    if (v && v->IsHolding<VtVec2dArray>()) {
        VtVec2dArray array;
        v->Swap(array);
        for (auto& entry : array) {
            entry[0] = offset * entry[0];
        }
        v->Swap(array);
    }
}

static boost::optional<SdfReference>
_ApplyLayerOffsetToReference(const SdfLayerOffset &offset,
                             const SdfReference &ref)
{
    SdfReference result = ref;
    result.SetLayerOffset(offset * ref.GetLayerOffset());
    return boost::optional<SdfReference>(result);
}

// Apply layer offsets (time remapping) to time-keyed metadata.
static VtValue
_ApplyLayerOffset(const SdfLayerOffset &offset,
                  const TfToken &field, VtValue val)
{
    SdfLayerOffset offsetToApply = UsdPrepLayerOffset(offset);
    if (field == SdfFieldKeys->TimeSamples) {
        if (val.IsHolding<SdfTimeSampleMap>()) {
            SdfTimeSampleMap entries = val.UncheckedGet<SdfTimeSampleMap>();
            SdfTimeSampleMap mappedEntries;
            for (const auto &entry: entries) {
                mappedEntries[offsetToApply * entry.first] = entry.second;
            }
            return VtValue(mappedEntries);
        }
    }
    else if (field == UsdTokens->clipActive ||
             field == UsdTokens->clipTimes) {
        if (val.IsHolding<VtVec2dArray>()) {
            VtVec2dArray entries = val.UncheckedGet<VtVec2dArray>();
            for (auto &entry: entries) {
                entry[0] = offsetToApply * entry[0];
            }
            return VtValue(entries);
        }
    }
    else if (field == UsdTokens->clipTemplateStartTime ||
             field == UsdTokens->clipTemplateEndTime) {
        if (val.IsHolding<double>()) {
            return VtValue(offsetToApply * val.UncheckedGet<double>());
        }
    }
    else if (field == UsdTokens->clips) {
        if (val.IsHolding<VtDictionary>()) {
            VtDictionary clips = val.UncheckedGet<VtDictionary>();
            for (auto &entry: clips) {
                const std::string& clipSetName = entry.first;
                VtValue& clipInfoVal = entry.second;
                if (!clipInfoVal.IsHolding<VtDictionary>()) {
                    TF_WARN("Expected dictionary for entry '%s' in 'clips'",
                            clipSetName.c_str());
                    continue;
                }
                VtDictionary clipInfo =
                    clipInfoVal.UncheckedGet<VtDictionary>();
                _ApplyLayerOffsetToClipInfo(
                    offsetToApply, UsdClipsAPIInfoKeys->active, &clipInfo);
                _ApplyLayerOffsetToClipInfo(
                    offsetToApply, UsdClipsAPIInfoKeys->times, &clipInfo);
                clipInfoVal = VtValue(clipInfo);
            }
            return VtValue(clips);
        }
    }
    else if (field == SdfFieldKeys->References) {
        if (val.IsHolding<SdfReferenceListOp>()) {
            SdfReferenceListOp refs = val.UncheckedGet<SdfReferenceListOp>();
            // We do not need to call UsdPrepLayerOffset() here since
            // we want to author a new offset, not apply one.
            refs.ModifyOperations(std::bind(
                _ApplyLayerOffsetToReference, offset, std::placeholders::_1));
            return VtValue(refs);
        }
    }
    return val;
}

static boost::optional<SdfReference>
_FixReference(const SdfLayerHandle &sourceLayer,
              const SdfReference &ref)
{
    SdfReference result = ref;
    result.SetAssetPath(SdfComputeAssetPathRelativeToLayer(sourceLayer,
                                                           ref.GetAssetPath()));
    return boost::optional<SdfReference>(result);
}

static void
_FixAssetPaths(const SdfLayerHandle &sourceLayer,
               const TfToken &field,
               VtValue *val)
{
    if (val->IsHolding<SdfAssetPath>()) {
        SdfAssetPath ap;
        val->Swap(ap);
        ap = SdfAssetPath(
            SdfComputeAssetPathRelativeToLayer(sourceLayer,
                                               ap.GetAssetPath()));
        val->Swap(ap);
        return;
    }
    else if (val->IsHolding<VtArray<SdfAssetPath>>()) {
        VtArray<SdfAssetPath> a;
        val->Swap(a);
        for (SdfAssetPath &ap: a) {
            ap = SdfAssetPath(
                SdfComputeAssetPathRelativeToLayer(sourceLayer,
                                                   ap.GetAssetPath()));
        }
        val->Swap(a);
        return;
    }
    else if (val->IsHolding<SdfReference>()) {
        SdfReference ref;
        val->Swap(ref);
        ref = *_FixReference(sourceLayer, ref);
        val->Swap(ref);
        return;
    }
    else if (val->IsHolding<SdfReferenceListOp>()) {
        SdfReferenceListOp refs;
        val->Swap(refs);
        refs.ModifyOperations(std::bind(_FixReference, sourceLayer,
                                        std::placeholders::_1));
        val->Swap(refs);
        return;
    }
    else if (val->IsHolding<SdfPayload>()) {
        SdfPayload pl;
        val->Swap(pl);
        pl.SetAssetPath(
            SdfComputeAssetPathRelativeToLayer(sourceLayer,
                                               pl.GetAssetPath()));
        val->Swap(pl);
        return;
    }
}

// List of fields that we do not want to flatten generically.
TF_MAKE_STATIC_DATA(std::set<TfToken>, _fieldsToSkip) {
    // SdfChildrenKeys fields are maintained internally by Sdf.
    _fieldsToSkip->insert(SdfChildrenKeys->allTokens.begin(),
                           SdfChildrenKeys->allTokens.end());
    // We need to go through the SdfListEditorProxy API to
    // properly create attribute connections and rel targets,
    // so don't process the fields.
    _fieldsToSkip->insert(SdfFieldKeys->TargetPaths);
    _fieldsToSkip->insert(SdfFieldKeys->ConnectionPaths);
    // We flatten out sublayers, so discard them.
    _fieldsToSkip->insert(SdfFieldKeys->SubLayers);
    _fieldsToSkip->insert(SdfFieldKeys->SubLayerOffsets);
    // TimeSamples may be masked by Defaults, so handle them separately.
    _fieldsToSkip->insert(SdfFieldKeys->TimeSamples);
}

static VtValue
_ReduceField(const PcpLayerStackRefPtr &layerStack,
             const SdfSpecHandle &targetSpec,
             const TfToken &field)
{
    const SdfLayerRefPtrVector &layers = layerStack->GetLayers();
    const SdfPath &path = targetSpec->GetPath();
    const SdfSpecType specType = targetSpec->GetSpecType();

    VtValue val;
    for (size_t i=0; i < layers.size(); ++i) {
        if (!layers[i]->HasSpec(path)) {
            continue;
        }
        // Ignore mismatched specs (which should be very rare).
        // An example would a property that is declared as an
        // attribute in one layer, and a relationship in another.
        if (layers[i]->GetSpecType(path) != specType) {
            TF_WARN("UsdUtilsFlattenLayerStack: Ignoring spec at "
                    "<%s> in @%s@: expected spec type %s but found %s",
                    path.GetText(),
                    layers[i]->GetIdentifier().c_str(),
                    TfStringify(specType).c_str(),
                    TfStringify(layers[i]->GetSpecType(path)).c_str());
            continue;
        }
        VtValue layerVal;
        if (!layers[i]->HasField(path, field, &layerVal)) {
            continue;
        }
        // Apply layer offsets.
        if (const SdfLayerOffset *offset =
            layerStack->GetLayerOffsetForLayer(i)) {
            layerVal = _ApplyLayerOffset(*offset, field, layerVal);
        }
        // Fix asset paths.
        _FixAssetPaths(layers[i], field, &layerVal);
        val = _Reduce(val, layerVal, field);
    }
    return val;
}

static void
_FlattenFields(const PcpLayerStackRefPtr &layerStack,
               const SdfSpecHandle &targetSpec)
{
    const SdfLayerRefPtrVector &layers = layerStack->GetLayers();
    const SdfSchemaBase &schema = targetSpec->GetLayer()->GetSchema();
    const SdfSpecType specType = targetSpec->GetSpecType();
    const SdfPath &path = targetSpec->GetPath();
    for (const TfToken &field: schema.GetFields(specType)) {
        if (_fieldsToSkip->find(field) != _fieldsToSkip->end()) {
            continue;
        }
        VtValue val = _ReduceField(layerStack, targetSpec, field);
        targetSpec->GetLayer()->SetField(path, field, val);
    }
    if (specType == SdfSpecTypeAttribute) {
        // Only flatten TimeSamples if not masked by stronger Defaults.
        for (size_t i=0; i < layers.size(); ++i) {
            if (layers[i]->HasField(path, SdfFieldKeys->TimeSamples)) {
                VtValue val = _ReduceField(layerStack, targetSpec,
                                           SdfFieldKeys->TimeSamples);
                targetSpec->GetLayer()
                    ->SetField(path, SdfFieldKeys->TimeSamples, val);
                break;
            } else if (layers[i]->HasField(path, SdfFieldKeys->Default)) {
                // This layer has defaults that mask any underlying
                // TimeSamples in weaker layers.
                break;
            }
        }
    }
}

static SdfSpecType
_GetSiteSpecType(const SdfLayerRefPtrVector &layers, const SdfPath &path)
{
    for (const auto &l: layers) {
        if (l->HasSpec(path)) {
            return l->GetSpecType(path);
        } 
    }
    return SdfSpecTypeUnknown;
}

// Fwd decl
static void
_FlattenSpec(const PcpLayerStackRefPtr &layerStack,
             const SdfPrimSpecHandle &prim);

static void
_FlattenSpec(const PcpLayerStackRefPtr &layerStack,
             const SdfVariantSpecHandle &var)
{
    _FlattenSpec(layerStack, var->GetPrimSpec());
}

static void
_FlattenSpec(const PcpLayerStackRefPtr &layerStack,
             const SdfVariantSetSpecHandle &vset)
{
    // Variants
    TfTokenVector nameOrder;
    PcpTokenSet nameSet;
    PcpComposeSiteChildNames(layerStack->GetLayers(), vset->GetPath(),
                             SdfChildrenKeys->VariantChildren,
                             &nameOrder, &nameSet);
    for (const TfToken &varName: nameOrder) {
        if (SdfVariantSpecHandle var = SdfVariantSpec::New(vset, varName)) {
            _FlattenFields(layerStack, var);
            _FlattenSpec(layerStack, var);
        }
    }
}

// Attribute connections / relationship targets
static void
_FlattenTargetPaths(const PcpLayerStackRefPtr &layerStack,
                    const SdfSpecHandle &spec,
                    const TfToken &field,
                    SdfPathEditorProxy targetProxy)
{
    VtValue val = _ReduceField(layerStack, spec, field);
    if (val.IsHolding<SdfPathListOp>()) {
        SdfPathListOp listOp = val.UncheckedGet<SdfPathListOp>();
        // We want to recreate the set of listOp operations, but we
        // must go through the proxy editor in order for the target
        // path specs to be created as a side effect.  So, we replay the
        // operations against the proxy.
        if (listOp.IsExplicit()) {
            targetProxy.ClearEditsAndMakeExplicit();
            targetProxy.GetExplicitItems() = listOp.GetExplicitItems();
        } else {
            targetProxy.ClearEdits();
            targetProxy.GetPrependedItems() = listOp.GetPrependedItems();
            targetProxy.GetAppendedItems() = listOp.GetAppendedItems();
            targetProxy.GetDeletedItems() = listOp.GetDeletedItems();
            // We deliberately do not handle reordered or added items.
        }
    }
}

void
_FlattenSpec(const PcpLayerStackRefPtr &layerStack,
             const SdfPrimSpecHandle &prim)
{
    const SdfLayerRefPtrVector &layers = layerStack->GetLayers();

    // Child prims
    TfTokenVector nameOrder;
    PcpTokenSet nameSet;
    PcpComposeSiteChildNames(layers, prim->GetPath(),
                             SdfChildrenKeys->PrimChildren,
                             &nameOrder, &nameSet,
                             &SdfFieldKeys->PrimOrder);
    for (const TfToken &childName: nameOrder) {
        // Use SdfSpecifierDef as a placeholder specifier; it will be
        // fixed up when we _FlattenFields().
        if (SdfPrimSpecHandle child =
            SdfPrimSpec::New(prim, childName, SdfSpecifierDef)) {
            _FlattenFields(layerStack, child);
            _FlattenSpec(layerStack, child);
        }
    }

    if (prim->GetSpecType() == SdfSpecTypePseudoRoot) {
        return;
    }

    // Variant sets
    nameOrder.clear();
    nameSet.clear();
    PcpComposeSiteChildNames(layers, prim->GetPath(),
                             SdfChildrenKeys->VariantSetChildren,
                             &nameOrder, &nameSet);
    for (const TfToken &vsetName: nameOrder) {
        if (SdfVariantSetSpecHandle vset =
            SdfVariantSetSpec::New(prim, vsetName)) {
            _FlattenFields(layerStack, vset);
            _FlattenSpec(layerStack, vset);
        }
    }

    // Properties
    nameOrder.clear();
    nameSet.clear();
    PcpComposeSiteChildNames(layers, prim->GetPath(),
                             SdfChildrenKeys->PropertyChildren,
                             &nameOrder, &nameSet);
    for (const TfToken &childName: nameOrder) {
        SdfPath childPath = prim->GetPath().AppendProperty(childName);
        SdfSpecType specType = _GetSiteSpecType(layers, childPath);
        if (specType == SdfSpecTypeAttribute) {
            // Use Int as a (required) placeholder type; it will
            // be updated when we _FlattenFields().
            if (SdfAttributeSpecHandle attr =
                SdfAttributeSpec::New(prim, childName,
                                      SdfValueTypeNames->Int)) {
                _FlattenFields(layerStack, attr);
                _FlattenTargetPaths(layerStack, attr,
                                    SdfFieldKeys->ConnectionPaths,
                                    attr->GetConnectionPathList());
            }
        } else if (specType == SdfSpecTypeRelationship) {
            if (SdfRelationshipSpecHandle rel =
                SdfRelationshipSpec::New(prim, childName)) {
                _FlattenFields(layerStack, rel);
                _FlattenTargetPaths(layerStack, rel,
                                    SdfFieldKeys->TargetPaths,
                                    rel->GetTargetPathList());
            }
        } else {
            TF_RUNTIME_ERROR("Unknown spec type %s at <%s> in %s\n",
                             TfStringify(specType).c_str(),
                             childPath.GetText(),
                             TfStringify(layerStack).c_str());
            continue;
        }
    }
}

SdfLayerRefPtr
UsdUtils_FlattenLayerStack(const PcpLayerStackRefPtr &layerStack,
                           std::string tag)
{
    // XXX Currently, SdfLayer::CreateAnonymous() examines the tag
    // file extension to determine the file type.  Provide an
    // extension here if needed to ensure that we get a usda file.
    if (!TfStringEndsWith(tag, ".usda")) {
        tag += ".usda";
    }
    ArResolverContextBinder arBinder(
        layerStack->GetIdentifier().pathResolverContext);
    SdfChangeBlock changeBlock;
    SdfLayerRefPtr outputLayer = SdfLayer::CreateAnonymous(tag);
    _FlattenFields(layerStack, outputLayer->GetPseudoRoot());
    _FlattenSpec(layerStack, outputLayer->GetPseudoRoot());
    return outputLayer;
}

SdfLayerRefPtr
UsdUtilsFlattenLayerStack(const UsdStagePtr &stage, const std::string& tag)
{
    PcpPrimIndex index = stage->GetPseudoRoot().GetPrimIndex();
    return UsdUtils_FlattenLayerStack(index.GetRootNode().GetLayerStack(), tag);
}

PXR_NAMESPACE_CLOSE_SCOPE
