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
#include "pxr/usd/sdf/copyUtils.h"

#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/childrenPolicies.h"
#include "pxr/usd/sdf/childrenUtils.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"

#include <deque>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

    // A _CopyStackEntry is a (source path, destination path) pair indicating
    // a spec that should be copied.
    struct _CopyStackEntry {
        SdfPath srcPath;
        SdfPath dstPath;

        _CopyStackEntry(const SdfPath& srcPath, const SdfPath& dstPath) :
            srcPath(srcPath), dstPath(dstPath) { };
    };

    typedef std::deque<_CopyStackEntry> _CopyStack;

    // Collection of (field name, value) pairs.
    typedef std::pair<TfToken, VtValue> _FieldValuePair;
    typedef std::vector<_FieldValuePair> _FieldValueList;

    // A _SpecDataEntry contains all of the information being copied for a 
    // single spec.
    struct _SpecDataEntry {
        _SpecDataEntry(const SdfPath& dstPath_, SdfSpecType specType_)
            : dstPath(dstPath_), specType(specType_) { }
        
        // Destination path to which this spec data should be copied.
        SdfPath dstPath;

        // Type of spec this entry represents.
        SdfSpecType specType;

        // List containing (field, value) pairs of data to be copied to
        // the destination spec.
        _FieldValueList dataToCopy;
    };

    typedef std::vector<_SpecDataEntry> _CopyEntryList;

} // end anonymous namespace

// Returns lists of value and children field names to be handled during
// the copy process. The returned lists are sorted using the
// TfTokenFastArbitraryLessThan comparator.
static
void
_GetFieldNames(
    const SdfLayerHandle& layer, const SdfPath& path, 
    std::vector<TfToken>* valueFields, 
    std::vector<TfToken>* childrenFields)
{
    const SdfSchemaBase& schema = layer->GetSchema();
    const std::vector<TfToken> allFields = layer->ListFields(path);
    for (const TfToken& field : allFields) {
        if (schema.HoldsChildren(field)) {
            childrenFields->push_back(field);
        }
        else {
            valueFields->push_back(field);
        }
    }

    TfTokenFastArbitraryLessThan lessThan;
    std::sort(valueFields->begin(), valueFields->end(), lessThan);
    std::sort(childrenFields->begin(), childrenFields->end(), lessThan);
}

// Add a (field, value) entry to the list of fields to copy as directed by
// the given policy. The value may be empty to indicate that the field
// should be removed from the destination.
static void
_AddFieldValueToCopy(
    SdfSpecType specType, const TfToken& field,
    const SdfLayerHandle& srcLayer, const SdfPath& srcPath, bool fieldInSrc,
    const SdfLayerHandle& dstLayer, const SdfPath& dstPath, bool fieldInDst,
    const SdfShouldCopyValueFn& shouldCopyValue, _FieldValueList* valueList)
{
    boost::optional<VtValue> value;
    if (shouldCopyValue(
            specType, field, 
            srcLayer, srcPath, fieldInSrc, dstLayer, dstPath, fieldInDst, 
            &value)) {

        valueList->emplace_back(
            field, value ? *value : srcLayer->GetField(srcPath, field));
    }
}

