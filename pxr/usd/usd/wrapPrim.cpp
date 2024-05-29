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

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/operators.hpp>

#include <string>
#include <vector>

using std::string;
using std::vector;

using namespace boost::python;

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
    boost::python::object pypred,
    bool recurseOnSources)
{
    using Predicate = std::function<bool (UsdAttribute const &)>;
    Predicate pred;
    if (!pypred.is_none()) {
        pred = boost::python::extract<Predicate>(pypred);
    }
    return self.FindAllAttributeConnectionPaths(
        traversal, pred, recurseOnSources);
}

static SdfPathVector
_FindAllAttributeConnectionPathsDefault(
    UsdPrim const &self,
    boost::python::object pypred,
    bool recurseOnSources)
{
    return _FindAllAttributeConnectionPaths(
        self, UsdPrimDefaultPredicate, pypred, recurseOnSources);
}

static SdfPathVector
_FindAllRelationshipTargetPaths(
    UsdPrim const &self,
    Usd_PrimFlagsPredicate const &traversal,
    boost::python::object pypred,
    bool recurseOnTargets)
{
    using Predicate = std::function<bool (UsdRelationship const &)>;
    Predicate pred;
    if (!pypred.is_none()) {
        pred = boost::python::extract<Predicate>(pypred);
    }
    return self.FindAllRelationshipTargetPaths(
        traversal, pred, recurseOnTargets);
}

static SdfPathVector
_FindAllRelationshipTargetPathsDefault(
    UsdPrim const &self,
    boost::python::object pypred,
    bool recurseOnTargets)
{
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
    boost::python::object predicate)
{
    const auto &pred = predicate ? 
        boost::python::extract<UsdPrim::PropertyPredicateFunc>(predicate) :
        UsdPrim::PropertyPredicateFunc();
    return prim.GetPropertyNames(pred);
}

static TfTokenVector _WrapGetAuthoredPropertyNames(
    const UsdPrim &prim, 
    boost::python::object predicate)
{
    const auto &pred = predicate ? 
        boost::python::extract<UsdPrim::PropertyPredicateFunc>(predicate) :
        UsdPrim::PropertyPredicateFunc();
    return prim.GetAuthoredPropertyNames(pred);
}

static std::vector<UsdProperty>
_WrapGetProperties(const UsdPrim &prim, boost::python::object predicate)
{
    const auto &pred = predicate ? 
        boost::python::extract<UsdPrim::PropertyPredicateFunc>(predicate) :
        UsdPrim::PropertyPredicateFunc();
    return prim.GetProperties(pred);
}

static std::vector<UsdProperty>
_WrapGetAuthoredProperties(const UsdPrim &prim, boost::python::object predicate)
{
    const auto &pred = predicate ? 
        boost::python::extract<UsdPrim::PropertyPredicateFunc>(predicate) :
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

static object
_WrapGetVersionIfIsInFamily(
    const UsdPrim &prim, const TfToken &schemaFamily) 
{
    UsdSchemaVersion version;
    if (prim.GetVersionIfIsInFamily(schemaFamily, &version)) {
        return object(version);
    }
    return object();
}

static object
_WrapGetVersionIfHasAPIInFamily_1(
    const UsdPrim &prim, const TfToken &schemaFamily) 
{
    UsdSchemaVersion version;
    if (prim.GetVersionIfHasAPIInFamily(schemaFamily, &version)) {
        return object(version);
    }
    return object();
}

static object
_WrapGetVersionIfHasAPIInFamily_2(
    const UsdPrim &prim, const TfToken &schemaFamily, 
    const TfToken &instanceName) 
{
    UsdSchemaVersion version;
    if (prim.GetVersionIfHasAPIInFamily(
            schemaFamily, instanceName, &version)) {
        return object(version);
    }
    return object();
}

static TfToken _GetKind(const UsdPrim &self)
{
    TfToken kind;
    self.GetKind(&kind);
    return kind;
}


} // anonymous namespace 

