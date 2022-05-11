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
#include "pxr/usd/usd/prim.h"

#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/inherits.h"
#include "pxr/usd/usd/instanceCache.h"
#include "pxr/usd/usd/payloads.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/references.h"
#include "pxr/usd/usd/resolver.h"
#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/specializes.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/tokens.h"
#include "pxr/usd/usd/variantSets.h"

#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/sdf/primSpec.h"

#include "pxr/base/plug/registry.h"
#include "pxr/base/work/dispatcher.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/singularTask.h"
#include "pxr/base/work/utils.h"
#include "pxr/base/work/withScopedParallelism.h"

#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/pxrTslRobinMap/robin_set.h"

#include <boost/functional/hash.hpp>

#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_set.h>
#include <tbb/parallel_sort.h>

#include <algorithm>
#include <functional>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

UsdPrim
UsdPrim::GetChild(const TfToken &name) const
{
    return GetStage()->GetPrimAtPath(GetPath().AppendChild(name));
}

bool
UsdPrim::_IsA(const TfType& schemaType, bool validateSchemaType) const
{
    if (validateSchemaType) {
        // Check Schema TfType
        if (schemaType.IsUnknown()) {
            TF_CODING_ERROR("Unknown schema type (%s) is invalid for IsA query",
                            schemaType.GetTypeName().c_str());
            return false;
        }
    }

    // Check that the actual schema type of the prim (accounts for fallback
    // types for types with no schema) is or derives from the passed in type.
    return GetPrimTypeInfo().GetSchemaType().IsA(schemaType);
}

bool
UsdPrim::IsA(const TfType& schemaType) const
{
    return _IsA(schemaType, true);
}

bool
UsdPrim::_HasSingleApplyAPI(const TfType& schemaType) const 
{
    TRACE_FUNCTION();

    // Get our composed set of all applied schemas.
    auto appliedSchemas = GetAppliedSchemas();
    if (appliedSchemas.empty()) {
        return false;
    }

    // The compile time and runtime schema type validation of HasAPI should
    // ensure that this schemaName won't be empty so we don't check for that 
    // here.
    const TfToken schemaName = 
        UsdSchemaRegistry::GetAPISchemaTypeName(schemaType);

    // Since this is a single apply API we're just looking for schemaName being
    // in the list.
    return std::find(appliedSchemas.begin(), appliedSchemas.end(), schemaName) 
        != appliedSchemas.end();
}

bool 
UsdPrim::_HasMultiApplyAPI(const TfType& schemaType,
                           const TfToken &instanceName) const
{
    TRACE_FUNCTION();

    // Get our composed set of all applied schemas.
    auto appliedSchemas = GetAppliedSchemas();
    if (appliedSchemas.empty()) {
        return false;
    }

    // The compile time and runtime schema type validation of HasAPI should
    // ensure that this schemaName won't be empty so we don't check for that 
    // here.
    const TfToken schemaName = 
        UsdSchemaRegistry::GetAPISchemaTypeName(schemaType);

    // If instance name is empty, we're looking for any instance of a multiple
    // apply schema of the schemaType. Thus we search for name in the list that
    // starts with the "<schemaName>:" prefix.
    if (instanceName.IsEmpty()) {
        const std::string schemaPrefix = 
            schemaName.GetString() + UsdObject::GetNamespaceDelimiter();
        return std::any_of(appliedSchemas.begin(), appliedSchemas.end(), 
            [&schemaPrefix](const TfToken &appliedSchema) {
                return TfStringStartsWith(appliedSchema, schemaPrefix);
            });
    } 

    // Otherwise we have an instance name so we're looking for exact match of
    // "<schemaType>:<instanceName>"
    const TfToken apiName(SdfPath::JoinIdentifier(schemaName, instanceName));
    return std::find(appliedSchemas.begin(), appliedSchemas.end(), apiName) 
        != appliedSchemas.end();
}

bool
UsdPrim::HasAPI(const TfType& schemaType, const TfToken& instanceName) const
{
    if (schemaType.IsUnknown()) {
        TF_CODING_ERROR("HasAPI: Invalid unknown schema type (%s) ",
                        schemaType.GetTypeName().c_str());
        return false;
    }

    if (!UsdSchemaRegistry::GetInstance().IsAppliedAPISchema(schemaType)) {
        TF_CODING_ERROR("HasAPI: provided schema type ( %s ) is not an "
            "applied API schema type.", schemaType.GetTypeName().c_str());
        return false;
    }

    static const auto apiSchemaBaseType = TfType::Find<UsdAPISchemaBase>();
    if (!schemaType.IsA(apiSchemaBaseType) || schemaType == apiSchemaBaseType) {
        TF_CODING_ERROR("HasAPI: provided schema type ( %s ) does not "
            "derive from UsdAPISchemaBase.", 
            schemaType.GetTypeName().c_str());
        return false;
    }

    // If the type is a multi apply API call the multi apply implementation.
    if (UsdSchemaRegistry::GetInstance().IsMultipleApplyAPISchema(schemaType)) {
        return _HasMultiApplyAPI(schemaType, instanceName);
    }

    // Otherwise it's a single apply API 
    if (!instanceName.IsEmpty()) {
        TF_CODING_ERROR("HasAPI: single application API schemas like %s do "
            "not contain an application instanceName ( %s ).",
            schemaType.GetTypeName().c_str(), instanceName.GetText());
        return false;
    }
    return _HasSingleApplyAPI(schemaType);
}

// Runtime validation for the single apply schema non-templated 
// ApplyAPI/CanApplyAPI/RemoveAPI functions. The templated versions perform this
// verification through compile time asserts.
static bool 
_ValidateSingleApplySchemaType(
    const TfType& schemaType, std::string *reason)
{
    if (UsdSchemaRegistry::GetSchemaKind(schemaType) != 
            UsdSchemaKind::SingleApplyAPI) {
        if (reason) {
            *reason = TfStringPrintf(
                "Provided schema type '%s' is not a single-apply API schema "
                "type.", 
                schemaType.GetTypeName().c_str());
        }
        return false;
    }
    return true;
}