// Process the given children and add any children specs that are indicated by
// the copy policy to the list of specs to be copied.
template <class ChildPolicy>
static void
_ProcessChildren(
    const TfToken& childrenField, 
    const VtValue& srcChildrenValue, const VtValue& dstChildrenValue,
    const SdfLayerHandle& srcLayer, const SdfPath& srcPath, bool childrenInSrc,
    const SdfLayerHandle& dstLayer, const SdfPath& dstPath, bool childrenInDst,
    _CopyStack* copyStack)
{
    typedef typename ChildPolicy::FieldType FieldType;
    typedef std::vector<FieldType> ChildrenVector;

    if (!TF_VERIFY(srcChildrenValue.IsHolding<ChildrenVector>() || 
                   srcChildrenValue.IsEmpty()) ||
        !TF_VERIFY(dstChildrenValue.IsHolding<ChildrenVector>() || 
                   dstChildrenValue.IsEmpty())) {
        return;
    }

    const ChildrenVector emptyChildren;
    const ChildrenVector& srcChildren = srcChildrenValue.IsEmpty() ? 
        emptyChildren : srcChildrenValue.UncheckedGet<ChildrenVector>();
    const ChildrenVector& dstChildren = dstChildrenValue.IsEmpty() ? 
        emptyChildren : dstChildrenValue.UncheckedGet<ChildrenVector>();

    for (size_t i = 0; i < srcChildren.size(); ++i) {
        if (srcChildren[i].IsEmpty() || dstChildren[i].IsEmpty()) {
            continue;
        }

        const SdfPath srcChildPath = 
            ChildPolicy::GetChildPath(srcPath, srcChildren[i]);
        const SdfPath dstChildPath = 
            ChildPolicy::GetChildPath(dstPath, dstChildren[i]);

        copyStack->emplace_back(srcChildPath, dstChildPath);
    }

    // Add entries to the copy stack to mark the removal of child specs
    // in the destination layer that aren't included in the list of children
    // to copy.
    if (childrenInDst) {
        const VtValue oldDstChildrenValue = 
            dstLayer->GetField(dstPath, childrenField);
        if (!TF_VERIFY(oldDstChildrenValue.IsHolding<ChildrenVector>())) {
            return;
        }

        for (const auto& oldDstChild : 
                 oldDstChildrenValue.UncheckedGet<ChildrenVector>()) {
            if (std::find(dstChildren.begin(), dstChildren.end(), 
                    oldDstChild) == dstChildren.end()) {
                
                const SdfPath oldDstChildPath = 
                    ChildPolicy::GetChildPath(dstPath, oldDstChild);
                copyStack->emplace_back(SdfPath(), oldDstChildPath);
            }
        }
    }
}

static void
_ProcessChildField(
    const TfToken& childField, 
    const SdfLayerHandle& srcLayer, const SdfPath& srcPath, bool childrenInSrc,
    const SdfLayerHandle& dstLayer, const SdfPath& dstPath, bool childrenInDst,
    const SdfShouldCopyChildrenFn& shouldCopyChildren, _CopyStack* copyStack)
{
    boost::optional<VtValue> srcChildrenToCopy, dstChildrenToCopy;
    if (!shouldCopyChildren(
            childField, 
            srcLayer, srcPath, childrenInSrc, dstLayer, dstPath, childrenInDst,
            &srcChildrenToCopy, &dstChildrenToCopy)) {
        return;
    }

    if (!srcChildrenToCopy || !dstChildrenToCopy) {
        srcChildrenToCopy = srcLayer->GetField(srcPath, childField);
        dstChildrenToCopy = srcChildrenToCopy;
    }

    const VtValue& srcChildren = *srcChildrenToCopy;
    const VtValue& dstChildren = *dstChildrenToCopy;

    if (childField == SdfChildrenKeys->ConnectionChildren) {
        _ProcessChildren<Sdf_AttributeConnectionChildPolicy>(
            childField, srcChildren, dstChildren,
            srcLayer, srcPath, childrenInSrc, dstLayer, dstPath, childrenInDst,
            copyStack);
        return;
    }
    if (childField == SdfChildrenKeys->MapperChildren) {
        _ProcessChildren<Sdf_MapperChildPolicy>(
            childField, srcChildren, dstChildren,
            srcLayer, srcPath, childrenInSrc, dstLayer, dstPath, childrenInDst,
            copyStack);
        return;
    }
    if (childField == SdfChildrenKeys->MapperArgChildren) {
        _ProcessChildren<Sdf_MapperArgChildPolicy>(
            childField, srcChildren, dstChildren,
            srcLayer, srcPath, childrenInSrc, dstLayer, dstPath, childrenInDst,
            copyStack);
        return;
    }
    if (childField == SdfChildrenKeys->ExpressionChildren) {
        _ProcessChildren<Sdf_ExpressionChildPolicy>(
            childField, srcChildren, dstChildren,
            srcLayer, srcPath, childrenInSrc, dstLayer, dstPath, childrenInDst,
            copyStack);
        return;
    }
    if (childField == SdfChildrenKeys->RelationshipTargetChildren) {
        _ProcessChildren<Sdf_RelationshipTargetChildPolicy>(
            childField, srcChildren, dstChildren,
            srcLayer, srcPath, childrenInSrc, dstLayer, dstPath, childrenInDst,
            copyStack);
        return;
    }
    if (childField == SdfChildrenKeys->VariantChildren) {
        _ProcessChildren<Sdf_VariantChildPolicy>(
            childField, srcChildren, dstChildren,
            srcLayer, srcPath, childrenInSrc, dstLayer, dstPath, childrenInDst,
            copyStack);
        return;
    }
    if (childField == SdfChildrenKeys->VariantSetChildren) {
        _ProcessChildren<Sdf_VariantSetChildPolicy>(
            childField, srcChildren, dstChildren,
            srcLayer, srcPath, childrenInSrc, dstLayer, dstPath, childrenInDst,
            copyStack);
        return;
    }
    if (childField == SdfChildrenKeys->PropertyChildren) {
        _ProcessChildren<Sdf_PropertyChildPolicy>(
            childField, srcChildren, dstChildren,
            srcLayer, srcPath, childrenInSrc, dstLayer, dstPath, childrenInDst,
            copyStack);
        return;
    }
    if (childField == SdfChildrenKeys->PrimChildren) {
        _ProcessChildren<Sdf_PrimChildPolicy>(
            childField, srcChildren, dstChildren,
            srcLayer, srcPath, childrenInSrc, dstLayer, dstPath, childrenInDst,
            copyStack);
        return;
    }

    TF_CODING_ERROR("Unknown child field '%s'", childField.GetText());
}

