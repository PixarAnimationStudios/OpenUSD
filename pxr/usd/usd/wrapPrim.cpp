//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/payloads.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/references.h"
#include "pxr/usd/usd/resolveTarget.h"
#include "pxr/usd/usd/inherits.h"
#include "pxr/usd/usd/specializes.h"
#include "pxr/usd/usd/variantSets.h"
#include "pxr/usd/usd/wrapUtils.h"

#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/sdf/payload.h"
#include "pxr/usd/sdf/primSpec.h"

#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <string>
#include <vector>

using std::string;
using std::vector;

namespace nb = nanobind;

PXR_NAMESPACE_OPEN_SCOPE

const PcpPrimIndex &
Usd_PrimGetSourcePrimIndex(const UsdPrim& prim)
{
    return prim._GetSourcePrimIndex();
}

PXR_NAMESPACE_CLOSE_SCOPE

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static SdfPathVector
_FindAllAttributeConnectionPaths(
    UsdPrim const &self,
    Usd_PrimFlagsPredicate const &traversal,
    nb::object pypred,
    bool recurseOnSources)
{
    using Predicate = std::function<bool (UsdAttribute const &)>;
    Predicate pred;
    if (!pypred.is_none()) {
        pred = nb::cast<Predicate>(pypred);
    }
    return self.FindAllAttributeConnectionPaths(
        traversal, pred, recurseOnSources);
}

static SdfPathVector
_FindAllAttributeConnectionPathsDefault(
    UsdPrim const &self,
    nb::object pypred,
    bool recurseOnSources)
{
    Usd_PrimFlagsPredicate traversal;
    if (!pypred.is_none() &&
        nb::try_cast<Usd_PrimFlagsPredicate>(pypred, traversal)) {
        return _FindAllAttributeConnectionPaths(
            self, traversal, nb::none(), recurseOnSources);
    }
    return _FindAllAttributeConnectionPaths(
        self, UsdPrimDefaultPredicate, pypred, recurseOnSources);
}

static SdfPathVector
_FindAllRelationshipTargetPaths(
    UsdPrim const &self,
    Usd_PrimFlagsPredicate const &traversal,
    nb::object pypred,
    bool recurseOnTargets)
{
    using Predicate = std::function<bool (UsdRelationship const &)>;
    Predicate pred;
    if (!pypred.is_none()) {
        pred = nb::cast<Predicate>(pypred);
    }
    return self.FindAllRelationshipTargetPaths(
        traversal, pred, recurseOnTargets);
}

static SdfPathVector
_FindAllRelationshipTargetPathsDefault(
    UsdPrim const &self,
    nb::object pypred,
    bool recurseOnTargets)
{
    Usd_PrimFlagsPredicate traversal;
    if (!pypred.is_none() &&
        nb::try_cast<Usd_PrimFlagsPredicate>(pypred, traversal)) {
        return _FindAllRelationshipTargetPaths(
            self, traversal, nb::none(), recurseOnTargets);
    }
    return _FindAllRelationshipTargetPaths(self, UsdPrimDefaultPredicate,
                                           pypred, recurseOnTargets);
}

static string
__repr__(const UsdPrim &self)
{
    if (self) {
        return TF_PY_REPR_PREFIX +
            TfStringPrintf("Prim(<%s>)", self.GetPath().GetText());
    } else {
        return "invalid " + self.GetDescription();
    }
}

static TfTokenVector _WrapGetPropertyNames(
    const UsdPrim &prim, 
    nb::object predicate)
{
    const auto &pred = (!predicate.is_none() && nb::bool_(predicate)) ? 
        nb::cast<UsdPrim::PropertyPredicateFunc>(predicate) :
        UsdPrim::PropertyPredicateFunc();
    return prim.GetPropertyNames(pred);
}

