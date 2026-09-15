//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdMedia/authorshipAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdMediaAuthorshipAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdMediaAuthorshipAPI::~UsdMediaAuthorshipAPI()
{
}

/* static */
UsdMediaAuthorshipAPI
UsdMediaAuthorshipAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdMediaAuthorshipAPI();
    }
    TfToken name;
    if (!IsAuthorshipAPIPath(path, &name)) {
        TF_CODING_ERROR("Invalid authorship path <%s>.", path.GetText());
        return UsdMediaAuthorshipAPI();
    }
    return UsdMediaAuthorshipAPI(stage->GetPrimAtPath(path.GetPrimPath()), name);
}

UsdMediaAuthorshipAPI
UsdMediaAuthorshipAPI::Get(const UsdPrim &prim, const TfToken &name)
{
    return UsdMediaAuthorshipAPI(prim, name);
}

/* static */
std::vector<UsdMediaAuthorshipAPI>
UsdMediaAuthorshipAPI::GetAll(const UsdPrim &prim)
{
    std::vector<UsdMediaAuthorshipAPI> schemas;
    
    for (const auto &schemaName :
         UsdAPISchemaBase::_GetMultipleApplyInstanceNames(prim, _GetStaticTfType())) {
        schemas.emplace_back(prim, schemaName);
    }

    return schemas;
}


/* static */
bool 
UsdMediaAuthorshipAPI::IsSchemaPropertyBaseName(const TfToken &baseName)
{
    static TfTokenVector attrsAndRels = {
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdMediaTokens->authorship_MultipleApplyTemplate_SoftwarePackage),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdMediaTokens->authorship_MultipleApplyTemplate_SoftwareVersion),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdMediaTokens->authorship_MultipleApplyTemplate_DigitalSourceType),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdMediaTokens->authorship_MultipleApplyTemplate_Creator),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdMediaTokens->authorship_MultipleApplyTemplate_Description),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdMediaTokens->authorship_MultipleApplyTemplate_InputNames),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdMediaTokens->authorship_MultipleApplyTemplate_InputValues),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdMediaTokens->authorship_MultipleApplyTemplate_Created),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdMediaTokens->authorship_MultipleApplyTemplate_InstanceID),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdMediaTokens->authorship_MultipleApplyTemplate_UsageTerms),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdMediaTokens->authorship_MultipleApplyTemplate_CopyrightOwner),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdMediaTokens->authorship_MultipleApplyTemplate_Contact),
    };

    return find(attrsAndRels.begin(), attrsAndRels.end(), baseName)
            != attrsAndRels.end();
}

/* static */
bool
UsdMediaAuthorshipAPI::IsAuthorshipAPIPath(
    const SdfPath &path, TfToken *name)
{
    if (!path.IsPropertyPath()) {
        return false;
    }

    std::string propertyName = path.GetName();
    TfTokenVector tokens = SdfPath::TokenizeIdentifierAsTokens(propertyName);

    // The baseName of the  path can't be one of the 
    // schema properties. We should validate this in the creation (or apply)
    // API.
    TfToken baseName = *tokens.rbegin();
    if (IsSchemaPropertyBaseName(baseName)) {
        return false;
    }

    if (tokens.size() >= 2
        && tokens[0] == UsdMediaTokens->authorship) {
        *name = TfToken(propertyName.substr(
           UsdMediaTokens->authorship.GetString().size() + 1));
        return true;
    }

    return false;
}

/* virtual */
UsdSchemaKind UsdMediaAuthorshipAPI::_GetSchemaKind() const
{
    return UsdMediaAuthorshipAPI::schemaKind;
}

/* static */
bool
UsdMediaAuthorshipAPI::CanApply(
    const UsdPrim &prim, const TfToken &name, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdMediaAuthorshipAPI>(name, whyNot);
}

/* static */
UsdMediaAuthorshipAPI
UsdMediaAuthorshipAPI::Apply(const UsdPrim &prim, const TfToken &name)
{
    if (prim.ApplyAPI<UsdMediaAuthorshipAPI>(name)) {
        return UsdMediaAuthorshipAPI(prim, name);
    }
    return UsdMediaAuthorshipAPI();
}

/* static */
const TfType &
UsdMediaAuthorshipAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdMediaAuthorshipAPI>();
    return tfType;
}

/* static */
bool 
UsdMediaAuthorshipAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdMediaAuthorshipAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/// Returns the property name prefixed with the correct namespace prefix, which
/// is composed of the the API's propertyNamespacePrefix metadata and the
/// instance name of the API.
static inline
TfToken
_GetNamespacedPropertyName(const TfToken instanceName, const TfToken propName)
{
    return UsdSchemaRegistry::MakeMultipleApplyNameInstance(propName, instanceName);
}