// Helpers to add a new spec to the given layer.
template <class ChildPolicy>
static void
_DoAddNewSpec(const SdfLayerHandle& destLayer, const _SpecDataEntry& specData)
{
    Sdf_ChildrenUtils<ChildPolicy>::CreateSpec(destLayer, specData.dstPath,
        specData.specType);
}

static
void _DoAddNewPrimSpec(
    const SdfLayerHandle& destLayer, const _SpecDataEntry& specData)
{
    // Need to determine whether this property is considered inert when
    // being initially created based on fields being copied in. This mimics
    // what's done in the SdfPrimSpec constructor.
    TfToken type;
    SdfSpecifier specifier = SdfSpecifierOver;
    for (const _FieldValuePair& fieldValue : specData.dataToCopy) {
        if (fieldValue.second.IsEmpty()) {
            continue;
        }

        if (fieldValue.first == SdfFieldKeys->TypeName) {
            type = fieldValue.second.Get<TfToken>();
        }
        else if (fieldValue.first == SdfFieldKeys->Specifier) {
            specifier = fieldValue.second.Get<SdfSpecifier>();
        }
    }

    const bool inert = (specifier == SdfSpecifierOver && type.IsEmpty());
    Sdf_ChildrenUtils<Sdf_PrimChildPolicy>::CreateSpec(
        destLayer, specData.dstPath, SdfSpecTypePrim,
        /* inert = */ inert);
}

template <class ChildPolicy>
static void
_DoAddNewPropertySpec(
    const SdfLayerHandle& destLayer, const _SpecDataEntry& specData)
{
    // Need to determine whether this property is considered to have only 
    // required fields when being initially created based on fields being 
    // copied in. This mimics what's done in the 
    // SdfAttributeSpec/SdfRelationshipSpec constructors.
    bool custom = false;
    for (const _FieldValuePair& fieldValue : specData.dataToCopy) {
        if (fieldValue.first == SdfFieldKeys->Custom) {
            custom = fieldValue.second.template Get<bool>();
            break;
        }
    }

    const bool hasOnlyRequiredFields = (!custom);
    Sdf_ChildrenUtils<ChildPolicy>::CreateSpec(
        destLayer, specData.dstPath, specData.specType, 
        /* hasOnlyRequiredFields = */ hasOnlyRequiredFields);
}