// Runtime validation for the multiple apply schema non-templated 
// ApplyAPI/CanApplyAPI/RemoveAPI functions. The templated versions perform this
// verification through compile time asserts.
static bool 
_ValidateMultipleApplySchemaType(
    const TfType& schemaType, std::string *reason)
{
    if (UsdSchemaRegistry::GetSchemaKind(schemaType) != 
            UsdSchemaKind::MultipleApplyAPI) {
        if (reason) {
            *reason = TfStringPrintf(
                "Provided schema type '%s' is not a mutiple-apply API schema "
                "type.", 
                schemaType.GetTypeName().c_str());
        }
        return false;
    }
    return true;
}

// Determines whether the given prim type can have the given API schema applied 
// to it based on a list of types the API "can only be applied to". If the 
// list is empty, this always returns true, otherwise the prim type must be in 
// the list or derived from a type in the list.
static bool 
_IsPrimTypeValidApplyToTarget(const TfType &primType, 
                              const TfToken &apiSchemaTypeName,
                              const TfToken &instanceName,
                              std::string *whyNot)
{
    // Get the list of prim types this API "can only apply to" if any.
    const TfTokenVector &canOnlyApplyToTypes =
        UsdSchemaRegistry::GetAPISchemaCanOnlyApplyToTypeNames(
            apiSchemaTypeName, instanceName);

    // If no "can only apply to" types are found, the schema can be 
    // applied to any prim type (including empty or invalid prims types)
    if (canOnlyApplyToTypes.empty()) {
        return true;
    }

    // If the prim type or any of its ancestor types are in the list, then it's
    // valid!
    if (!primType.IsUnknown()) {
        for (const TfToken &allowedPrimTypeName : canOnlyApplyToTypes) {
            const TfType allowedPrimType = 
                UsdSchemaRegistry::GetTypeFromSchemaTypeName(
                    allowedPrimTypeName);
            if (primType.IsA(allowedPrimType)) {
                return true;
            }
        }
    }

    // Otherwise, it wasn't in the list and can't be applied to.
    if (whyNot) {
        *whyNot = TfStringPrintf(
            "API schema '%s' can only be applied to prims of the following "
            "types: %s.", 
            SdfPath::JoinIdentifier(apiSchemaTypeName, instanceName).c_str(), 
            TfStringJoin(canOnlyApplyToTypes.begin(),
                         canOnlyApplyToTypes.end(), ", ").c_str());
    }
    return false;
}


bool
UsdPrim::_CanApplyAPI(const TfType& apiSchemaType, 
                      std::string *whyNot) const
{
    // The callers of this function will have already validated the schema is
    // single apply either through static asserts or runtime validation 
    // _ValidateSingleApplySchemaType.

    // Can't apply API schemas to an invalid prim
    if (!IsValid()) {
        if (whyNot) {
            *whyNot = "Prim is not valid.";
        }
        return false;
    }

    // Return whether this prim's type is a valid target for applying the given
    // API schema.
    const TfToken apiSchemaTypeName = 
        UsdSchemaRegistry::GetSchemaTypeName(apiSchemaType);
    return _IsPrimTypeValidApplyToTarget(
        GetPrimTypeInfo().GetSchemaType(), 
        apiSchemaTypeName,
        /*instanceName=*/ TfToken(),
        whyNot);
}

bool
UsdPrim::_CanApplyAPI(const TfType& apiSchemaType, 
                      const TfToken& instanceName,
                      std::string *whyNot) const
{
    // The callers of this function will have already validated the schema is
    // mulitple apply either through static asserts or runtime validation 
    // _ValidateMultipleApplySchemaType.

    // Instance name can only be validated at runtime. All API schema functions 
    // treat an empty instance for a multiple apply schema as a coding error.
    if (instanceName.IsEmpty()) {
        TF_CODING_ERROR("CanApplyAPI: for multiple apply API schema %s, a "
                        "non-empty instance name must be provided.",
            apiSchemaType.GetTypeName().c_str());
        return false;
    }

    // Can't apply API schemas to an invalid prim
    if (!IsValid()) {
        if (whyNot) {
            *whyNot = "Prim is not valid.";
        }
        return false;
    }

    const TfToken apiSchemaTypeName = 
        UsdSchemaRegistry::GetSchemaTypeName(apiSchemaType);

    // Multiple apply API schemas may have limitations on what instance names
    // are allowed to be used. Check if the requested instance name is valid.
    if (!UsdSchemaRegistry::IsAllowedAPISchemaInstanceName(
            apiSchemaTypeName, instanceName)) {
        if (whyNot) {
            *whyNot = TfStringPrintf(
                "'%s' is not an allowed instance name for multiple apply API "
                "schema '%s'.", 
                instanceName.GetText(), apiSchemaTypeName.GetText());
        }
        return false;
    }

    // Return whether this prim's type is a valid target for applying the given
    // API schema and instance name.
    return _IsPrimTypeValidApplyToTarget(
        GetPrimTypeInfo().GetSchemaType(), 
        apiSchemaTypeName,
        instanceName,
        whyNot);
}

bool 
UsdPrim::CanApplyAPI(const TfType& schemaType, 
                     std::string *whyNot) const
{
    // Validate that this function is being called on a single apply API schema.
    // Failure is a coding error as the matching template function would fail
    // to compile.
    std::string errorMsg;
    if (!_ValidateSingleApplySchemaType(schemaType, &errorMsg)) {
        TF_CODING_ERROR("CanApplyAPI: %s", errorMsg.c_str());
        if (whyNot) {
            *whyNot = std::move(errorMsg);
        }
        return false;
    }
    return _CanApplyAPI(schemaType, whyNot);
}