UsdAttribute
UsdMediaAuthorshipAPI::GetSoftwarePackageAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdMediaTokens->authorship_MultipleApplyTemplate_SoftwarePackage));
}

UsdAttribute
UsdMediaAuthorshipAPI::CreateSoftwarePackageAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdMediaTokens->authorship_MultipleApplyTemplate_SoftwarePackage),
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdMediaAuthorshipAPI::GetSoftwareVersionAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdMediaTokens->authorship_MultipleApplyTemplate_SoftwareVersion));
}

UsdAttribute
UsdMediaAuthorshipAPI::CreateSoftwareVersionAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdMediaTokens->authorship_MultipleApplyTemplate_SoftwareVersion),
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdMediaAuthorshipAPI::GetDigitalSourceTypeAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdMediaTokens->authorship_MultipleApplyTemplate_DigitalSourceType));
}

UsdAttribute
UsdMediaAuthorshipAPI::CreateDigitalSourceTypeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdMediaTokens->authorship_MultipleApplyTemplate_DigitalSourceType),
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdMediaAuthorshipAPI::GetCreatorAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdMediaTokens->authorship_MultipleApplyTemplate_Creator));
}

UsdAttribute
UsdMediaAuthorshipAPI::CreateCreatorAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdMediaTokens->authorship_MultipleApplyTemplate_Creator),
                       SdfValueTypeNames->StringArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdMediaAuthorshipAPI::GetDescriptionAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdMediaTokens->authorship_MultipleApplyTemplate_Description));
}

UsdAttribute
UsdMediaAuthorshipAPI::CreateDescriptionAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdMediaTokens->authorship_MultipleApplyTemplate_Description),
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdMediaAuthorshipAPI::GetInputNamesAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdMediaTokens->authorship_MultipleApplyTemplate_InputNames));
}

UsdAttribute
UsdMediaAuthorshipAPI::CreateInputNamesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdMediaTokens->authorship_MultipleApplyTemplate_InputNames),
                       SdfValueTypeNames->StringArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdMediaAuthorshipAPI::GetInputValuesAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdMediaTokens->authorship_MultipleApplyTemplate_InputValues));
}

UsdAttribute
UsdMediaAuthorshipAPI::CreateInputValuesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdMediaTokens->authorship_MultipleApplyTemplate_InputValues),
                       SdfValueTypeNames->StringArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdMediaAuthorshipAPI::GetCreatedAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdMediaTokens->authorship_MultipleApplyTemplate_Created));
}

UsdAttribute
UsdMediaAuthorshipAPI::CreateCreatedAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdMediaTokens->authorship_MultipleApplyTemplate_Created),
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdMediaAuthorshipAPI::GetInstanceIDAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdMediaTokens->authorship_MultipleApplyTemplate_InstanceID));
}

UsdAttribute
UsdMediaAuthorshipAPI::CreateInstanceIDAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdMediaTokens->authorship_MultipleApplyTemplate_InstanceID),
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdMediaAuthorshipAPI::GetUsageTermsAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdMediaTokens->authorship_MultipleApplyTemplate_UsageTerms));
}

UsdAttribute
UsdMediaAuthorshipAPI::CreateUsageTermsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdMediaTokens->authorship_MultipleApplyTemplate_UsageTerms),
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdMediaAuthorshipAPI::GetCopyrightOwnerAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdMediaTokens->authorship_MultipleApplyTemplate_CopyrightOwner));
}

UsdAttribute
UsdMediaAuthorshipAPI::CreateCopyrightOwnerAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdMediaTokens->authorship_MultipleApplyTemplate_CopyrightOwner),
                       SdfValueTypeNames->StringArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdMediaAuthorshipAPI::GetContactAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdMediaTokens->authorship_MultipleApplyTemplate_Contact));
}

UsdAttribute
UsdMediaAuthorshipAPI::CreateContactAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdMediaTokens->authorship_MultipleApplyTemplate_Contact),
                       SdfValueTypeNames->StringArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

namespace {
static inline TfTokenVector
_ConcatenateAttributeNames(const TfTokenVector& left,const TfTokenVector& right)
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());
    result.insert(result.end(), right.begin(), right.end());
    return result;
}
}