static void
_AddNewSpecToLayer(
    const SdfLayerHandle& destLayer, const _SpecDataEntry& specData)
{
    if (destLayer->HasSpec(specData.dstPath)) {
        return;
    }

    switch (specData.specType) {
    case SdfSpecTypeAttribute:
        _DoAddNewPropertySpec<Sdf_AttributeChildPolicy>(destLayer, specData);
        break;
    case SdfSpecTypeConnection:
        _DoAddNewSpec<Sdf_AttributeConnectionChildPolicy>(destLayer, specData);
        break;
    case SdfSpecTypeExpression:
        _DoAddNewSpec<Sdf_ExpressionChildPolicy>(destLayer, specData);
        break;
    case SdfSpecTypeMapper:
        _DoAddNewSpec<Sdf_MapperChildPolicy>(destLayer, specData);
        break;
    case SdfSpecTypeMapperArg:
        _DoAddNewSpec<Sdf_MapperArgChildPolicy>(destLayer, specData);
        break;
    case SdfSpecTypePrim:
        _DoAddNewPrimSpec(destLayer, specData);
        break;
    case SdfSpecTypeRelationship:
        _DoAddNewPropertySpec<Sdf_RelationshipChildPolicy>(destLayer, specData);
        break;
    case SdfSpecTypeRelationshipTarget:
        _DoAddNewSpec<Sdf_RelationshipTargetChildPolicy>(destLayer, specData);
        break;
    case SdfSpecTypeVariant:
        _DoAddNewSpec<Sdf_VariantChildPolicy>(destLayer, specData);
        break;
    case SdfSpecTypeVariantSet:
        _DoAddNewSpec<Sdf_VariantSetChildPolicy>(destLayer, specData);
        break;

    case SdfSpecTypePseudoRoot:
    case SdfSpecTypeUnknown:
    case SdfNumSpecTypes:
        break;
    }
}

template <class ChildPolicy>
static void
_DoRemoveSpec(
    const SdfLayerHandle& dstLayer, const _SpecDataEntry& specData)
{
    Sdf_ChildrenUtils<ChildPolicy>::RemoveChild(
        dstLayer, 
        ChildPolicy::GetParentPath(specData.dstPath),
        ChildPolicy::GetFieldValue(specData.dstPath));
}

static void
_RemoveSpecFromLayer(
    const SdfLayerHandle& dstLayer, const _SpecDataEntry& specData)
{
    switch (dstLayer->GetSpecType(specData.dstPath)) {
    case SdfSpecTypeAttribute:
        _DoRemoveSpec<Sdf_AttributeChildPolicy>(dstLayer, specData);
        break;
    case SdfSpecTypeConnection:
        _DoRemoveSpec<Sdf_AttributeConnectionChildPolicy>(dstLayer, specData);
        break;
    case SdfSpecTypeExpression:
        _DoRemoveSpec<Sdf_ExpressionChildPolicy>(dstLayer, specData);
        break;
    case SdfSpecTypeMapper:
        _DoRemoveSpec<Sdf_MapperChildPolicy>(dstLayer, specData);
        break;
    case SdfSpecTypeMapperArg:
        _DoRemoveSpec<Sdf_MapperArgChildPolicy>(dstLayer, specData);
        break;
    case SdfSpecTypePrim:
        _DoRemoveSpec<Sdf_PrimChildPolicy>(dstLayer, specData);
        break;
    case SdfSpecTypeRelationship:
        _DoRemoveSpec<Sdf_RelationshipChildPolicy>(dstLayer, specData);
        break;
    case SdfSpecTypeRelationshipTarget:
        _DoRemoveSpec<Sdf_RelationshipTargetChildPolicy>(dstLayer, specData);
        break;
    case SdfSpecTypeVariant:
        _DoRemoveSpec<Sdf_VariantChildPolicy>(dstLayer, specData);
        break;
    case SdfSpecTypeVariantSet:
        _DoRemoveSpec<Sdf_VariantSetChildPolicy>(dstLayer, specData);
        break;

    case SdfSpecTypePseudoRoot:
    case SdfSpecTypeUnknown:
    case SdfNumSpecTypes:
        break;
    }
}