bool 
UsdPrim::CanApplyAPI(const TfType& schemaType, 
                     const TfToken& instanceName,
                     std::string *whyNot) const
{
    // Validate that this function is being called on a multiple apply API 
    // schema. Failure is a coding error as the matching template function would
    // fail to compile.
    std::string errorMsg;
    if (!_ValidateMultipleApplySchemaType(schemaType, &errorMsg)) {
        TF_CODING_ERROR("CanApplyAPI: %s", errorMsg.c_str());
        if (whyNot) {
            *whyNot = std::move(errorMsg);
        }
        return false;
    }
    return _CanApplyAPI(schemaType, instanceName, whyNot);
}

bool
UsdPrim::_ApplyAPI(const TfType& schemaType) const
{
    // The callers of this function will have already validated the schema is
    // single apply either through static asserts or runtime validation 
    // _ValidateSingleApplySchemaType.

    // Validate the prim to protect against crashes in the schema generated 
    // SchemaClass::Apply(const UsdPrim &prim) functions when called with a null
    // prim as these generated functions call prim.ApplyAPI<SchemaClass>
    //
    // While we don't typically validate "this" prim for public UsdPrim C++ API,
    // for performance reasons, we opt to do so here since ApplyAPI isn't 
    // performance critical. If ApplyAPI becomes performance critical in the
    // future, we may have to move this validation elsewhere if this validation
    // is problematic.
    if (!IsValid()) {
        TF_CODING_ERROR("Invalid prim '%s'", GetDescription().c_str());
        return false;
    }

    const TfToken typeName = UsdSchemaRegistry::GetSchemaTypeName(schemaType);
    return AddAppliedSchema(typeName);
}

bool
UsdPrim::_ApplyAPI(const TfType& schemaType, const TfToken& instanceName) const
{
    // The callers of this function will have already validated the schema is
    // mulitple apply either through static asserts or runtime validation 
    // _ValidateMultipleApplySchemaType.

    // Instance name can only be validated at runtime. All API schema functions 
    // treat an empty instance for a multiple apply schema as a coding error.
    if (instanceName.IsEmpty()) {
        TF_CODING_ERROR("ApplyAPI: for mutiple apply API schema %s, a "
                        "non-empty instance name must be provided.",
            schemaType.GetTypeName().c_str());
        return false;
    }

    // Validate the prim to protect against crashes in the schema generated 
    // SchemaClass::Apply(const UsdPrim &prim) functions when called with a null
    // prim as these generated functions call prim.ApplyAPI<SchemaClass>
    //
    // While we don't typically validate "this" prim for public UsdPrim C++ API,
    // for performance reasons, we opt to do so here since ApplyAPI isn't 
    // performance critical. If ApplyAPI becomes performance critical in the
    // future, we may have to move this validation elsewhere if this validation
    // is problematic.
    if (!IsValid()) {
        TF_CODING_ERROR("Invalid prim '%s'", GetDescription().c_str());
        return false;
    }

    const TfToken typeName = UsdSchemaRegistry::GetSchemaTypeName(schemaType);
    TfToken apiName(SdfPath::JoinIdentifier(typeName, instanceName));
    return AddAppliedSchema(apiName);
}

bool
UsdPrim::ApplyAPI(const TfType& schemaType) const
{
    // Validate that this function is being called on a single apply API schema.
    // Failure is a coding error as the matching template function would fail
    // to compile.
    std::string errorMsg;
    if (!_ValidateSingleApplySchemaType(schemaType, &errorMsg)) {
        TF_CODING_ERROR("ApplyAPI: %s", errorMsg.c_str());
        return false;
    }
    return _ApplyAPI(schemaType);
}

bool
UsdPrim::ApplyAPI(const TfType& schemaType, const TfToken& instanceName) const
{
    // Validate that this function is being called on a multiple apply API 
    // schema. Failure is a coding error as the matching template function would
    // fail to compile.
    std::string errorMsg;
    if (!_ValidateMultipleApplySchemaType(schemaType, &errorMsg)) {
        TF_CODING_ERROR("ApplyAPI: %s", errorMsg.c_str());
        return false;
    }
    return _ApplyAPI(schemaType, instanceName);
}

bool
UsdPrim::_RemoveAPI(const TfType& schemaType) const
{
    // The callers of this function will have already validated the schema is
    // single apply either through static asserts or runtime validation 
    // _ValidateSingleApplySchemaType.

    const TfToken typeName = UsdSchemaRegistry::GetSchemaTypeName(schemaType);
    return RemoveAppliedSchema(typeName);
}

bool
UsdPrim::_RemoveAPI(const TfType& schemaType, const TfToken& instanceName) const
{
    // The callers of this function will have already validated the schema is
    // mulitple apply either through static asserts or runtime validation 
    // _ValidateMultipleApplySchemaType.

    // Instance name can only be validated at runtime. All API schema functions 
    // treat an empty instance for a multiple apply schema as a coding error.
    if (instanceName.IsEmpty()) {
        TF_CODING_ERROR("RemoveAPI: for mutiple apply API schema %s, a "
                        "non-empty instance name must be provided.",
            schemaType.GetTypeName().c_str());
        return false;
    }

    const TfToken typeName = UsdSchemaRegistry::GetSchemaTypeName(schemaType);
    TfToken apiName(SdfPath::JoinIdentifier(typeName, instanceName));
    return RemoveAppliedSchema(apiName);
}

bool
UsdPrim::RemoveAPI(const TfType& schemaType) const
{
    // Validate that this function is being called on a single apply API schema.
    // Failure is a coding error as the matching template function would fail
    // to compile.
    std::string errorMsg;
    if (!_ValidateSingleApplySchemaType(schemaType, &errorMsg)) {
        TF_CODING_ERROR("RemoveAPI: %s", errorMsg.c_str());
        return false;
    }
    return _RemoveAPI(schemaType);
}

bool
UsdPrim::RemoveAPI(const TfType& schemaType, const TfToken& instanceName) const
{
    // Validate that this function is being called on a multiple apply API 
    // schema. Failure is a coding error as the matching template function would
    // fail to compile.
    std::string errorMsg;
    if (!_ValidateMultipleApplySchemaType(schemaType, &errorMsg)) {
        TF_CODING_ERROR("RemoveAPI: %s", errorMsg.c_str());
        return false;
    }
    return _RemoveAPI(schemaType, instanceName);
}