/*static*/
const TfTokenVector&
UsdMediaAuthorshipAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdMediaTokens->authorship_MultipleApplyTemplate_SoftwarePackage,
        UsdMediaTokens->authorship_MultipleApplyTemplate_SoftwareVersion,
        UsdMediaTokens->authorship_MultipleApplyTemplate_DigitalSourceType,
        UsdMediaTokens->authorship_MultipleApplyTemplate_Creator,
        UsdMediaTokens->authorship_MultipleApplyTemplate_Description,
        UsdMediaTokens->authorship_MultipleApplyTemplate_InputNames,
        UsdMediaTokens->authorship_MultipleApplyTemplate_InputValues,
        UsdMediaTokens->authorship_MultipleApplyTemplate_Created,
        UsdMediaTokens->authorship_MultipleApplyTemplate_InstanceID,
        UsdMediaTokens->authorship_MultipleApplyTemplate_UsageTerms,
        UsdMediaTokens->authorship_MultipleApplyTemplate_CopyrightOwner,
        UsdMediaTokens->authorship_MultipleApplyTemplate_Contact,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdAPISchemaBase::GetSchemaAttributeNames(true),
            localNames);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

/*static*/
TfTokenVector
UsdMediaAuthorshipAPI::GetSchemaAttributeNames(
    bool includeInherited, const TfToken &instanceName)
{
    const TfTokenVector &attrNames = GetSchemaAttributeNames(includeInherited);
    if (instanceName.IsEmpty()) {
        return attrNames;
    }
    TfTokenVector result;
    result.reserve(attrNames.size());
    for (const TfToken &attrName : attrNames) {
        result.push_back(
            UsdSchemaRegistry::MakeMultipleApplyNameInstance(attrName, instanceName));
    }
    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/tokens.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"

#include <algorithm>
#include <set>

PXR_NAMESPACE_OPEN_SCOPE

// The identifier as it appears in apiSchemas, ahead of ":instanceName".
static const TfToken &
_GetSchemaIdentifier()
{
    static const TfToken identifier =
        UsdSchemaRegistry::GetSchemaTypeName(
            TfType::Find<UsdMediaAuthorshipAPI>());
    return identifier;
}

// Instance name from e.g. "AuthorshipAPI:hunyuan3d", or empty if not this
// schema.
static TfToken
_GetInstanceNameFromAPISchemaName(const TfToken &apiSchemaName)
{
    const std::pair<TfToken, TfToken> typeNameAndInstance =
        UsdSchemaRegistry::GetTypeNameAndInstance(apiSchemaName);
    if (typeNameAndInstance.first != _GetSchemaIdentifier()) {
        return TfToken();
    }
    return typeNameAndInstance.second;
}

// Appends the instance name of every record \p spec applies. Only scans
// apiSchemas, since a bare property name can't be split back into
// instance and base name reliably (both can be namespaced).
static void
_AppendInstanceNamesInSpec(const SdfPrimSpecHandle &spec,
                           std::vector<TfToken> *instanceNames)
{
    if (!spec->HasInfo(UsdTokens->apiSchemas)) {
        return;
    }
    const VtValue value = spec->GetInfo(UsdTokens->apiSchemas);
    if (!value.IsHolding<SdfTokenListOp>()) {
        return;
    }

    // A token could appear in more than one list-op field, so dedup with a
    // set.
    std::set<TfToken> found;
    const SdfTokenListOp &listOp = value.UncheckedGet<SdfTokenListOp>();

    // Scan every op, not just the applied result: shadowed opinions are
    // exactly what this search is for.
    for (const SdfListOpType listOpType :
            {SdfListOpTypeExplicit, SdfListOpTypeAdded,
             SdfListOpTypeOrdered, SdfListOpTypePrepended,
             SdfListOpTypeAppended}) {
        for (const TfToken &apiSchemaName : listOp.GetItems(listOpType)) {
            const TfToken instanceName =
                _GetInstanceNameFromAPISchemaName(apiSchemaName);
            if (!instanceName.IsEmpty()) {
                found.insert(instanceName);
            }
        }
    }

    instanceNames->insert(instanceNames->end(), found.begin(), found.end());
}

static void
_AppendComposedRecordsForPrim(const UsdPrim &prim,
                              std::vector<UsdMediaAuthorshipAPI> *result)
{
    for (const UsdMediaAuthorshipAPI &record :
            UsdMediaAuthorshipAPI::GetAll(prim)) {
        result->push_back(record);
    }
}

static void
_AppendPrimStackRecordsForPrim(const UsdPrim &prim,
                               std::vector<UsdMediaAuthorshipAPI> *result)
{
    // Strongest to weakest, not deduplicated: a record in several layers
    // is reported once per layer, in that order.
    for (const SdfPrimSpecHandle &spec : prim.GetPrimStack()) {
        if (!spec) {
            continue;
        }
        std::vector<TfToken> instanceNames;
        _AppendInstanceNamesInSpec(spec, &instanceNames);
        for (const TfToken &instanceName : instanceNames) {
            result->emplace_back(prim, instanceName);
        }
    }
}

// SdfPath orders a prim before its descendants, so sorting by path also
// sorts ancestors first.
static bool
_RecordLess(const UsdMediaAuthorshipAPI &lhs, const UsdMediaAuthorshipAPI &rhs)
{
    const SdfPath lhsPath = lhs.GetPrim().GetPath();
    const SdfPath rhsPath = rhs.GetPrim().GetPath();
    if (lhsPath != rhsPath) {
        return lhsPath < rhsPath;
    }
    return lhs.GetName() < rhs.GetName();
}

// Shared by GetAllOnStage() and GetAllInPrimStacks(); they differ only in
// how they gather records per prim.
static std::vector<UsdMediaAuthorshipAPI>
_GetAllOnStageImpl(
    const UsdStagePtr &stage,
    void (*appendForPrim)(const UsdPrim &,
                          std::vector<UsdMediaAuthorshipAPI> *))
{
    std::vector<UsdMediaAuthorshipAPI> result;

    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return result;
    }

    // TraverseAll() includes inactive and abstract prims.
    for (const UsdPrim &prim : stage->TraverseAll()) {
        appendForPrim(prim, &result);
    }

    // Prims beneath a native instance are not visited above, so walk the
    // prototypes to reach records authored inside them.
    for (const UsdPrim &prototype : stage->GetPrototypes()) {
        for (const UsdPrim &prim :
                UsdPrimRange(prototype, UsdPrimAllPrimsPredicate)) {
            appendForPrim(prim, &result);
        }
    }

    // Stable, so that repeated records stay in strongest-to-weakest order.
    std::stable_sort(result.begin(), result.end(), _RecordLess);

    return result;
}