void wrapUsdPrim()
{
    Usd_PrimCanApplyAPIResult::Wrap<Usd_PrimCanApplyAPIResult>(
        "_CanApplyAPIResult", "whyNot");

    // Predicate signature for FindAllRelationshipTargetPaths().
    TfPyFunctionFromPython<bool (UsdRelationship const &)>();

    // Predicate signature for FindAllAttributeConnectionPaths().
    TfPyFunctionFromPython<bool (UsdAttribute const &)>();

    // Predicate signature for Get{Authored}PropertyNames and 
    // Get{Authored}Properties.
    TfPyFunctionFromPython<bool (TfToken const &)>();

    class_<UsdPrim, bases<UsdObject> >("Prim")
        .def(Usd_ObjectSubclass())
        .def("__repr__", __repr__)

        .def("GetPrimTypeInfo", &UsdPrim::GetPrimTypeInfo,
             return_internal_reference<>())
        .def("GetPrimDefinition", &UsdPrim::GetPrimDefinition,
             return_internal_reference<>())
        .def("GetPrimStack", &UsdPrim::GetPrimStack)
        .def("GetPrimStackWithLayerOffsets", 
             &UsdPrim::GetPrimStackWithLayerOffsets,
             return_value_policy<TfPySequenceToList>())

        .def("GetSpecifier", &UsdPrim::GetSpecifier)
        .def("SetSpecifier", &UsdPrim::SetSpecifier, arg("specifier"))

        .def("GetTypeName", &UsdPrim::GetTypeName,
             return_value_policy<return_by_value>())
        .def("SetTypeName", &UsdPrim::SetTypeName, arg("typeName"))
        .def("ClearTypeName", &UsdPrim::ClearTypeName)
        .def("HasAuthoredTypeName", &UsdPrim::HasAuthoredTypeName)

        .def("IsActive", &UsdPrim::IsActive)
        .def("SetActive", &UsdPrim::SetActive, arg("active"))
        .def("ClearActive", &UsdPrim::ClearActive)
        .def("HasAuthoredActive", &UsdPrim::HasAuthoredActive)

        .def("GetKind", _GetKind)
        .def("SetKind", &UsdPrim::SetKind, arg("value"))

        .def("IsLoaded", &UsdPrim::IsLoaded)
        .def("IsModel", &UsdPrim::IsModel)
        .def("IsGroup", &UsdPrim::IsGroup)
        .def("IsComponent", &UsdPrim::IsComponent)
        .def("IsSubComponent", &UsdPrim::IsSubComponent)
        .def("IsAbstract", &UsdPrim::IsAbstract)
        .def("IsDefined", &UsdPrim::IsDefined)
        .def("HasDefiningSpecifier", &UsdPrim::HasDefiningSpecifier)

        .def("GetPropertyNames", &_WrapGetPropertyNames,
             (arg("predicate")=boost::python::object()),
             return_value_policy<TfPySequenceToList>())
        .def("GetAuthoredPropertyNames", &_WrapGetAuthoredPropertyNames,
             (arg("predicate")=boost::python::object()),
             return_value_policy<TfPySequenceToList>())
        
        .def("GetProperties", &_WrapGetProperties,
             arg("predicate")=boost::python::object(),
             return_value_policy<TfPySequenceToList>())

        .def("GetAuthoredProperties", &_WrapGetAuthoredProperties,
             arg("predicate")=boost::python::object(),
             return_value_policy<TfPySequenceToList>())

        .def("GetPropertiesInNamespace",
             (vector<UsdProperty> (UsdPrim::*)(const vector<string> &) const)
             &UsdPrim::GetPropertiesInNamespace,
             arg("namespaces"),
             return_value_policy<TfPySequenceToList>())

        .def("GetPropertiesInNamespace",
             (vector<UsdProperty> (UsdPrim::*)(const string &) const)
             &UsdPrim::GetPropertiesInNamespace,
             arg("namespaces"),
             return_value_policy<TfPySequenceToList>())

        .def("GetAuthoredPropertiesInNamespace",
             (vector<UsdProperty> (UsdPrim::*)(const vector<string> &) const)
             &UsdPrim::GetAuthoredPropertiesInNamespace,
             arg("namespaces"),
             return_value_policy<TfPySequenceToList>())

        .def("GetAuthoredPropertiesInNamespace",
             (vector<UsdProperty> (UsdPrim::*)(const string &) const)
             &UsdPrim::GetAuthoredPropertiesInNamespace,
             arg("namespaces"),
             return_value_policy<TfPySequenceToList>())

        .def("GetAppliedSchemas", &UsdPrim::GetAppliedSchemas,
             return_value_policy<TfPySequenceToList>())

        .def("GetPropertyOrder", &UsdPrim::GetPropertyOrder,
             return_value_policy<TfPySequenceToList>())
        .def("SetPropertyOrder", &UsdPrim::SetPropertyOrder, arg("order"))
        .def("ClearPropertyOrder", &UsdPrim::ClearPropertyOrder)

        .def("IsA",
            (bool (UsdPrim::*)(const TfType&) const)
            &UsdPrim::IsA, 
            (arg("schemaType")))
        .def("IsA",
            (bool (UsdPrim::*)(const TfToken&) const)
            &UsdPrim::IsA, 
            (arg("schemaIdentifier")))
        .def("IsA",
            (bool (UsdPrim::*)(const TfToken&, 
                               UsdSchemaVersion) const)
            &UsdPrim::IsA, 
            (arg("schemaFamily"),
             arg("version")))

        .def("IsInFamily",
            (bool (UsdPrim::*)(const TfToken&) const)
            &UsdPrim::IsInFamily, 
            (arg("schemaFamily")))
        .def("IsInFamily",
            (bool (UsdPrim::*)(const TfToken&, 
                               UsdSchemaVersion, 
                               UsdSchemaRegistry::VersionPolicy) const)
            &UsdPrim::IsInFamily, 
            (arg("schemaFamily"),
             arg("version"), 
             arg("versionPolicy")))
        .def("IsInFamily",
            (bool (UsdPrim::*)(const TfType&, 
                               UsdSchemaRegistry::VersionPolicy) const)
            &UsdPrim::IsInFamily, 
            (arg("schemaType"), 
             arg("versionPolicy")))
        .def("IsInFamily",
            (bool (UsdPrim::*)(const TfToken&, 
                               UsdSchemaRegistry::VersionPolicy) const)
            &UsdPrim::IsInFamily, 
            (arg("schemaIdentifier"), 
             arg("versionPolicy")))

        .def("GetVersionIfIsInFamily",
             &_WrapGetVersionIfIsInFamily)

        .def("HasAPI", 
            (bool (UsdPrim::*)(const TfType&) const)
            &UsdPrim::HasAPI,
            (arg("schemaType")))
        .def("HasAPI", 
            (bool (UsdPrim::*)(const TfType&, const TfToken&) const)
            &UsdPrim::HasAPI,
            (arg("schemaType"), arg("instanceName")))
        .def("HasAPI", 
            (bool (UsdPrim::*)(const TfToken&) const)
            &UsdPrim::HasAPI,
            (arg("schemaIdentifier")))
        .def("HasAPI", 
            (bool (UsdPrim::*)(const TfToken&, const TfToken&) const)
            &UsdPrim::HasAPI,
            (arg("schemaIdentifier"), arg("instanceName")))
        .def("HasAPI", 
            (bool (UsdPrim::*)(const TfToken&, UsdSchemaVersion) const)
            &UsdPrim::HasAPI,
            (arg("schemaFamily"), arg("schemaVersion")))
        .def("HasAPI", 
            (bool (UsdPrim::*)(const TfToken&, UsdSchemaVersion, 
                               const TfToken&) const)
            &UsdPrim::HasAPI,
            (arg("schemaFamily"), arg("schemaVersion"), arg("instanceName")))

        .def("HasAPIInFamily", 
            (bool (UsdPrim::*)(const TfType&, 
                               UsdSchemaRegistry::VersionPolicy) const)
            &UsdPrim::HasAPIInFamily,
            (arg("schemaType"), 
             arg("versionPolicy")))
        .def("HasAPIInFamily", 
            (bool (UsdPrim::*)(const TfType&, 
                               UsdSchemaRegistry::VersionPolicy, const TfToken&) const)
            &UsdPrim::HasAPIInFamily,
            (arg("schemaType"), 
             arg("versionPolicy"), arg("instanceName")))
        .def("HasAPIInFamily", 
            (bool (UsdPrim::*)(const TfToken&) const)
            &UsdPrim::HasAPIInFamily,
            (arg("schemaFamily")))
        .def("HasAPIInFamily", 
            (bool (UsdPrim::*)(const TfToken&, const TfToken&) const)
            &UsdPrim::HasAPIInFamily,
            (arg("schemaFamily"), arg("instanceName")))
        .def("HasAPIInFamily", 
            (bool (UsdPrim::*)(const TfToken&, UsdSchemaVersion, 
                               UsdSchemaRegistry::VersionPolicy) const)
            &UsdPrim::HasAPIInFamily,
            (arg("schemaFamily"), arg("schemaVersion"), 
             arg("versionPolicy")))
        .def("HasAPIInFamily", 
            (bool (UsdPrim::*)(const TfToken&, UsdSchemaVersion,
                               UsdSchemaRegistry::VersionPolicy, const TfToken&) const)
            &UsdPrim::HasAPIInFamily,
            (arg("schemaFamily"), arg("schemaVersion"), 
             arg("versionPolicy"), arg("instanceName")))
        .def("HasAPIInFamily", 
            (bool (UsdPrim::*)(const TfToken&, 
                               UsdSchemaRegistry::VersionPolicy) const)
            &UsdPrim::HasAPIInFamily,
            (arg("schemaIdentifier"), 
             arg("versionPolicy")))
        .def("HasAPIInFamily", 
            (bool (UsdPrim::*)(const TfToken&, 
                               UsdSchemaRegistry::VersionPolicy, const TfToken&) const)
            &UsdPrim::HasAPIInFamily,
            (arg("schemaIdentifier"), 
             arg("versionPolicy"), arg("instanceName")))

        .def("GetVersionIfHasAPIInFamily", 
             &_WrapGetVersionIfHasAPIInFamily_1)
        .def("GetVersionIfHasAPIInFamily", 
             &_WrapGetVersionIfHasAPIInFamily_2)

        .def("CanApplyAPI", 
            +[](const UsdPrim& prim, const TfType &schemaType) {
                return _WrapCanApply(prim, schemaType);
            },
            (arg("schemaType")))
        .def("CanApplyAPI", 
            +[](const UsdPrim& prim, const TfType &schemaType, 
                const TfToken &instanceName) {
                return _WrapCanApply(prim, schemaType, instanceName);
            },
            (arg("schemaType"), arg("instanceName")))
        .def("CanApplyAPI", 
            +[](const UsdPrim& prim, const TfToken &schemaIdentifier) {
                return _WrapCanApply(prim, schemaIdentifier);
            },
            (arg("schemaIdentifier")))
        .def("CanApplyAPI", 
            +[](const UsdPrim& prim, const TfToken &schemaIdentifier, 
                const TfToken &instanceName) {
                return _WrapCanApply(prim, schemaIdentifier, instanceName);
            },
            (arg("schemaIdentifier"), arg("instanceName")))
        .def("CanApplyAPI", 
            +[](const UsdPrim& prim, const TfToken &schemaFamily, 
                UsdSchemaVersion version) {
                return _WrapCanApply(prim, schemaFamily, version);
            },
            (arg("schemaFamily"), arg("schemaVersion")))
        .def("CanApplyAPI", 
            +[](const UsdPrim& prim, const TfToken &schemaFamily, 
                UsdSchemaVersion version, const TfToken &instanceName) {
                return _WrapCanApply(prim, schemaFamily, version, instanceName);
            },
            (arg("schemaFamily"), arg("schemaVersion"), arg("instanceName")))

        .def("ApplyAPI", 
            (bool (UsdPrim::*)(const TfType&) const)
            &UsdPrim::ApplyAPI,
            (arg("schemaType")))
        .def("ApplyAPI", 
            (bool (UsdPrim::*)(const TfType&, const TfToken&) const)
            &UsdPrim::ApplyAPI,
            (arg("schemaType"), arg("instanceName")))
        .def("ApplyAPI", 
            (bool (UsdPrim::*)(const TfToken&) const)
            &UsdPrim::ApplyAPI,
            (arg("schemaIdentifier")))
        .def("ApplyAPI", 
            (bool (UsdPrim::*)(const TfToken&, const TfToken&) const)
            &UsdPrim::ApplyAPI,
            (arg("schemaIdentifier"), arg("instanceName")))
        .def("ApplyAPI", 
            (bool (UsdPrim::*)(const TfToken&, UsdSchemaVersion) const)
            &UsdPrim::ApplyAPI,
            (arg("schemaFamily"), arg("schemaVersion")))
        .def("ApplyAPI", 
            (bool (UsdPrim::*)(const TfToken&, UsdSchemaVersion, 
                               const TfToken&) const)
            &UsdPrim::ApplyAPI,
            (arg("schemaFamily"), arg("schemaVersion"), arg("instanceName")))

        .def("RemoveAPI", 
            (bool (UsdPrim::*)(const TfType&) const)
            &UsdPrim::RemoveAPI,
            (arg("schemaType")))
        .def("RemoveAPI", 
            (bool (UsdPrim::*)(const TfType&, const TfToken&) const)
            &UsdPrim::RemoveAPI,
            (arg("schemaType"), arg("instanceName")))
        .def("RemoveAPI", 
            (bool (UsdPrim::*)(const TfToken&) const)
            &UsdPrim::RemoveAPI,
            (arg("schemaIdentifier")))
        .def("RemoveAPI", 
            (bool (UsdPrim::*)(const TfToken&, const TfToken&) const)
            &UsdPrim::RemoveAPI,
            (arg("schemaIdentifier"), arg("instanceName")))
        .def("RemoveAPI", 
            (bool (UsdPrim::*)(const TfToken&, UsdSchemaVersion) const)
            &UsdPrim::RemoveAPI,
            (arg("schemaFamily"), arg("schemaVersion")))
        .def("RemoveAPI", 
            (bool (UsdPrim::*)(const TfToken&, UsdSchemaVersion, 
                               const TfToken&) const)
            &UsdPrim::RemoveAPI,
            (arg("schemaFamily"), arg("schemaVersion"), arg("instanceName")))

        .def("AddAppliedSchema", &UsdPrim::AddAppliedSchema)
        .def("RemoveAppliedSchema", &UsdPrim::RemoveAppliedSchema)

        .def("GetChild", &UsdPrim::GetChild, arg("name"))

        .def("GetChildren", &UsdPrim::GetChildren,
             return_value_policy<TfPySequenceToList>())
        .def("GetAllChildren", &UsdPrim::GetAllChildren,
             return_value_policy<TfPySequenceToList>())
        .def("GetFilteredChildren", &UsdPrim::GetFilteredChildren,
             arg("predicate"),
             return_value_policy<TfPySequenceToList>())

        .def("GetChildrenNames", &UsdPrim::GetChildrenNames,
             return_value_policy<TfPySequenceToList>())
        .def("GetAllChildrenNames", &UsdPrim::GetAllChildrenNames,
             return_value_policy<TfPySequenceToList>())
        .def("GetFilteredChildrenNames", &UsdPrim::GetFilteredChildrenNames,
             return_value_policy<TfPySequenceToList>())

        .def("GetChildrenReorder", &UsdPrim::GetChildrenReorder,
             return_value_policy<TfPySequenceToList>())
        .def("SetChildrenReorder", &UsdPrim::SetChildrenReorder, arg("order"))
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
             return_value_policy<return_by_value>())
        .def("ComputeExpandedPrimIndex", &UsdPrim::ComputeExpandedPrimIndex)

        .def("CreateAttribute",
             (UsdAttribute (UsdPrim::*)(
                 const TfToken &, const SdfValueTypeName &,
                 bool, SdfVariability) const)
             &UsdPrim::CreateAttribute,
             (arg("name"), arg("typeName"), arg("custom")=true,
              arg("variability")=SdfVariabilityVarying))

        .def("CreateAttribute",
             (UsdAttribute (UsdPrim::*)(
                 const vector<string> &, const SdfValueTypeName &,
                 bool, SdfVariability) const)
             &UsdPrim::CreateAttribute,
             (arg("nameElts"), arg("typeName"), arg("custom")=true,
              arg("variability")=SdfVariabilityVarying))


        .def("GetAttributes", &UsdPrim::GetAttributes,
             return_value_policy<TfPySequenceToList>())
        .def("GetAuthoredAttributes", &UsdPrim::GetAuthoredAttributes,
             return_value_policy<TfPySequenceToList>())

        .def("GetAttribute", &UsdPrim::GetAttribute, arg("attrName"))
        .def("HasAttribute", &UsdPrim::HasAttribute, arg("attrName"))

        .def("FindAllAttributeConnectionPaths",
             &_FindAllAttributeConnectionPathsDefault,
             (arg("predicate")=object(), arg("recurseOnSources")=false))
        
        .def("FindAllAttributeConnectionPaths",
             &_FindAllAttributeConnectionPaths,
             (arg("traversalPredicate"),
              arg("predicate")=object(), arg("recurseOnSources")=false))

        .def("CreateRelationship",
             (UsdRelationship (UsdPrim::*)(const TfToken &, bool) const)
             &UsdPrim::CreateRelationship, (arg("name"), arg("custom")=true))

        .def("CreateRelationship",
             (UsdRelationship (UsdPrim::*)(const vector<string> &, bool) const)
             &UsdPrim::CreateRelationship, (arg("nameElts"), arg("custom")=true))

        .def("GetRelationships", &UsdPrim::GetRelationships,
             return_value_policy<TfPySequenceToList>())
        .def("GetAuthoredRelationships", &UsdPrim::GetAuthoredRelationships,
             return_value_policy<TfPySequenceToList>())

        .def("GetRelationship", &UsdPrim::GetRelationship, arg("relName"))
        .def("HasRelationship", &UsdPrim::HasRelationship, arg("relName"))

        .def("FindAllRelationshipTargetPaths",
             &_FindAllRelationshipTargetPathsDefault,
             (arg("predicate")=object(), arg("recurseOnTargets")=false))

        .def("FindAllRelationshipTargetPaths",
             &_FindAllRelationshipTargetPaths,
             (arg("traversalPredicate"),
              arg("predicate")=object(), arg("recurseOnTargets")=false))

        .def("HasPayload", &UsdPrim::HasPayload)
        .def("SetPayload",
             (bool (UsdPrim::*)(const SdfPayload &) const)
             &UsdPrim::SetPayload, (arg("payload")))
        .def("SetPayload",
             (bool (UsdPrim::*)(const string &, const SdfPath &) const)
             &UsdPrim::SetPayload, (arg("assetPath"), arg("primPath")))
        .def("SetPayload",
             (bool (UsdPrim::*)(const SdfLayerHandle &, const SdfPath &) const)
             &UsdPrim::SetPayload, (arg("layer"), arg("primPath")))
        .def("ClearPayload", &UsdPrim::ClearPayload)

        .def("GetPayloads", &UsdPrim::GetPayloads)
        .def("HasAuthoredPayloads", &UsdPrim::HasAuthoredPayloads)

        .def("Load", &UsdPrim::Load, (arg("policy")=UsdLoadWithDescendants))
        .def("Unload", &UsdPrim::Unload)

        .def("GetReferences", &UsdPrim::GetReferences)
        .def("HasAuthoredReferences", &UsdPrim::HasAuthoredReferences)

        .def("GetInherits", &UsdPrim::GetInherits)
        .def("HasAuthoredInherits", &UsdPrim::HasAuthoredInherits)

        .def("GetSpecializes", &UsdPrim::GetSpecializes)
        .def("HasAuthoredSpecializes", &UsdPrim::HasAuthoredSpecializes)
    
        .def("RemoveProperty", &UsdPrim::RemoveProperty, arg("propName"))
        .def("GetProperty", &UsdPrim::GetProperty, arg("propName"))
        .def("HasProperty", &UsdPrim::HasProperty, arg("propName"))

        .def("IsInstanceable", &UsdPrim::IsInstanceable)
        .def("SetInstanceable", &UsdPrim::SetInstanceable, arg("instanceable"))
        .def("ClearInstanceable", &UsdPrim::ClearInstanceable)
        .def("HasAuthoredInstanceable", &UsdPrim::HasAuthoredInstanceable)

        .def("IsPrototypePath", &UsdPrim::IsPrototypePath, arg("path"))
        .staticmethod("IsPrototypePath")
        .def("IsPathInPrototype", &UsdPrim::IsPathInPrototype, arg("path"))
        .staticmethod("IsPathInPrototype")

        .def("IsInstance", &UsdPrim::IsInstance)
        .def("IsPrototype", &UsdPrim::IsPrototype)
        .def("IsInPrototype", &UsdPrim::IsInPrototype)
        .def("GetPrototype", &UsdPrim::GetPrototype)

        .def("IsInstanceProxy", &UsdPrim::IsInstanceProxy)
        .def("GetPrimInPrototype", &UsdPrim::GetPrimInPrototype)

        .def("GetPrimAtPath", &UsdPrim::GetPrimAtPath, arg("path"))
        .def("GetObjectAtPath", &UsdPrim::GetObjectAtPath, arg("path"))
        .def("GetPropertyAtPath", &UsdPrim::GetPropertyAtPath, arg("path"))
        .def("GetAttributeAtPath", &UsdPrim::GetAttributeAtPath, arg("path"))
        .def("GetRelationshipAtPath",
            &UsdPrim::GetRelationshipAtPath, arg("path"))
        .def("GetInstances", &UsdPrim::GetInstances,
                return_value_policy<TfPySequenceToList>())

        .def("MakeResolveTargetUpToEditTarget", 
            &UsdPrim::MakeResolveTargetUpToEditTarget)
        .def("MakeResolveTargetStrongerThanEditTarget", 
            &UsdPrim::MakeResolveTargetStrongerThanEditTarget)
        
        // Exposed only for testing and debugging.
        .def("_GetSourcePrimIndex", &Usd_PrimGetSourcePrimIndex,
             return_value_policy<return_by_value>())
        ;
    
    to_python_converter<std::vector<UsdPrim>, 
                        TfPySequenceToPython<std::vector<UsdPrim>>>();

    TfPyRegisterStlSequencesFromPython<UsdPrim>();
    TfPyContainerConversions::tuple_mapping_pair<
        std::pair<SdfPrimSpecHandle, SdfLayerOffset>>();

    // This is wrapped in order to let python call an API that will get through
    // our usual Python API guards to access an invalid prim and throw an
    // exception.
    boost::python::def(
        "_UnsafeGetStageForTesting", &_UnsafeGetStageForTesting);
    
}