bool 
UsdPrim::AddAppliedSchema(const TfToken &appliedSchemaName) const
{
    // This should find or create the primSpec in the current edit target.
    // It will also issue an error if it's unable to.
    SdfPrimSpecHandle primSpec = _GetStage()->_CreatePrimSpecForEditing(*this);

    // _CreatePrimSpecForEditing would have already issued a runtime error
    // in case of a failure.
    if (!primSpec) {
        TF_WARN("Unable to create primSpec at path <%s> in edit target '%s'. "
                "Failed to add applied API schema.",
            GetPath().GetText(),
            _GetStage()->GetEditTarget().GetLayer()->GetIdentifier().c_str());
        return false;
    }

    auto _HasItem = [](const TfTokenVector &items, const TfToken &item) {
        return std::find(items.begin(), items.end(), item) != items.end();
    };

    SdfTokenListOp listOp =
        primSpec->GetInfo(UsdTokens->apiSchemas).Get<SdfTokenListOp>();

    if (listOp.IsExplicit()) {
        // If the list op is explicit we check if the explicit item to see if
        // our name is already in it. We'll add it to the end of the explicit
        // list if it is not.
        const TfTokenVector &items = listOp.GetExplicitItems();
        if (_HasItem(items, appliedSchemaName)) {
            return true;
        }
        // Use ReplaceOperations to append in place.
        if (!listOp.ReplaceOperations(SdfListOpTypeExplicit, 
                items.size(), 0, {appliedSchemaName})) {
            return false;
        }
    } else {
        // Otherwise our name could be in the append or prepend list (we 
        // purposefully ignore the "add" list which is deprecated) so we check 
        // both before adding it to the end of prepends.
        const TfTokenVector &preItems = listOp.GetPrependedItems();
        const TfTokenVector &appItems = listOp.GetAppendedItems();
        if (_HasItem(preItems, appliedSchemaName) || 
            _HasItem(appItems, appliedSchemaName)) {
            return true;
        }
        // Use ReplaceOperations to append in place.
        if (!listOp.ReplaceOperations(SdfListOpTypePrepended, 
                preItems.size(), 0, {appliedSchemaName})) {
            return false;
        }
    }

    // If we got here, we edited the list op, so author it back to the spec.
    primSpec->SetInfo(UsdTokens->apiSchemas, VtValue::Take(listOp));
    return true;
}

bool 
UsdPrim::RemoveAppliedSchema(const TfToken &appliedSchemaName) const
{
    // This should create the primSpec in the current edit target.
    // It will also issue an error if it's unable to.
    SdfPrimSpecHandle primSpec = _GetStage()->_CreatePrimSpecForEditing(*this);

    // _CreatePrimSpecForEditing would have already issued a runtime error
    // in case of a failure.
    if (!primSpec) {
        TF_WARN("Unable to create primSpec at path <%s> in edit target '%s'. "
                "Failed to remove applied API schema.",
            GetPath().GetText(),
            _GetStage()->GetEditTarget().GetLayer()->GetIdentifier().c_str());
        return false;
    }

    SdfTokenListOp listOp =
        primSpec->GetInfo(UsdTokens->apiSchemas).Get<SdfTokenListOp>();

    // Create a list op that deletes our schema name and apply it to the current
    // apiSchemas list op for the edit prim spec. This will take care of making
    // sure it ends up in the deletes list (for non-explicit list ops) and is
    // removed from any other items list that would add it back.
    SdfTokenListOp editListOp;
    editListOp.SetDeletedItems({appliedSchemaName});
    if (auto result = editListOp.ApplyOperations(listOp)) {
        primSpec->SetInfo(UsdTokens->apiSchemas, VtValue(*result));
        return true;
    } else {
        TF_CODING_ERROR("Failed to apply list op edits to 'apiSchemas' on spec "
                        "at path <%s> in layer '%s'", 
                        primSpec->GetLayer()->GetIdentifier().c_str(), 
                        primSpec->GetPath().GetText());
        return false;
    }
}

std::vector<UsdProperty>
UsdPrim::_MakeProperties(const TfTokenVector &names) const
{
    std::vector<UsdProperty> props;
    UsdStage *stage = _GetStage();
    props.reserve(names.size());
    for (auto const &propName : names) {
        SdfSpecType specType =
            stage->_GetDefiningSpecType(get_pointer(_Prim()), propName);
        if (specType == SdfSpecTypeAttribute) {
            props.push_back(GetAttribute(propName));
        } else if (TF_VERIFY(specType == SdfSpecTypeRelationship)) {
            props.push_back(GetRelationship(propName));
        }
    }
    return props;
}

// Change the order of items in 'names' so that all the things in 'order' that
// are also in 'names' are at the beginning in the order that they appear in
// 'order', followed by any remaining items in 'names' in their existing order.
static void
_ApplyOrdering(const TfTokenVector &order, TfTokenVector *names)
{
    // If order is empty or names is empty, nothing to do.
    if (order.empty() || names->empty())
        return;

    // Perf note: this walks 'order' and linear searches 'names' to find each
    // element, for O(M*N) operations, where M and N are the lengths of 'order'
    // and 'names'.  We hope 1) that propertyOrder stmts are relatively rare and
    // 2) that property lists are relatively short.  If those assumptions fail,
    // this may need revisiting.  In some quick microbenchmarking, this linear
    // search seems to outperform binary search up to about 5000 names.  We
    // suspect this is because linear search does TfToken pointer comparisons,
    // while binary search has to dereference and do string comparisons.

    typedef TfTokenVector::iterator Iter;

    Iter namesRest = names->begin(), namesEnd = names->end();
    for (const TfToken &oName: order) {
        // Look for this name from 'order' in the rest of 'names'.
        Iter i = std::find(namesRest, namesEnd, oName);
        if (i != namesEnd) {
            // Found.  Move to the front by rotating the sub-range.  Using
            // std::rotate invokes swap(), which avoids TfToken refcounting.
            // Also advance 'namesRest' to the next element.
            std::rotate(namesRest++, i, i+1);
        }
    }
}

