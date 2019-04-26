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
#include "pxr/usd/usdUtils/stitch.h"

#include "pxr/usd/sdf/copyUtils.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/sdf/payload.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/usd/sdf/schema.h"

#include "pxr/base/vt/value.h"
#include "pxr/base/tf/warning.h"
#include "pxr/base/tf/token.h"

#include <algorithm>
#include <functional>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

VtValue
_Reduce(
    SdfVariantSelectionMap& stronger,
    const SdfVariantSelectionMap& weaker
)
{
    stronger.insert(weaker.begin(), weaker.end());
    return VtValue::Take(stronger);
}

VtValue
_Reduce(const VtDictionary &stronger, const VtDictionary &weaker)
{
    // Dictionaries compose keys recursively.
    return VtValue(VtDictionaryOverRecursive(stronger, weaker));
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
_Reduce(const SdfListOp<T> &stronger, const SdfListOp<T> &weaker)
{
    boost::optional<SdfListOp<T>> r = stronger.ApplyOperations(weaker);
    if (r) {
        return VtValue(*r);
    }
    // List ops that use added or reordered items cannot, in general, be
    // composed into another listop. In those cases, we fall back to a
    // best-effort approximation by discarding reorders and converting
    // adds to appends.
    r = _FixListOp(stronger).ApplyOperations(_FixListOp(weaker));
    if (r) {
        return VtValue(*r);
    }
    // The approximation used should always be composable,
    // so error if that didn't work.
    TF_CODING_ERROR("Could not reduce listOp %s over %s",
                    TfStringify(stronger).c_str(), TfStringify(weaker).c_str());
    return VtValue();
}

template <class T>
static bool
_MergeValue(
    const TfToken& field, const VtValue& fallback,
    const SdfLayerHandle& srcLayer, const SdfPath& srcPath,
    const SdfLayerHandle& dstLayer, const SdfPath& dstPath,
    boost::optional<VtValue>* valueToCopy)
{
    if (!fallback.IsHolding<T>()) {
        return false;
    }

    T srcValue, dstValue;
    if (!TF_VERIFY(srcLayer->HasField(srcPath, field, &srcValue))
        || !TF_VERIFY(dstLayer->HasField(dstPath, field, &dstValue))) {
        return false;
    }

    VtValue mergedValue = _Reduce(srcValue, dstValue);
    if (!mergedValue.IsEmpty()) {
        (*valueToCopy) = VtValue();
        (*valueToCopy)->Swap(mergedValue);
        return true;
    }

    return false;
}

static bool
_MergeValueFn(
    SdfSpecType specType, const TfToken& field,
    const SdfLayerHandle& srcLayer, const SdfPath& srcPath, bool fieldInSrc,
    const SdfLayerHandle& dstLayer, const SdfPath& dstPath, bool fieldInDst,
    boost::optional<VtValue>* valueToCopy,
    const UsdUtilsStitchValueFn& stitchFn)
{
    TF_VERIFY(srcPath == dstPath);

    if (stitchFn) {
        VtValue value;

        // Note that the source layer corresponds to the weaker layer and
        // the destination layer corresponds to the stronger layer in the
        // callback signature.
        using Status = UsdUtilsStitchValueStatus;
        const Status status = stitchFn(
            field, srcPath, dstLayer, fieldInDst, srcLayer, fieldInSrc, &value);

        if (status == Status::NoStitchedValue) {
            return false;
        }
        else if (status == Status::UseSuppliedValue) {
            (*valueToCopy) = VtValue();
            (*valueToCopy)->Swap(value);
            return true;
        }
        // Fall through to default stitching behavior.
    }

    // Field does not exist in source; don't copy this over, since that will
    // clear the value in the destination.
    if (!fieldInSrc) {
        return false;
    }

    // Field does not exist in destination; just copy whatever's in the
    // source over.
    if (!fieldInDst) {
        return true;
    }

    // Merge specific fields together.
    if (field == SdfFieldKeys->TimeSamples) {
        SdfTimeSampleMap edits;
        for (const double time : srcLayer->ListTimeSamplesForPath(srcPath)) {
            if (!dstLayer->QueryTimeSample(dstPath, time)) {
                VtValue srcSample;
                srcLayer->QueryTimeSample(srcPath, time, &srcSample);
                edits[time].Swap(srcSample);
            }
        }

        if (!edits.empty()) {
            *valueToCopy = VtValue(SdfCopySpecsValueEdit(
                [edits](const SdfLayerHandle& layer, const SdfPath& path) {
                    for (const auto& entry : edits) {
                        layer->SetTimeSample(path, entry.first, entry.second);
                    }
                }));
            return true;
        }
        return false;
    }
    else if (field == SdfFieldKeys->StartTimeCode) {
        double srcStartCode, dstStartCode;
        TF_VERIFY(srcLayer->HasField(srcPath, field, &srcStartCode));
        TF_VERIFY(dstLayer->HasField(dstPath, field, &dstStartCode));
        *valueToCopy = VtValue(std::min(srcStartCode, dstStartCode));
        return true;
    }
    else if (field == SdfFieldKeys->EndTimeCode) {
        double srcEndCode, dstEndCode;
        TF_VERIFY(srcLayer->HasField(srcPath, field, &srcEndCode));
        TF_VERIFY(dstLayer->HasField(dstPath, field, &dstEndCode));
        *valueToCopy = VtValue(std::max(srcEndCode, dstEndCode));
        return true;
    }

    // Validate that certain fields match between both layers,
    // but leave the stronger value in place.
    if (field == SdfFieldKeys->FramesPerSecond) {
        double srcFPS, dstFPS;
        TF_VERIFY(srcLayer->HasField(srcPath, field, &srcFPS));
        TF_VERIFY(dstLayer->HasField(dstPath, field, &dstFPS));
        if (srcFPS != dstFPS) {
            TF_WARN(
                "Mismatched framesPerSecond values in @%s@ and @%s@",
                srcLayer->GetIdentifier().c_str(),
                dstLayer->GetIdentifier().c_str());
        }
        return false;
    }
    else if (field == SdfFieldKeys->TimeCodesPerSecond) {
        double srcTPS, dstTPS;
        TF_VERIFY(srcLayer->HasField(srcPath, field, &srcTPS));
        TF_VERIFY(dstLayer->HasField(dstPath, field, &dstTPS));
        if (srcTPS != dstTPS) {
            TF_WARN(
                "Mismatched timeCodesPerSecond values in @%s@ and @%s@",
                srcLayer->GetIdentifier().c_str(),
                dstLayer->GetIdentifier().c_str());
        }
        return false;
    }
    else if (field == SdfFieldKeys->FramePrecision) {
        double srcPrecision, dstPrecision;
        TF_VERIFY(srcLayer->HasField(srcPath, field, &srcPrecision));
        TF_VERIFY(dstLayer->HasField(dstPath, field, &dstPrecision));
        if (srcPrecision != dstPrecision) {
            TF_WARN(
                "Mismatched framePrecision values in @%s@ and @%s@",
                srcLayer->GetIdentifier().c_str(),
                dstLayer->GetIdentifier().c_str());
        }
        return false;
    }

    // Merge fields based on type. If the field is not one of these types,
    // return false to indicate that the stronger value should not be
    // copied over.
    const VtValue fallback = srcLayer->GetSchema().GetFallback(field); 
    return _MergeValue<VtDictionary>(
        field, fallback, dstLayer, dstPath, srcLayer, srcPath, valueToCopy)
        || _MergeValue<SdfVariantSelectionMap>(
        field, fallback, dstLayer, dstPath, srcLayer, srcPath, valueToCopy)
        || _MergeValue<SdfIntListOp>(
        field, fallback, dstLayer, dstPath, srcLayer, srcPath, valueToCopy)
        || _MergeValue<SdfUIntListOp>(
        field, fallback, dstLayer, dstPath, srcLayer, srcPath, valueToCopy)
        || _MergeValue<SdfUInt64ListOp>(
        field, fallback, dstLayer, dstPath, srcLayer, srcPath, valueToCopy)
        || _MergeValue<SdfTokenListOp>(
        field, fallback, dstLayer, dstPath, srcLayer, srcPath, valueToCopy)
        || _MergeValue<SdfStringListOp>(
        field, fallback, dstLayer, dstPath, srcLayer, srcPath, valueToCopy)
        || _MergeValue<SdfPathListOp>(
        field, fallback, dstLayer, dstPath, srcLayer, srcPath, valueToCopy)
        || _MergeValue<SdfReferenceListOp>(
        field, fallback, dstLayer, dstPath, srcLayer, srcPath, valueToCopy)
        || _MergeValue<SdfPayloadListOp>(
        field, fallback, dstLayer, dstPath, srcLayer, srcPath, valueToCopy)
        || _MergeValue<SdfUnregisteredValueListOp>(
        field, fallback, dstLayer, dstPath, srcLayer, srcPath, valueToCopy)
        ;
}

static bool
_DontCopyChildrenFn(
    const TfToken& childrenField,
    const SdfLayerHandle& srcLayer, const SdfPath& srcPath, bool childrenInSrc,
    const SdfLayerHandle& dstLayer, const SdfPath& dstPath, bool childrenInDst,
    boost::optional<VtValue>* srcChildren, 
    boost::optional<VtValue>* dstChildren)
{
    return false;
}

template <class T>
static bool
_MergeChildren(
    const TfToken& field, const VtValue& fallback,
    const SdfLayerHandle& srcLayer, const SdfPath& srcPath,
    const SdfLayerHandle& dstLayer, const SdfPath& dstPath,
    boost::optional<VtValue>* finalSrcValue, 
    boost::optional<VtValue>* finalDstValue)
{
    if (!fallback.IsHolding<T>()) {
        return false;
    }

    T srcChildren, dstChildren;
    if (!TF_VERIFY(srcLayer->HasField(srcPath, field, &srcChildren))
        || !TF_VERIFY(dstLayer->HasField(dstPath, field, &dstChildren))) {
        return false;
    }

    T finalSrcChildren(dstChildren.size());
    T finalDstChildren(dstChildren);

    for (const auto& srcChild : srcChildren) {
        auto dstChildIt = std::find(
            finalDstChildren.begin(), finalDstChildren.end(), srcChild);
        if (dstChildIt == finalDstChildren.end()) {
            finalSrcChildren.push_back(srcChild);
            finalDstChildren.push_back(srcChild);
        }
        else {
            const size_t idx = 
                std::distance(finalDstChildren.begin(), dstChildIt);
            finalSrcChildren[idx] = srcChild;
        }
    }

    *finalSrcValue = VtValue::Take(finalSrcChildren);
    *finalDstValue = VtValue::Take(finalDstChildren);
    return true;
}

static bool
_MergeChildrenFn(
    const TfToken& childrenField,
    const SdfLayerHandle& srcLayer, const SdfPath& srcPath, bool childrenInSrc,
    const SdfLayerHandle& dstLayer, const SdfPath& dstPath, bool childrenInDst,
    boost::optional<VtValue>* finalSrcChildren, 
    boost::optional<VtValue>* finalDstChildren)
{
    if (!childrenInSrc) {
        // Children on the destination spec are never cleared if the
        // source spec does not have any children of the same type.
        return false;
    }

    if (!childrenInDst) {
        // No children of the given type exist in the destination.
        // Copy all of the children from the source over.
        return true;
    }

    // There are children under both the source and destination spec.
    // We need to merge the two lists. 
    const VtValue fallback = srcLayer->GetSchema().GetFallback(childrenField); 
    if (_MergeChildren<std::vector<TfToken>>(
            childrenField, fallback, srcLayer, srcPath, dstLayer, dstPath, 
            finalSrcChildren, finalDstChildren)
        || _MergeChildren<std::vector<SdfPath>>(
            childrenField, fallback, srcLayer, srcPath, dstLayer, dstPath, 
            finalSrcChildren, finalDstChildren)) {
        return true;
    }

    TF_CODING_ERROR(
        "Children field '%s' holding unexpected type '%s'", 
        childrenField.GetText(), fallback.GetTypeName().c_str());

    return false;
}

} // end anon namespace