static TfTokenVector _WrapGetAuthoredPropertyNames(
    const UsdPrim &prim, 
    nb::object predicate)
{
    const auto &pred = (!predicate.is_none() && nb::bool_(predicate)) ? 
        nb::cast<UsdPrim::PropertyPredicateFunc>(predicate) :
        UsdPrim::PropertyPredicateFunc();
    return prim.GetAuthoredPropertyNames(pred);
}

static std::vector<UsdProperty>
_WrapGetProperties(const UsdPrim &prim, nb::object predicate)
{
    const auto &pred = (!predicate.is_none() && nb::bool_(predicate)) ? 
        nb::cast<UsdPrim::PropertyPredicateFunc>(predicate) :
        UsdPrim::PropertyPredicateFunc();
    return prim.GetProperties(pred);
}

static std::vector<UsdProperty>
_WrapGetAuthoredProperties(const UsdPrim &prim, nb::object predicate)
{
    const auto &pred = (!predicate.is_none() && nb::bool_(predicate)) ? 
        nb::cast<UsdPrim::PropertyPredicateFunc>(predicate) :
        UsdPrim::PropertyPredicateFunc();
    return prim.GetAuthoredProperties(pred);
}

struct Usd_PrimCanApplyAPIResult : public TfPyAnnotatedBoolResult<string>
{
    Usd_PrimCanApplyAPIResult(bool val, string const &msg) :
        TfPyAnnotatedBoolResult<string>(val, msg) {}
};

template <class... Args>
Usd_PrimCanApplyAPIResult
_WrapCanApply(const UsdPrim &prim, const Args &... args) 
{
    std::string whyNot;
    bool result = prim.CanApplyAPI(args..., &whyNot);
    return Usd_PrimCanApplyAPIResult(result, whyNot);
}

static UsdStageWeakPtr
_UnsafeGetStageForTesting(UsdObject const &obj)
{
    return obj.GetStage();
}

static nb::object
_WrapGetVersionIfIsInFamily(
    const UsdPrim &prim, const TfToken &schemaFamily) 
{
    UsdSchemaVersion version;
    if (prim.GetVersionIfIsInFamily(schemaFamily, &version)) {
        return nb::cast(version);
    }
    return nb::none();
}

static nb::object
_WrapGetVersionIfHasAPIInFamily_1(
    const UsdPrim &prim, const TfToken &schemaFamily) 
{
    UsdSchemaVersion version;
    if (prim.GetVersionIfHasAPIInFamily(schemaFamily, &version)) {
        return nb::cast(version);
    }
    return nb::none();
}

static nb::object
_WrapGetVersionIfHasAPIInFamily_2(
    const UsdPrim &prim, const TfToken &schemaFamily, 
    const TfToken &instanceName) 
{
    UsdSchemaVersion version;
    if (prim.GetVersionIfHasAPIInFamily(
            schemaFamily, instanceName, &version)) {
        return nb::cast(version);
    }
    return nb::none();
}

static TfToken _GetKind(const UsdPrim &self)
{
    TfToken kind;
    self.GetKind(&kind);
    return kind;
}

} // anonymous namespace 