// Call the given function for each field in srcFields and dstFields.
// The function will be called once for each unique field and will be 
// passed flags that indicate which container the field was in.
//
// The callable must have the signature: 
//    void (const TfToken& field, bool fieldInSrc, bool fieldInDst)
//
// srcFields and dstFields must be sorted using the TfTokenArbitraryLessThan
// comparator prior to calling this function.
//
template <class Callable>
static void
_ForEachField(
    const std::vector<TfToken>& srcFields,
    const std::vector<TfToken>& dstFields,
    const Callable& fn)
{
    auto lessThan = TfTokenFastArbitraryLessThan();

    auto srcIt = srcFields.begin(), srcEndIt = srcFields.end();
    auto dstIt = dstFields.begin(), dstEndIt = dstFields.end();
    while (srcIt != srcEndIt && dstIt != dstEndIt) {
        if (*srcIt == *dstIt) {
            fn(*srcIt, /* inSrc = */ true, /* inDst = */ true);
            ++srcIt, ++dstIt;
        }
        else if (lessThan(*srcIt, *dstIt)) {
            for (; srcIt != srcEndIt && lessThan(*srcIt, *dstIt); ++srcIt) {
                fn(*srcIt, /* inSrc = */ true, /* inDst = */ false);
            }
        }
        else {
            for (; dstIt != dstEndIt && lessThan(*dstIt, *srcIt); ++dstIt) {
                fn(*dstIt, /* inSrc = */ false, /* inDst = */ true);
            }
        }
    }

    auto finalIt = (srcIt == srcEndIt) ? dstIt : srcIt;
    auto finalEndIt = (srcIt == srcEndIt) ? dstEndIt : srcEndIt;
    const bool inSrc = (finalIt == srcIt);

    for (; finalIt != finalEndIt; ++finalIt) {
        fn(*finalIt, /* inSrc = */ inSrc, /* inDst = */ !inSrc);
    }
}