// public facing API
// ----------------------------------------------------------------------------

void
UsdUtilsStitchInfo(
    const SdfSpecHandle& strongObj,
    const SdfSpecHandle& weakObj)
{
    UsdUtilsStitchInfo(strongObj, weakObj, UsdUtilsStitchValueFn());
}

void
UsdUtilsStitchInfo(
    const SdfSpecHandle& strongObj,
    const SdfSpecHandle& weakObj,
    const UsdUtilsStitchValueFn& stitchValueFn)
{
    namespace ph = std::placeholders;

    SdfCopySpec(
        weakObj->GetLayer(), weakObj->GetPath(),
        strongObj->GetLayer(), strongObj->GetPath(),
        /* shouldCopyValueFn = */ std::bind(
            _MergeValueFn, 
            ph::_1, ph::_2, ph::_3, ph::_4, ph::_5, ph::_6, ph::_7, 
            ph::_8, ph::_9, std::cref(stitchValueFn)),
        /* shouldCopyChildrenFn = */ _DontCopyChildrenFn);
}

void 
UsdUtilsStitchLayers(
    const SdfLayerHandle& strongLayer,
    const SdfLayerHandle& weakLayer)
{
    UsdUtilsStitchLayers(strongLayer, weakLayer, UsdUtilsStitchValueFn());
}

void 
UsdUtilsStitchLayers(
    const SdfLayerHandle& strongLayer,
    const SdfLayerHandle& weakLayer,
    const UsdUtilsStitchValueFn& stitchValueFn)
{
    namespace ph = std::placeholders;
  
    SdfCopySpec(
        weakLayer, SdfPath::AbsoluteRootPath(),
        strongLayer, SdfPath::AbsoluteRootPath(),
        /* shouldCopyValueFn = */ std::bind(
            _MergeValueFn, 
            ph::_1, ph::_2, ph::_3, ph::_4, ph::_5, ph::_6, ph::_7, 
            ph::_8, ph::_9, std::cref(stitchValueFn)),
        /* shouldCopyChildrenFn = */ _MergeChildrenFn);
}

PXR_NAMESPACE_CLOSE_SCOPE