void wrapUsdPrim(nb::module_ &m)
{
    Usd_PrimCanApplyAPIResult::WrapNb<Usd_PrimCanApplyAPIResult>(
        m, "_CanApplyAPIResult", "whyNot");

    // Predicate signature for FindAllRelationshipTargetPaths().
    TfPyFunctionFromPython<bool (UsdRelationship const &)>();

    // Predicate signature for FindAllAttributeConnectionPaths().
    TfPyFunctionFromPython<bool (UsdAttribute const &)>();

    // Predicate signature for Get{Authored}PropertyNames and 
    // Get{Authored}Properties.
    TfPyFunctionFromPython<bool (TfToken const &)>();

    nb::class_<UsdPrim, UsdObject>(m, "Prim")
        .def(nb::init<>())
        .def(Usd_ObjectSubclass())
        .def("__repr__", __repr__)

        .def("GetPrimTypeInfo", &UsdPrim::GetPrimTypeInfo,
             nb::rv_policy::reference_internal)
        .def("GetPrimDefinition", &UsdPrim::GetPrimDefinition,
             nb::rv_policy::reference_internal)
        .def("GetPrimStack", &UsdPrim::GetPrimStack)
        .def("GetPrimStackWithLayerOffsets", 
             &UsdPrim::GetPrimStackWithLayerOffsets)

        .def("GetSpecifier", &UsdPrim::GetSpecifier)
        .def("SetSpecifier", &UsdPrim::SetSpecifier, nb::arg("specifier"))

        .def("GetTypeName", &UsdPrim::GetTypeName,
             nb::rv_policy::copy)
        .def("SetTypeName", &UsdPrim::SetTypeName, nb::arg("typeName"))
        .def("ClearTypeName", &UsdPrim::ClearTypeName)
        .def("HasAuthoredTypeName", &UsdPrim::HasAuthoredTypeName)

        .def("IsActive", &UsdPrim::IsActive)
        .def("SetActive", &UsdPrim::SetActive, nb::arg("active"))
        .def("ClearActive", &UsdPrim::ClearActive)
        .def("HasAuthoredActive", &UsdPrim::HasAuthoredActive)

        .def("GetKind", _GetKind)
        .def("SetKind", &UsdPrim::SetKind, nb::arg("value"))

        .def("IsLoaded", &UsdPrim::IsLoaded)
        .def("IsModel", &UsdPrim::IsModel)
        .def("IsGroup", &UsdPrim::IsGroup)
        .def("IsComponent", &UsdPrim::IsComponent)
        .def("IsSubComponent", &UsdPrim::IsSubComponent)
        .def("IsAbstract", &UsdPrim::IsAbstract)
        .def("IsDefined", &UsdPrim::IsDefined)
        .def("HasClassSpecifier", &UsdPrim::HasClassSpecifier)
        .def("HasDefiningSpecifier", &UsdPrim::HasDefiningSpecifier)

        .def("GetPropertyNames", &_WrapGetPropertyNames,
             nb::arg("predicate")=nb::none())
        .def("GetAuthoredPropertyNames", &_WrapGetAuthoredPropertyNames,
             nb::arg("predicate")=nb::none())
        
        .def("GetProperties", &_WrapGetProperties,
             nb::arg("predicate")=nb::none())

        .def("GetAuthoredProperties", &_WrapGetAuthoredProperties,
             nb::arg("predicate")=nb::none())

        .def("GetPropertiesInNamespace",
             (vector<UsdProperty> (UsdPrim::*)(const vector<string> &) const)
             &UsdPrim::GetPropertiesInNamespace,
             nb::arg("namespaces"))

        .def("GetPropertiesInNamespace",
             (vector<UsdProperty> (UsdPrim::*)(const string &) const)
             &UsdPrim::GetPropertiesInNamespace,
             nb::arg("namespaces"))

        .def("GetAuthoredPropertiesInNamespace",
             (vector<UsdProperty> (UsdPrim::*)(const vector<string> &) const)
             &UsdPrim::GetAuthoredPropertiesInNamespace,
             nb::arg("namespaces"))

        .def("GetAuthoredPropertiesInNamespace",
             (vector<UsdProperty> (UsdPrim::*)(const string &) const)
             &UsdPrim::GetAuthoredPropertiesInNamespace,
             nb::arg("namespaces"))

        .def("GetAppliedSchemas", &UsdPrim::GetAppliedSchemas)

        .def("GetPropertyOrder", &UsdPrim::GetPropertyOrder)
        .def("SetPropertyOrder", &UsdPrim::SetPropertyOrder, nb::arg("order"))
        .def("ClearPropertyOrder", &UsdPrim::ClearPropertyOrder)

        .def("IsA",
            (bool (UsdPrim::*)(const TfType&) const)
            &UsdPrim::IsA, 
            nb::arg("schemaType"))
        .def("IsA",
            (bool (UsdPrim::*)(const TfToken&) const)
            &UsdPrim::IsA, 
            nb::arg("schemaIdentifier"))
        .def("IsA",
            (bool (UsdPrim::*)(const TfToken&, 
                               UsdSchemaVersion) const)
            &UsdPrim::IsA, 
            nb::arg("schemaFamily"),
            nb::arg("version"))

        .def("IsInFamily",
            (bool (UsdPrim::*)(const TfToken&) const)
            &UsdPrim::IsInFamily, 
            nb::arg("schemaFamily"))
        .def("IsInFamily",
            (bool (UsdPrim::*)(const TfToken&, 
                               UsdSchemaVersion, 
                               UsdSchemaRegistry::VersionPolicy) const)
            &UsdPrim::IsInFamily, 
            nb::arg("schemaFamily"),
            nb::arg("version"), 
            nb::arg("versionPolicy"))
        .def("IsInFamily",
            (bool (UsdPrim::*)(const TfType&, 
                               UsdSchemaRegistry::VersionPolicy) const)
            &UsdPrim::IsInFamily, 
            nb::arg("schemaType"), 
            nb::arg("versionPolicy"))
        .def("IsInFamily",
            (bool (UsdPrim::*)(const TfToken&, 
                               UsdSchemaRegistry::VersionPolicy) const)
            &UsdPrim::IsInFamily, 
            nb::arg("schemaIdentifier"), 
            nb::arg("versionPolicy"))

        .def("GetVersionIfIsInFamily",
             &_WrapGetVersionIfIsInFamily)

        .def("HasAPI", 
            (bool (UsdPrim::*)(const TfType&) const)
            &UsdPrim::HasAPI,
            nb::arg("schemaType"))
        .def("HasAPI", 
            (bool (UsdPrim::*)(const TfType&, const TfToken&) const)
            &UsdPrim::HasAPI,
            nb::arg("schemaType"), nb::arg("instanceName"))
        .def("HasAPI", 
            (bool (UsdPrim::*)(const TfToken&) const)
            &UsdPrim::HasAPI,
            nb::arg("schemaIdentifier"))
        .def("HasAPI", 
            (bool (UsdPrim::*)(const TfToken&, const TfToken&) const)
            &UsdPrim::HasAPI,
            nb::arg("schemaIdentifier"), nb::arg("instanceName"))
        .def("HasAPI", 
            (bool (UsdPrim::*)(const TfToken&, UsdSchemaVersion) const)
            &UsdPrim::HasAPI,
            nb::arg("schemaFamily"), nb::arg("schemaVersion"))
        .def("HasAPI", 
            (bool (UsdPrim::*)(const TfToken&, UsdSchemaVersion, 
                               const TfToken&) const)
            &UsdPrim::HasAPI,
            nb::arg("schemaFamily"), nb::arg("schemaVersion"), nb::arg("instanceName"))

        .def("HasAPIInFamily", 
            (bool (UsdPrim::*)(const TfType&, 
                               UsdSchemaRegistry::VersionPolicy) const)
            &UsdPrim::HasAPIInFamily,
            nb::arg("schemaType"), 
            nb::arg("versionPolicy"))
        .def("HasAPIInFamily", 
            (bool (UsdPrim::*)(const TfType&, 
                               UsdSchemaRegistry::VersionPolicy, const TfToken&) const)
            &UsdPrim::HasAPIInFamily,
            nb::arg("schemaType"), 
            nb::arg("versionPolicy"), nb::arg("instanceName"))
        .def("HasAPIInFamily", 
            (bool (UsdPrim::*)(const TfToken&) const)
            &UsdPrim::HasAPIInFamily,
            nb::arg("schemaFamily"))
        .def("HasAPIInFamily", 
            (bool (UsdPrim::*)(const TfToken&, const TfToken&) const)
            &UsdPrim::HasAPIInFamily,
            nb::arg("schemaFamily"), nb::arg("instanceName"))
        .def("HasAPIInFamily", 
            (bool (UsdPrim::*)(const TfToken&, UsdSchemaVersion, 
                               UsdSchemaRegistry::VersionPolicy) const)
            &UsdPrim::HasAPIInFamily,
            nb::arg("schemaFamily"), nb::arg("schemaVersion"), 
            nb::arg("versionPolicy"))
        .def("HasAPIInFamily", 
            (bool (UsdPrim::*)(const TfToken&, UsdSchemaVersion,
                               UsdSchemaRegistry::VersionPolicy, const TfToken&) const)
            &UsdPrim::HasAPIInFamily,
            nb::arg("schemaFamily"), nb::arg("schemaVersion"), 
            nb::arg("versionPolicy"), nb::arg("instanceName"))
        .def("HasAPIInFamily", 
            (bool (UsdPrim::*)(const TfToken&, 
                               UsdSchemaRegistry::VersionPolicy) const)
            &UsdPrim::HasAPIInFamily,
            nb::arg("schemaIdentifier"), 
            nb::arg("versionPolicy"))
        .def("HasAPIInFamily", 
            (bool (UsdPrim::*)(const TfToken&, 
                               UsdSchemaRegistry::VersionPolicy, const TfToken&) const)
            &UsdPrim::HasAPIInFamily,
            nb::arg("schemaIdentifier"), 
            nb::arg("versionPolicy"), nb::arg("instanceName"))

        .def("GetVersionIfHasAPIInFamily", 
             &_WrapGetVersionIfHasAPIInFamily_1)
        .def("GetVersionIfHasAPIInFamily", 
             &_WrapGetVersionIfHasAPIInFamily_2)

        .def("CanApplyAPI", 
            +[](const UsdPrim& prim, const TfType &schemaType) {
                return _WrapCanApply(prim, schemaType);
            },
            nb::arg("schemaType"))
        .def("CanApplyAPI", 
            +[](const UsdPrim& prim, const TfType &schemaType, 
                const TfToken &instanceName) {
                return _WrapCanApply(prim, schemaType, instanceName);
            },
            nb::arg("schemaType"), nb::arg("instanceName"))
        .def("CanApplyAPI", 
            +[](const UsdPrim& prim, const TfToken &schemaIdentifier) {
                return _WrapCanApply(prim, schemaIdentifier);
            },
            nb::arg("schemaIdentifier"))
        .def("CanApplyAPI", 
            +[](const UsdPrim& prim, const TfToken &schemaIdentifier, 
                const TfToken &instanceName) {
                return _WrapCanApply(prim, schemaIdentifier, instanceName);
            },
            nb::arg("schemaIdentifier"), nb::arg("instanceName"))
        .def("CanApplyAPI", 
            +[](const UsdPrim& prim, const TfToken &schemaFamily, 
                UsdSchemaVersion version) {
                return _WrapCanApply(prim, schemaFamily, version);
            },
            nb::arg("schemaFamily"), nb::arg("schemaVersion"))
        .def("CanApplyAPI", 
            +[](const UsdPrim& prim, const TfToken &schemaFamily, 
                UsdSchemaVersion version, const TfToken &instanceName) {
                return _WrapCanApply(prim, schemaFamily, version, instanceName);
            },
            nb::arg("schemaFamily"), nb::arg("schemaVersion"), nb::arg("instanceName"))

        .def("ApplyAPI", 
            (bool (UsdPrim::*)(const TfType&) const)
            &UsdPrim::ApplyAPI,
            nb::arg("schemaType"))
        .def("ApplyAPI", 
            (bool (UsdPrim::*)(const TfType&, const TfToken&) const)
            &UsdPrim::ApplyAPI,
            nb::arg("schemaType"), nb::arg("instanceName"))
        .def("ApplyAPI", 
            (bool (UsdPrim::*)(const TfToken&) const)
            &UsdPrim::ApplyAPI,
            nb::arg("schemaIdentifier"))
        .def("ApplyAPI", 
            (bool (UsdPrim::*)(const TfToken&, const TfToken&) const)
            &UsdPrim::ApplyAPI,
            nb::arg("schemaIdentifier"), nb::arg("instanceName"))
        .def("ApplyAPI", 
            (bool (UsdPrim::*)(const TfToken&, UsdSchemaVersion) const)
            &UsdPrim::ApplyAPI,
            nb::arg("schemaFamily"), nb::arg("schemaVersion"))
        .def("ApplyAPI", 
            (bool (UsdPrim::*)(const TfToken&, UsdSchemaVersion, 
                               const TfToken&) const)
            &UsdPrim::ApplyAPI,
            nb::arg("schemaFamily"), nb::arg("schemaVersion"), nb::arg("instanceName"))

        .def("RemoveAPI", 
            (bool (UsdPrim::*)(const TfType&) const)
            &UsdPrim::RemoveAPI,
            nb::arg("schemaType"))
        .def("RemoveAPI", 
            (bool (UsdPrim::*)(const TfType&, const TfToken&) const)
            &UsdPrim::RemoveAPI,
            nb::arg("schemaType"), nb::arg("instanceName"))
        .def("RemoveAPI", 
            (bool (UsdPrim::*)(const TfToken&) const)
            &UsdPrim::RemoveAPI,
            nb::arg("schemaIdentifier"))
        .def("RemoveAPI", 
            (bool (UsdPrim::*)(const TfToken&, const TfToken&) const)
            &UsdPrim::RemoveAPI,
            nb::arg("schemaIdentifier"), nb::arg("instanceName"))
        .def("RemoveAPI", 
            (bool (UsdPrim::*)(const TfToken&, UsdSchemaVersion) const)
            &UsdPrim::RemoveAPI,
            nb::arg("schemaFamily"), nb::arg("schemaVersion"))
        .def("RemoveAPI", 
            (bool (UsdPrim::*)(const TfToken&, UsdSchemaVersion, 
                               const TfToken&) const)
            &UsdPrim::RemoveAPI,
            nb::arg("schemaFamily"), nb::arg("schemaVersion"), nb::arg("instanceName"))

        .def("AddAppliedSchema", &UsdPrim::AddAppliedSchema)
        .def("RemoveAppliedSchema", &UsdPrim::RemoveAppliedSchema)

        .def("GetChild", &UsdPrim::GetChild, nb::arg("name"))

        .def("GetChildren", [](const UsdPrim &self) {
            auto r = self.GetChildren();
            return std::vector<UsdPrim>(r.begin(), r.end());
        })
        .def("GetAllChildren", [](const UsdPrim &self) {
            auto r = self.GetAllChildren();
            return std::vector<UsdPrim>(r.begin(), r.end());
        })
        .def("GetFilteredChildren", [](const UsdPrim &self, const Usd_PrimFlagsPredicate &predicate) {
            auto r = self.GetFilteredChildren(predicate);
            return std::vector<UsdPrim>(r.begin(), r.end());
        }, nb::arg("predicate"))

        .def("GetChildrenNames", &UsdPrim::GetChildrenNames)
        .def("GetAllChildrenNames", &UsdPrim::GetAllChildrenNames)
        .def("GetFilteredChildrenNames", &UsdPrim::GetFilteredChildrenNames)

        .def("GetChildrenReorder", &UsdPrim::GetChildrenReorder)
        .def("SetChildrenReorder", &UsdPrim::SetChildrenReorder, nb::arg("order"))
        .def("ClearChildrenReorder", &UsdPrim::ClearChildrenReorder)

        .def("GetParent", &UsdPrim::GetParent)
        .def("GetNextSibling", (UsdPrim (UsdPrim::*)() const)
             &UsdPrim::GetNextSibling)
        .def("GetFilteredNextSibling",
             (UsdPrim (UsdPrim::*)(const Usd_PrimFlagsPredicate &) const)
             &UsdPrim::GetFilteredNextSibling)
        .def("IsPseudoRoot", &UsdPrim::IsPseudoRoot)

        .def("HasVariantSets", &UsdPrim::HasVariantSets)
        .def("GetVariantSets", &UsdPrim::GetVariantSets)

        .def("GetVariantSet", &UsdPrim::GetVariantSet)

        .def("GetPrimIndex", &UsdPrim::GetPrimIndex,
             nb::rv_policy::copy)
        .def("ComputeExpandedPrimIndex", &UsdPrim::ComputeExpandedPrimIndex)

        .def("CreateAttribute",
             (UsdAttribute (UsdPrim::*)(
                 const TfToken &, const SdfValueTypeName &,
                 bool, SdfVariability) const)
             &UsdPrim::CreateAttribute,
             nb::arg("name"), nb::arg("typeName"), nb::arg("custom")=true,
             nb::arg("variability")=SdfVariabilityVarying)

        .def("CreateAttribute",
             (UsdAttribute (UsdPrim::*)(
                 const vector<string> &, const SdfValueTypeName &,
                 bool, SdfVariability) const)
             &UsdPrim::CreateAttribute,
             nb::arg("nameElts"), nb::arg("typeName"), nb::arg("custom")=true,
             nb::arg("variability")=SdfVariabilityVarying)

        .def("GetAttributes", &UsdPrim::GetAttributes)
        .def("GetAuthoredAttributes", &UsdPrim::GetAuthoredAttributes)

        .def("GetAttribute", &UsdPrim::GetAttribute, nb::arg("attrName"))
        .def("HasAttribute", &UsdPrim::HasAttribute, nb::arg("attrName"))

        .def("FindAllAttributeConnectionPaths",
             &_FindAllAttributeConnectionPaths,
             nb::arg("traversalPredicate"),
             nb::arg("predicate").none()=nb::none(), nb::arg("recurseOnSources")=false)

        .def("FindAllAttributeConnectionPaths",
             &_FindAllAttributeConnectionPathsDefault,
             nb::arg("predicate").none()=nb::none(), nb::arg("recurseOnSources")=false)

        .def("CreateRelationship",
             (UsdRelationship (UsdPrim::*)(const TfToken &, bool) const)
             &UsdPrim::CreateRelationship, nb::arg("name"), nb::arg("custom")=true)

        .def("CreateRelationship",
             (UsdRelationship (UsdPrim::*)(const vector<string> &, bool) const)
             &UsdPrim::CreateRelationship, nb::arg("nameElts"), nb::arg("custom")=true)

        .def("GetRelationships", &UsdPrim::GetRelationships)
        .def("GetAuthoredRelationships", &UsdPrim::GetAuthoredRelationships)

        .def("GetRelationship", &UsdPrim::GetRelationship, nb::arg("relName"))
        .def("HasRelationship", &UsdPrim::HasRelationship, nb::arg("relName"))

        .def("FindAllRelationshipTargetPaths",
             &_FindAllRelationshipTargetPaths,
             nb::arg("traversalPredicate"),
             nb::arg("predicate").none()=nb::none(), nb::arg("recurseOnTargets")=false)

        .def("FindAllRelationshipTargetPaths",
             &_FindAllRelationshipTargetPathsDefault,
             nb::arg("predicate").none()=nb::none(), nb::arg("recurseOnTargets")=false)

        .def("HasPayload", &UsdPrim::HasPayload)
        .def("SetPayload",
             (bool (UsdPrim::*)(const SdfPayload &) const)
             &UsdPrim::SetPayload, nb::arg("payload"))
        .def("SetPayload",
             (bool (UsdPrim::*)(const string &, const SdfPath &) const)
             &UsdPrim::SetPayload, nb::arg("assetPath"), nb::arg("primPath"))
        .def("SetPayload",
             (bool (UsdPrim::*)(const SdfLayerHandle &, const SdfPath &) const)
             &UsdPrim::SetPayload, nb::arg("layer"), nb::arg("primPath"))
        .def("ClearPayload", &UsdPrim::ClearPayload)

        .def("GetPayloads", &UsdPrim::GetPayloads)
        .def("HasAuthoredPayloads", &UsdPrim::HasAuthoredPayloads)

        .def("Load", &UsdPrim::Load, nb::arg("policy")=UsdLoadWithDescendants)
        .def("Unload", &UsdPrim::Unload)

        .def("GetReferences", &UsdPrim::GetReferences)
        .def("HasAuthoredReferences", &UsdPrim::HasAuthoredReferences)

        .def("GetInherits", &UsdPrim::GetInherits)
        .def("HasAuthoredInherits", &UsdPrim::HasAuthoredInherits)

        .def("GetSpecializes", &UsdPrim::GetSpecializes)
        .def("HasAuthoredSpecializes", &UsdPrim::HasAuthoredSpecializes)
    
        .def("RemoveProperty", &UsdPrim::RemoveProperty, nb::arg("propName"))
        .def("GetProperty", &UsdPrim::GetProperty, nb::arg("propName"))
        .def("HasProperty", &UsdPrim::HasProperty, nb::arg("propName"))

        .def("IsInstanceable", &UsdPrim::IsInstanceable)
        .def("SetInstanceable", &UsdPrim::SetInstanceable, nb::arg("instanceable"))
        .def("ClearInstanceable", &UsdPrim::ClearInstanceable)
        .def("HasAuthoredInstanceable", &UsdPrim::HasAuthoredInstanceable)

        .def_static("IsPrototypePath", &UsdPrim::IsPrototypePath, nb::arg("path"))
        .def_static("IsPathInPrototype", &UsdPrim::IsPathInPrototype, nb::arg("path"))

        .def("IsInstance", &UsdPrim::IsInstance)
        .def("IsPrototype", &UsdPrim::IsPrototype)
        .def("IsInPrototype", &UsdPrim::IsInPrototype)
        .def("GetPrototype", &UsdPrim::GetPrototype)

        .def("IsInstanceProxy", &UsdPrim::IsInstanceProxy)
        .def("GetPrimInPrototype", &UsdPrim::GetPrimInPrototype)

        .def("GetPrimAtPath", &UsdPrim::GetPrimAtPath, nb::arg("path"))
        .def("GetObjectAtPath", &UsdPrim::GetObjectAtPath, nb::arg("path"))
        .def("GetPropertyAtPath", &UsdPrim::GetPropertyAtPath, nb::arg("path"))
        .def("GetAttributeAtPath", &UsdPrim::GetAttributeAtPath, nb::arg("path"))
        .def("GetRelationshipAtPath",
            &UsdPrim::GetRelationshipAtPath, nb::arg("path"))
        .def("GetInstances", &UsdPrim::GetInstances)

        .def("MakeResolveTargetUpToEditTarget", 
            &UsdPrim::MakeResolveTargetUpToEditTarget)
        .def("MakeResolveTargetStrongerThanEditTarget", 
            &UsdPrim::MakeResolveTargetStrongerThanEditTarget)
        
        // Exposed only for testing and debugging.
        .def("_GetSourcePrimIndex", &Usd_PrimGetSourcePrimIndex,
             nb::rv_policy::copy)
        ;
    

    TfPyRegisterStlSequencesFromPython<UsdPrim>();
    TfPyContainerConversions::tuple_mapping_pair<
        std::pair<SdfPrimSpecHandle, SdfLayerOffset>>();

    // This is wrapped in order to let python call an API that will get through
    // our usual Python API guards to access an invalid prim and throw an
    // exception.
    m.def("_UnsafeGetStageForTesting", &_UnsafeGetStageForTesting);
    
}