bool 
UsdPrim::RemoveProperty(const TfToken &propName) 
{
    SdfPath propPath = GetPath().AppendProperty(propName);
    return _GetStage()->_RemoveProperty(propPath);
}

UsdProperty
UsdPrim::GetProperty(const TfToken &propName) const
{
    SdfSpecType specType =
        _GetStage()->_GetDefiningSpecType(get_pointer(_Prim()), propName);
    if (specType == SdfSpecTypeAttribute) {
        return GetAttribute(propName);
    }
    else if (specType == SdfSpecTypeRelationship) {
        return GetRelationship(propName);
    }
    return UsdProperty(UsdTypeProperty, _Prim(), _ProxyPrimPath(), propName);
}

bool
UsdPrim::HasProperty(const TfToken &propName) const 
{
    return static_cast<bool>(GetProperty(propName));
}

TfTokenVector
UsdPrim::GetPropertyOrder() const
{
    TfTokenVector order;
    GetMetadata(SdfFieldKeys->PropertyOrder, &order);
    return order;
}

using TokenRobinSet = pxr_tsl::robin_set<TfToken, TfToken::HashFunctor>;

// This function was copied from Pcp/{PrimIndex,ComposeSite} and optimized for
// Usd.
static void
_ComposePrimPropertyNames( 
    const PcpPrimIndex& primIndex,
    const UsdPrim::PropertyPredicateFunc &predicate,
    TokenRobinSet *inOutNames)
{
    
    auto const &nodeRange = primIndex.GetNodeRange();
    const bool hasPredicate = static_cast<bool>(predicate);

    for (PcpNodeRef node: nodeRange) {
        if (node.IsCulled() || !node.CanContributeSpecs()) {
            continue;
        }
        for (auto const &layer : node.GetLayerStack()->GetLayers()) {
            VtValue namesVal;
            if (layer->HasField(node.GetPath(),
                                SdfChildrenKeys->PropertyChildren,
                                &namesVal) &&
                namesVal.IsHolding<TfTokenVector>()) {
                // If we have a predicate, then check to see if the name is
                // already included to avoid redundantly invoking it.  The most
                // common case for us is repeated names already present.
                TfTokenVector
                    localNames = namesVal.UncheckedRemove<TfTokenVector>();
                if (hasPredicate) {
                    for (auto &name: localNames) {
                        if (!inOutNames->count(name) && predicate(name)) {
                            inOutNames->insert(std::move(name));
                        }
                    }
                } else {
                    inOutNames->insert(
                        std::make_move_iterator(localNames.begin()),
                        std::make_move_iterator(localNames.end()));
                }
            }
        }
    }
}

TfTokenVector
UsdPrim::_GetPropertyNames(
    bool onlyAuthored, 
    bool applyOrder,
    const UsdPrim::PropertyPredicateFunc &predicate) const
{
    TRACE_FUNCTION();
    
    TokenRobinSet names;
    TfTokenVector namesVec;

    // If we're including unauthored properties, take names from definition, if
    // present.
    const UsdPrimDefinition &primDef = _Prim()->GetPrimDefinition();
    if (!onlyAuthored) {   
        const TfTokenVector &builtInNames = primDef.GetPropertyNames();
        for (const auto &builtInName : builtInNames) {
            if (!predicate || predicate(builtInName)) {
                names.insert(builtInName);
            }
        }
    }

    // Add authored names, then sort and apply ordering.
    _ComposePrimPropertyNames(GetPrimIndex(), predicate, &names);

    if (!names.empty()) {
        // Sort and uniquify the names.
        namesVec.resize(names.size());
        std::copy(names.begin(), names.end(), namesVec.begin());
        sort(namesVec.begin(), namesVec.end(), TfDictionaryLessThan());
        if (applyOrder) {
            _ApplyOrdering(GetPropertyOrder(), &namesVec);
        }
    }

    return namesVec;
}

TfTokenVector
UsdPrim::GetAppliedSchemas() const
{
    return GetPrimDefinition().GetAppliedAPISchemas();
}

TfTokenVector
UsdPrim::GetPropertyNames(
    const UsdPrim::PropertyPredicateFunc &predicate) const
{
    return _GetPropertyNames(/*onlyAuthored=*/ false, 
                             /*applyOrder*/ true, 
                             predicate);
}

TfTokenVector
UsdPrim::GetAuthoredPropertyNames(
    const UsdPrim::PropertyPredicateFunc &predicate) const
{
    return _GetPropertyNames(/*onlyAuthored=*/ true, 
                             /*applyOrder*/ true, 
                             predicate);
}

std::vector<UsdProperty>
UsdPrim::GetProperties(const UsdPrim::PropertyPredicateFunc &predicate) const
{
    return _MakeProperties(GetPropertyNames(predicate));
}

std::vector<UsdProperty>
UsdPrim::GetAuthoredProperties(
    const UsdPrim::PropertyPredicateFunc &predicate) const
{
    return _MakeProperties(GetAuthoredPropertyNames(predicate));
}

std::vector<UsdProperty>
UsdPrim::_GetPropertiesInNamespace(
    const std::string &namespaces,
    bool onlyAuthored) const
{
    if (namespaces.empty())
        return onlyAuthored ? GetAuthoredProperties() : GetProperties();

    const char delim = UsdObject::GetNamespaceDelimiter();

    // Set terminator to the expected position of the delimiter after all the
    // supplied namespaces.  We perform an explicit test for this char below
    // so that we don't need to allocate a new string if namespaces does not
    // already end with the delimiter
    const size_t terminator = namespaces.size() -
        (*namespaces.rbegin() == delim);

    TfTokenVector names = _GetPropertyNames(onlyAuthored, 
        /*applyOrder=*/ true,
        /*predicate*/ [&namespaces, terminator, delim](const TfToken &name) {
            const std::string &s = name.GetString();
            return s.size() > terminator &&
                   TfStringStartsWith(s, namespaces)   && 
                   s[terminator] == delim;
        });

    std::vector<UsdProperty> properties(_MakeProperties(names));
    WorkSwapDestroyAsync(names);
    return properties;
}