/* static */
std::vector<UsdMediaAuthorshipAPI>
UsdMediaAuthorshipAPI::GetAllOnStage(const UsdStagePtr &stage)
{
    return _GetAllOnStageImpl(stage, _AppendComposedRecordsForPrim);
}

/* static */
std::vector<UsdMediaAuthorshipAPI>
UsdMediaAuthorshipAPI::GetAllInPrimStacks(const UsdStagePtr &stage)
{
    return _GetAllOnStageImpl(stage, _AppendPrimStackRecordsForPrim);
}

/* static */
std::vector<UsdMediaAuthorshipAPI>
UsdMediaAuthorshipAPI::ComputeAccumulatedRecords(const UsdPrim &prim)
{
    std::vector<UsdMediaAuthorshipAPI> result;

    if (!prim) {
        TF_CODING_ERROR("Invalid prim");
        return result;
    }

    for (UsdPrim p = prim; p && !p.IsPseudoRoot(); p = p.GetParent()) {
        _AppendComposedRecordsForPrim(p, &result);
    }

    std::stable_sort(result.begin(), result.end(), _RecordLess);

    return result;
}

/* static */
std::vector<UsdMediaAuthorshipAPI>
UsdMediaAuthorshipAPI::GetAllUnder(const UsdPrim &prim)
{
    std::vector<UsdMediaAuthorshipAPI> result;

    if (!prim) {
        TF_CODING_ERROR("Invalid prim");
        return result;
    }

    for (const UsdPrim &p : UsdPrimRange(prim, UsdPrimAllPrimsPredicate)) {
        _AppendComposedRecordsForPrim(p, &result);
    }

    std::stable_sort(result.begin(), result.end(), _RecordLess);

    return result;
}

/* static */
SdfPathVector
UsdMediaAuthorshipAPI::GetAllInLayer(const SdfLayerHandle &layer)
{
    SdfPathVector result;

    if (!layer) {
        TF_CODING_ERROR("Invalid layer");
        return result;
    }

    layer->Traverse(SdfPath::AbsoluteRootPath(),
                    [&layer, &result](const SdfPath &path) {
        const SdfPrimSpecHandle spec = layer->GetPrimAtPath(path);
        if (!spec) {
            return;
        }
        std::vector<TfToken> instanceNames;
        _AppendInstanceNamesInSpec(spec, &instanceNames);
        for (const TfToken &instanceName : instanceNames) {
            result.push_back(path.AppendProperty(TfToken(
                SdfPath::JoinIdentifier(
                    UsdMediaTokens->authorship, instanceName))));
        }
    });

    std::sort(result.begin(), result.end());

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