bool 
SdfCopySpec(
    const SdfLayerHandle& srcLayer, const SdfPath& srcPath,
    const SdfLayerHandle& dstLayer, const SdfPath& dstPath,
    const SdfShouldCopyValueFn& shouldCopyValueFn,
    const SdfShouldCopyChildrenFn& shouldCopyChildrenFn)
{
    if (!srcLayer || !dstLayer) {
        TF_CODING_ERROR("Invalid layer handle");
        return false;
    }

    if (srcPath.IsEmpty() || dstPath.IsEmpty()) {
        TF_CODING_ERROR("Invalid empty path");
        return false;
    }

    // Validate compatible source and destination path types.
    if ((srcPath.IsAbsoluteRootOrPrimPath()
                || srcPath.IsPrimVariantSelectionPath())
            != (dstPath.IsAbsoluteRootOrPrimPath()
                || dstPath.IsPrimVariantSelectionPath())
            || srcPath.IsPropertyPath() != dstPath.IsPropertyPath()
            || srcPath.IsTargetPath() != dstPath.IsTargetPath()
            || srcPath.IsMapperPath() != dstPath.IsMapperPath()
            || srcPath.IsMapperArgPath() != dstPath.IsMapperArgPath()
            || srcPath.IsExpressionPath() != dstPath.IsExpressionPath()) {
        TF_CODING_ERROR("Incompatible source and destination paths");
        return false;
    }

    // For target paths (relationship targets and connections), verify the
    // destination spec already exists.  See the documentation comment.
    if (dstPath.IsTargetPath() && !dstLayer->HasSpec(dstPath)) {
        TF_CODING_ERROR("Spec does not exist at destination target path");
        return false;
    }

    // This function collects all of the data that will be copied for each
    // spec into this list, then applies it to the layer at the very end.
    // This allows us to do some analysis on the data first.
    _CopyEntryList dataToCopy;

    // Create a stack of source/dest copy requests, initially populated with
    // the passed parameters.  The copy routine will add additional requests
    // as needed to handle children etc... and runs until the stack is empty.
    _CopyStack copyStack(1, _CopyStackEntry(srcPath, dstPath));
    while (!copyStack.empty()) {
        const _CopyStackEntry toCopy = copyStack.front();
        copyStack.pop_front();

        // If the source path is empty, it indicates that the spec at the
        // destination path should be removed. Add an entry to the queue
        // to reflect that.
        if (toCopy.srcPath.IsEmpty()) {
            _SpecDataEntry removeEntry(toCopy.dstPath, SdfSpecTypeUnknown);
            dataToCopy.push_back(removeEntry);
            continue;
        }

        // Figure out the concrete type of the spec we're copying. The spec type
        // dictates copying behavior below.
        const SdfSpecType specType = srcLayer->GetSpecType(toCopy.srcPath);
        if (specType == SdfSpecTypeUnknown) {
            TF_CODING_ERROR("Cannot copy unknown spec at <%s> from layer <%s>",
                srcPath.GetText(), srcLayer->GetIdentifier().c_str());
            return false;
        }

        _SpecDataEntry copyEntry(toCopy.dstPath, specType);

        // Determine what data is present for the current source and dest specs
        // and what needs to be copied. Divide the present fields into those
        // that contain values and those that index children specs.
        std::vector<TfToken> dstValueFields;
        std::vector<TfToken> dstChildrenFields;
        _GetFieldNames(
            dstLayer, toCopy.dstPath, &dstValueFields, &dstChildrenFields);

        std::vector<TfToken> srcValueFields;
        std::vector<TfToken> srcChildrenFields;
        _GetFieldNames(
            srcLayer, toCopy.srcPath, &srcValueFields, &srcChildrenFields);

        // From the list of value fields, retrieve all values that the copy
        // policy says we need to copy over to the destination.
        _ForEachField(
            srcValueFields, dstValueFields,
            [&](const TfToken& field, bool fieldInSrc, bool fieldInDst) {
                _AddFieldValueToCopy(
                    specType, field, 
                    srcLayer, toCopy.srcPath, fieldInSrc,
                    dstLayer, toCopy.dstPath, fieldInDst,
                    shouldCopyValueFn, &copyEntry.dataToCopy);
            });
    
        // Add an entry for all of the data we're copying for this spec.
        dataToCopy.push_back(copyEntry);

        // Now add any children specs that need to be copied to our
        // copy stack.
        _ForEachField(
            srcChildrenFields, dstChildrenFields,
            [&](const TfToken& field, bool fieldInSrc, bool fieldInDst) {
                _ProcessChildField(
                    field,
                    srcLayer, toCopy.srcPath, fieldInSrc,
                    dstLayer, toCopy.dstPath, fieldInDst,
                    shouldCopyChildrenFn, &copyStack);
            });
    }

    // Now that we have all the data we want to copy, set it into the 
    // destination layer.
    SdfChangeBlock block;

    for (const _SpecDataEntry& specData : dataToCopy) {
        if (specData.specType == SdfSpecTypeUnknown) {
            _RemoveSpecFromLayer(dstLayer, specData);
        }
        else {
            _AddNewSpecToLayer(dstLayer, specData);
        }

        for (const _FieldValuePair& fieldValue : specData.dataToCopy) {
            dstLayer->SetField(
                specData.dstPath, fieldValue.first, fieldValue.second);
        }
    }
    
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