std::vector<UsdProperty>
UsdPrim::GetPropertiesInNamespace(const std::string &namespaces) const
{
    return _GetPropertiesInNamespace(namespaces, /*onlyAuthored=*/false);
}

std::vector<UsdProperty>
UsdPrim::GetAuthoredPropertiesInNamespace(const std::string &namespaces) const
{
    return _GetPropertiesInNamespace(namespaces, /*onlyAuthored=*/true);
}

std::vector<UsdProperty>
UsdPrim::GetPropertiesInNamespace(const std::vector<std::string> &namespaces) const
{
    return GetPropertiesInNamespace(SdfPath::JoinIdentifier(namespaces));
}

std::vector<UsdProperty>
UsdPrim::GetAuthoredPropertiesInNamespace(
    const std::vector<std::string> &namespaces) const
{
    return GetAuthoredPropertiesInNamespace(
        SdfPath::JoinIdentifier(namespaces));
}

UsdAttribute
UsdPrim::CreateAttribute(const TfToken& name,
                         const SdfValueTypeName &typeName,
                         bool custom,
                         SdfVariability variability) const
{
    UsdAttribute attr = GetAttribute(name);
    attr._Create(typeName, custom, variability);
    return attr;
}

UsdAttribute
UsdPrim::CreateAttribute(const TfToken& name,
                         const SdfValueTypeName &typeName,
                         SdfVariability variability) const
{
    return CreateAttribute(name, typeName, /*custom=*/true, variability);
}

UsdAttribute
UsdPrim::CreateAttribute(const std::vector<std::string> &nameElts,
                         const SdfValueTypeName &typeName,
                         bool custom,
                         SdfVariability variability) const
{
    return CreateAttribute(TfToken(SdfPath::JoinIdentifier(nameElts)),
                           typeName, custom, variability);
}

UsdAttribute
UsdPrim::CreateAttribute(const std::vector<std::string> &nameElts,
                         const SdfValueTypeName &typeName,
                         SdfVariability variability) const
{
    return CreateAttribute(nameElts, typeName, /*custom=*/true, variability);
}

UsdAttributeVector
UsdPrim::_GetAttributes(bool onlyAuthored, bool applyOrder) const
{
    const TfTokenVector names = _GetPropertyNames(onlyAuthored, applyOrder);
    UsdAttributeVector attrs;

    // PERFORMANCE: This is sloppy, since property names are a superset of
    // attribute names, however this vector is likely short lived and worth the
    // trade off of repeated reallocation.
    attrs.reserve(names.size());
    for (const auto& propName : names) {
        if (UsdAttribute attr = GetAttribute(propName)) {
            attrs.push_back(attr);
        }
    }

    return attrs;
}

UsdAttributeVector
UsdPrim::GetAttributes() const
{
    return _GetAttributes(/*onlyAuthored=*/false, /*applyOrder=*/true);
}

UsdAttributeVector
UsdPrim::GetAuthoredAttributes() const
{
    return _GetAttributes(/*onlyAuthored=*/true, /*applyOrder=*/true);
}

UsdAttribute
UsdPrim::GetAttribute(const TfToken& attrName) const
{
    // An invalid prim will present a coding error, and then return an
    // invalid attribute
    return UsdAttribute(_Prim(), _ProxyPrimPath(), attrName);
}

bool
UsdPrim::HasAttribute(const TfToken& attrName) const
{
    return static_cast<bool>(GetAttribute(attrName));
}

UsdRelationship
UsdPrim::CreateRelationship(const TfToken& name, bool custom) const
{
    UsdRelationship rel = GetRelationship(name);
    rel._Create(custom);
    return rel;
}

UsdRelationship
UsdPrim::CreateRelationship(const std::vector<std::string> &nameElts, 
                            bool custom) const
{
    return CreateRelationship(TfToken(SdfPath::JoinIdentifier(nameElts)),
                              custom);
}

UsdRelationshipVector
UsdPrim::_GetRelationships(bool onlyAuthored, bool applyOrder) const
{
    const TfTokenVector names = _GetPropertyNames(onlyAuthored, applyOrder);
    UsdRelationshipVector rels;

    // PERFORMANCE: This is sloppy, since property names are a superset of
    // relationship names, however this vector is likely short lived and worth
    // the trade off of repeated reallocation.
    rels.reserve(names.size());
    for (const auto& propName : names) {
        if (UsdRelationship rel = GetRelationship(propName)) {
            rels.push_back(rel);
        }
    }

    return rels;
}

UsdRelationshipVector
UsdPrim::GetRelationships() const
{
    return _GetRelationships(/*onlyAuthored=*/false, /*applyOrder=*/true);
}

UsdRelationshipVector
UsdPrim::GetAuthoredRelationships() const
{
    return _GetRelationships(/*onlyAuthored=*/true, /*applyOrder=*/true);
}

UsdRelationship
UsdPrim::GetRelationship(const TfToken& relName) const
{
    return UsdRelationship(_Prim(), _ProxyPrimPath(), relName);
}

bool
UsdPrim::HasRelationship(const TfToken& relName) const
{
    return static_cast<bool>(GetRelationship(relName));
} 

template <class PropertyType, class Derived>
struct UsdPrim_TargetFinder
{
    using Predicate = std::function<bool (PropertyType const &)>;
    
    static SdfPathVector
    Find(UsdPrim const &prim, Predicate const &pred, bool recurse) {
        UsdPrim_TargetFinder tf(prim, pred, recurse);
        tf._Find();
        return std::move(tf._result);
    }

private:
    explicit UsdPrim_TargetFinder(
        UsdPrim const &prim, Predicate const &pred, bool recurse)
        : _prim(prim)
        , _consumerTask(_dispatcher, [this]() { _ConsumerTask(); })
        , _predicate(pred)
        , _recurse(recurse) {}

    void _Visit(UsdRelationship const &rel) {
        SdfPathVector targets;
        rel._GetForwardedTargets(&targets,
                                 /*includeForwardingRels=*/true);
        _VisitImpl(targets);
    }
    
    void _Visit(UsdAttribute const &attr) {
        SdfPathVector sources;
        attr.GetConnections(&sources);
        _VisitImpl(sources);
    }
    
    void _VisitImpl(SdfPathVector const &paths) {
        if (!paths.empty()) {
            for (SdfPath const &p: paths) {
                _workQueue.push(p);
            }
            _consumerTask.Wake();
        }
        
        if (_recurse) {
            WorkParallelForEach(
                paths.begin(), paths.end(),
                [this](SdfPath const &path) {
                    if (!path.HasPrefix(_prim.GetPath())) {
                        if (UsdPrim owningPrim = _prim.GetStage()->
                            GetPrimAtPath(path.GetPrimPath())) {
                            _VisitSubtree(owningPrim);
                        }
                    }
                });
        }
    }    

    void _VisitPrim(UsdPrim const &prim) {
        if (_seenPrims.insert(prim).second) {
            auto props = static_cast<Derived *>(this)->_GetProperties(prim);
            for (auto const &prop: props) {
                if (!_predicate || _predicate(prop)) {
                    _dispatcher.Run([this, prop]() { _Visit(prop); });
                }
            }
        }
    };

    void _VisitSubtree(UsdPrim const &prim) {
        _VisitPrim(prim);
        auto range = prim.GetDescendants();
        WorkParallelForEach(range.begin(), range.end(),
                            [this](UsdPrim const &desc) { _VisitPrim(desc); });
    }

    void _Find() {
        TF_PY_ALLOW_THREADS_IN_SCOPE();

        WorkWithScopedParallelism([this]() {
                _VisitSubtree(_prim);
                _dispatcher.Wait();
                tbb::parallel_sort(_result.begin(), _result.end(),
                                   SdfPath::FastLessThan());
            });

        _result.erase(unique(_result.begin(), _result.end()), _result.end());
    }

    void _ConsumerTask() {
        SdfPath path;
        while (_workQueue.try_pop(path)) {
            _result.push_back(path);
        }
    }

    UsdPrim _prim;
    WorkDispatcher _dispatcher;
    WorkSingularTask _consumerTask;
    Predicate const &_predicate;
    tbb::concurrent_queue<SdfPath> _workQueue;
    tbb::concurrent_unordered_set<UsdPrim, boost::hash<UsdPrim> > _seenPrims;
    SdfPathVector _result;
    bool _recurse;
};
          
struct UsdPrim_RelTargetFinder
    : public UsdPrim_TargetFinder<UsdRelationship, UsdPrim_RelTargetFinder>
{
    std::vector<UsdRelationship> _GetProperties(UsdPrim const &prim) const {
        return prim._GetRelationships(/*onlyAuthored=*/true,
                                      /*applyOrder=*/false);
    }
};

struct UsdPrim_AttrConnectionFinder
    : public UsdPrim_TargetFinder<UsdAttribute, UsdPrim_AttrConnectionFinder>
{
    std::vector<UsdAttribute> _GetProperties(UsdPrim const &prim) const {
        return prim._GetAttributes(/*onlyAuthored=*/true,
                                   /*applyOrder=*/false);
    }
};

USD_API
SdfPathVector
UsdPrim::FindAllAttributeConnectionPaths(
    std::function<bool (UsdAttribute const &)> const &predicate,
    bool recurseOnSources) const
{
    return UsdPrim_AttrConnectionFinder
        ::Find(*this, predicate, recurseOnSources);
}
    
SdfPathVector
UsdPrim::FindAllRelationshipTargetPaths(
    std::function<bool (UsdRelationship const &)> const &predicate,
    bool recurseOnTargets) const
{
    return UsdPrim_RelTargetFinder::Find(*this, predicate, recurseOnTargets);
}

bool
UsdPrim::HasVariantSets() const
{
    // Variant sets can't be defined in schema fallbacks as of yet so we only
    // need to check for authored variant sets.
    return HasAuthoredMetadata(SdfFieldKeys->VariantSetNames);
}

UsdVariantSets
UsdPrim::GetVariantSets() const
{
    return UsdVariantSets(*this);
}

UsdVariantSet
UsdPrim::GetVariantSet(const std::string& variantSetName) const
{
    return UsdVariantSet(*this, variantSetName);
}


UsdInherits
UsdPrim::GetInherits() const
{
    return UsdInherits(*this);
}

bool
UsdPrim::HasAuthoredInherits() const
{
    return HasAuthoredMetadata(SdfFieldKeys->InheritPaths);
}

UsdSpecializes
UsdPrim::GetSpecializes() const
{
    return UsdSpecializes(*this);
}

bool
UsdPrim::HasAuthoredSpecializes() const
{
    return HasAuthoredMetadata(SdfFieldKeys->Specializes);
}

UsdReferences
UsdPrim::GetReferences() const
{
    return UsdReferences(*this);
}

bool
UsdPrim::HasAuthoredReferences() const
{
    return HasAuthoredMetadata(SdfFieldKeys->References);
}

// --------------------------------------------------------------------- //
/// \name Payloads, Load and Unload
// --------------------------------------------------------------------- //

bool
UsdPrim::HasPayload() const
{
    return HasAuthoredPayloads();
}

bool
UsdPrim::SetPayload(const SdfPayload& payload) const
{
    UsdPayloads payloads = GetPayloads();
    payloads.ClearPayloads();
    return payloads.SetPayloads(SdfPayloadVector{payload});
}

bool
UsdPrim::SetPayload(const std::string& assetPath, const SdfPath& primPath) const
{
    return SetPayload(SdfPayload(assetPath, primPath));
}

bool
UsdPrim::SetPayload(const SdfLayerHandle& layer, const SdfPath& primPath) const
{
    return SetPayload(SdfPayload(layer->GetIdentifier(), primPath));
}

bool
UsdPrim::ClearPayload() const
{
    return GetPayloads().ClearPayloads();
}

UsdPayloads
UsdPrim::GetPayloads() const
{
    return UsdPayloads(*this);
}

bool
UsdPrim::HasAuthoredPayloads() const
{
    // Unlike the equivalent function for references, we query the prim data
    // for the cached value of HasPayload computed by Pcp instead of querying
    // the composed metadata. This is necessary as this function is called by 
    // _IncludeNewlyDiscoveredPayloadsPredicate in UsdStage which can't safely
    // call back into the querying the composed metatdata.
    return _Prim()->HasPayload();
}

void
UsdPrim::Load(UsdLoadPolicy policy) const
{
    if (IsInPrototype()) {
        TF_CODING_ERROR("Attempted to load a prim in a prototype <%s>",
                        GetPath().GetText());
        return;
    }
    _GetStage()->Load(GetPath(), policy);
}

void
UsdPrim::Unload() const
{
    if (IsInPrototype()) {
        TF_CODING_ERROR("Attempted to unload a prim in a prototype <%s>",
                        GetPath().GetText());
        return;
    }
    _GetStage()->Unload(GetPath());
}

TfTokenVector 
UsdPrim::GetChildrenNames() const
{
    TfTokenVector names;
    for (const auto &child : GetChildren()) {
        names.push_back(child.GetName());
    }
    return names;
}

TfTokenVector 
UsdPrim::GetAllChildrenNames() const
{
    TfTokenVector names;
    for (const auto &child : GetAllChildren()) {
        names.push_back(child.GetName());
    }
    return names;
}

TfTokenVector 
UsdPrim::GetFilteredChildrenNames(const Usd_PrimFlagsPredicate &predicate) const
{
    TfTokenVector names;
    for (const auto &child : GetFilteredChildren(predicate)) {
        names.push_back(child.GetName());
    }
    return names;
}

TfTokenVector
UsdPrim::GetChildrenReorder() const
{
    TfTokenVector reorder;
    GetMetadata(SdfFieldKeys->PrimOrder, &reorder);
    return reorder;
}

UsdPrim
UsdPrim::GetNextSibling() const
{
    return GetFilteredNextSibling(UsdPrimDefaultPredicate);
}

UsdPrim
UsdPrim::GetFilteredNextSibling(const Usd_PrimFlagsPredicate &inPred) const
{
    Usd_PrimDataConstPtr sibling = get_pointer(_Prim());
    SdfPath siblingPath = _ProxyPrimPath();
    const Usd_PrimFlagsPredicate pred = 
        Usd_CreatePredicateForTraversal(sibling, siblingPath, inPred);

    if (Usd_MoveToNextSiblingOrParent(sibling, siblingPath, pred)) {
        return UsdPrim();
    }
    return UsdPrim(sibling, siblingPath);
}

bool
UsdPrim::IsPseudoRoot() const
{
    return GetPath() == SdfPath::AbsoluteRootPath();
}

bool
UsdPrim::IsPrototypePath(const SdfPath& path)
{
    return Usd_InstanceCache::IsPrototypePath(path);
}

bool
UsdPrim::IsPathInPrototype(const SdfPath& path)
{
    return Usd_InstanceCache::IsPathInPrototype(path);
}

UsdPrim
UsdPrim::GetPrototype() const
{
    Usd_PrimDataConstPtr protoPrimData = 
        _GetStage()->_GetPrototypeForInstance(get_pointer(_Prim()));
    return UsdPrim(protoPrimData, SdfPath());
}

std::vector<UsdPrim>
UsdPrim::GetInstances() const
{
    return _GetStage()->_GetInstancesForPrototype(*this);
}

SdfPrimSpecHandleVector 
UsdPrim::GetPrimStack() const
{
    SdfPrimSpecHandleVector primStack;

    for (Usd_Resolver resolver(&(_Prim()->GetPrimIndex())); 
                      resolver.IsValid(); resolver.NextLayer()) {

        auto primSpec = resolver.GetLayer()
            ->GetPrimAtPath(resolver.GetLocalPath());

        if (primSpec) { 
            primStack.push_back(primSpec); 
        }
    }

    return primStack;
}

PcpPrimIndex 
UsdPrim::ComputeExpandedPrimIndex() const
{
    // Get the prim index path to compute from the index stored in the prim
    // data. This ensures we get consistent behavior when dealing with 
    // instancing and instance proxies.
    const PcpPrimIndex& cachedPrimIndex = _Prim()->GetPrimIndex();
    if (!cachedPrimIndex.IsValid()) {
        return PcpPrimIndex();
    }
    
    const SdfPath& primIndexPath = cachedPrimIndex.GetPath();
    PcpCache* cache = _GetStage()->_GetPcpCache();
    
    PcpPrimIndexOutputs outputs;
    PcpComputePrimIndex(
        primIndexPath, cache->GetLayerStack(),
        cache->GetPrimIndexInputs().Cull(false), 
        &outputs);

    _GetStage()->_ReportPcpErrors(
        outputs.allErrors, 
        TfStringPrintf(
            "computing expanded prim index for <%s>", GetPath().GetText()));
    
    return outputs.primIndex;
}

UsdPrim
UsdPrim::GetPrimAtPath(const SdfPath& path) const{
    const SdfPath absolutePath = path.MakeAbsolutePath(GetPath());
    return GetStage()->GetPrimAtPath(absolutePath);
}

UsdObject
UsdPrim::GetObjectAtPath(const SdfPath& path) const{
    const SdfPath absolutePath = path.MakeAbsolutePath(GetPath());
    return GetStage()->GetObjectAtPath(absolutePath);
}

UsdProperty
UsdPrim::GetPropertyAtPath(const SdfPath& path) const{
    return GetObjectAtPath(path).As<UsdProperty>();
}

UsdAttribute
UsdPrim::GetAttributeAtPath(const SdfPath& path) const{
    return GetObjectAtPath(path).As<UsdAttribute>();
}


UsdRelationship
UsdPrim::GetRelationshipAtPath(const SdfPath& path) const{
    return GetObjectAtPath(path).As<UsdRelationship>();
}

PXR_NAMESPACE_CLOSE_SCOPE

