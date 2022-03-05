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
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/payloads.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/references.h"
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
    boost::python::object pypred,
    bool recurseOnSources)
{
    using Predicate = std::function<bool (UsdAttribute const &)>;
    Predicate pred;
    if (!pypred.is_none())
        pred = boost::python::extract<Predicate>(pypred);
    return self.FindAllAttributeConnectionPaths(pred, recurseOnSources);
}

static SdfPathVector
_FindAllRelationshipTargetPaths(
    UsdPrim const &self,
    boost::python::object pypred,
    bool recurseOnTargets)
{
    using Predicate = std::function<bool (UsdRelationship const &)>;
    Predicate pred;
    if (!pypred.is_none())
        pred = boost::python::extract<Predicate>(pypred);
    return self.FindAllRelationshipTargetPaths(pred, recurseOnTargets);
}

static std::string
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

struct Usd_PrimCanApplyAPIResult : public TfPyAnnotatedBoolResult<std::string>
{
    Usd_PrimCanApplyAPIResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static Usd_PrimCanApplyAPIResult
_WrapCanApplyAPI(
    const UsdPrim &prim,
    const TfType& schemaType)
{
    std::string whyNot;
    bool result = prim.CanApplyAPI(schemaType, &whyNot);
    return Usd_PrimCanApplyAPIResult(result, whyNot);
}

static Usd_PrimCanApplyAPIResult
_WrapCanApplyAPI_2(
    const UsdPrim &prim,
    const TfType& schemaType,
    const TfToken& instanceName)
{
    std::string whyNot;
    bool result = prim.CanApplyAPI(schemaType, instanceName, &whyNot);
    return Usd_PrimCanApplyAPIResult(result, whyNot);
}

static UsdStageWeakPtr
_UnsafeGetStageForTesting(UsdObject const &obj)
{
    return obj.GetStage();
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

    boost::python::class_<UsdPrim, boost::python::bases<UsdObject> >("Prim")
        .def(Usd_ObjectSubclass())
        .def("__repr__", __repr__)

        .def("GetPrimTypeInfo", &UsdPrim::GetPrimTypeInfo,
             boost::python::return_internal_reference<>())
        .def("GetPrimDefinition", &UsdPrim::GetPrimDefinition,
             boost::python::return_internal_reference<>())
        .def("GetPrimStack", &UsdPrim::GetPrimStack)

        .def("GetSpecifier", &UsdPrim::GetSpecifier)
        .def("SetSpecifier", &UsdPrim::SetSpecifier, boost::python::arg("specifier"))

        .def("GetTypeName", &UsdPrim::GetTypeName,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .def("SetTypeName", &UsdPrim::SetTypeName, boost::python::arg("typeName"))
        .def("ClearTypeName", &UsdPrim::ClearTypeName)
        .def("HasAuthoredTypeName", &UsdPrim::HasAuthoredTypeName)

        .def("IsActive", &UsdPrim::IsActive)
        .def("SetActive", &UsdPrim::SetActive, boost::python::arg("active"))
        .def("ClearActive", &UsdPrim::ClearActive)
        .def("HasAuthoredActive", &UsdPrim::HasAuthoredActive)

        .def("IsLoaded", &UsdPrim::IsLoaded)
        .def("IsModel", &UsdPrim::IsModel)
        .def("IsGroup", &UsdPrim::IsGroup)
        .def("IsAbstract", &UsdPrim::IsAbstract)
        .def("IsDefined", &UsdPrim::IsDefined)
        .def("HasDefiningSpecifier", &UsdPrim::HasDefiningSpecifier)

        .def("GetPropertyNames", &_WrapGetPropertyNames,
             (boost::python::arg("predicate")=boost::python::object()),
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("GetAuthoredPropertyNames", &_WrapGetAuthoredPropertyNames,
             (boost::python::arg("predicate")=boost::python::object()),
             boost::python::return_value_policy<TfPySequenceToList>())
        
        .def("GetProperties", &_WrapGetProperties,
             boost::python::arg("predicate")=boost::python::object(),
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("GetAuthoredProperties", &_WrapGetAuthoredProperties,
             boost::python::arg("predicate")=boost::python::object(),
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("GetPropertiesInNamespace",
             (std::vector<UsdProperty> (UsdPrim::*)(const std::vector<std::string> &) const)
             &UsdPrim::GetPropertiesInNamespace,
             boost::python::arg("namespaces"),
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("GetPropertiesInNamespace",
             (std::vector<UsdProperty> (UsdPrim::*)(const std::string &) const)
             &UsdPrim::GetPropertiesInNamespace,
             boost::python::arg("namespaces"),
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("GetAuthoredPropertiesInNamespace",
             (std::vector<UsdProperty> (UsdPrim::*)(const std::vector<std::string> &) const)
             &UsdPrim::GetAuthoredPropertiesInNamespace,
             boost::python::arg("namespaces"),
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("GetAuthoredPropertiesInNamespace",
             (std::vector<UsdProperty> (UsdPrim::*)(const std::string &) const)
             &UsdPrim::GetAuthoredPropertiesInNamespace,
             boost::python::arg("namespaces"),
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("GetAppliedSchemas", &UsdPrim::GetAppliedSchemas,
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("GetPropertyOrder", &UsdPrim::GetPropertyOrder,
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("SetPropertyOrder", &UsdPrim::SetPropertyOrder, boost::python::arg("order"))
        .def("ClearPropertyOrder", &UsdPrim::ClearPropertyOrder)

        .def("IsA",
            (bool (UsdPrim::*)(const TfType&) const)&UsdPrim::IsA, 
            boost::python::arg("schemaType"))
        .def("HasAPI", 
            (bool (UsdPrim::*)(const TfType&, const TfToken&) const)
            &UsdPrim::HasAPI,
            (boost::python::arg("schemaType"), boost::python::arg("instanceName")=TfToken()))
        .def("CanApplyAPI", 
            &_WrapCanApplyAPI,
            (boost::python::arg("schemaType")))
        .def("CanApplyAPI", 
            &_WrapCanApplyAPI_2,
            (boost::python::arg("schemaType"), boost::python::arg("instanceName")))
        .def("ApplyAPI", 
            (bool (UsdPrim::*)(const TfType&) const)
            &UsdPrim::ApplyAPI,
            (boost::python::arg("schemaType")))
        .def("ApplyAPI", 
            (bool (UsdPrim::*)(const TfType&, const TfToken&) const)
            &UsdPrim::ApplyAPI,
            (boost::python::arg("schemaType"), boost::python::arg("instanceName")))
        .def("RemoveAPI", 
            (bool (UsdPrim::*)(const TfType&) const)
            &UsdPrim::RemoveAPI,
            (boost::python::arg("schemaType")))
        .def("RemoveAPI", 
            (bool (UsdPrim::*)(const TfType&, const TfToken&) const)
            &UsdPrim::RemoveAPI,
            (boost::python::arg("schemaType"), boost::python::arg("instanceName")))

        .def("AddAppliedSchema", &UsdPrim::AddAppliedSchema)
        .def("RemoveAppliedSchema", &UsdPrim::RemoveAppliedSchema)

        .def("GetChild", &UsdPrim::GetChild, boost::python::arg("name"))

        .def("GetChildren", &UsdPrim::GetChildren,
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("GetAllChildren", &UsdPrim::GetAllChildren,
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("GetFilteredChildren", &UsdPrim::GetFilteredChildren,
             boost::python::arg("predicate"),
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("GetChildrenNames", &UsdPrim::GetChildrenNames,
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("GetAllChildrenNames", &UsdPrim::GetAllChildrenNames,
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("GetFilteredChildrenNames", &UsdPrim::GetFilteredChildrenNames,
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("GetChildrenReorder", &UsdPrim::GetChildrenReorder,
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("SetChildrenReorder", &UsdPrim::SetChildrenReorder, boost::python::arg("order"))
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
             boost::python::return_value_policy<boost::python::return_by_value>())
        .def("ComputeExpandedPrimIndex", &UsdPrim::ComputeExpandedPrimIndex)

        .def("CreateAttribute",
             (UsdAttribute (UsdPrim::*)(
                 const TfToken &, const SdfValueTypeName &,
                 bool, SdfVariability) const)
             &UsdPrim::CreateAttribute,
             (boost::python::arg("name"), boost::python::arg("typeName"), boost::python::arg("custom")=true,
              boost::python::arg("variability")=SdfVariabilityVarying))

        .def("CreateAttribute",
             (UsdAttribute (UsdPrim::*)(
                 const std::vector<std::string> &, const SdfValueTypeName &,
                 bool, SdfVariability) const)
             &UsdPrim::CreateAttribute,
             (boost::python::arg("nameElts"), boost::python::arg("typeName"), boost::python::arg("custom")=true,
              boost::python::arg("variability")=SdfVariabilityVarying))


        .def("GetAttributes", &UsdPrim::GetAttributes,
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("GetAuthoredAttributes", &UsdPrim::GetAuthoredAttributes,
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("GetAttribute", &UsdPrim::GetAttribute, boost::python::arg("attrName"))
        .def("HasAttribute", &UsdPrim::HasAttribute, boost::python::arg("attrName"))

        .def("FindAllAttributeConnectionPaths",
             &_FindAllAttributeConnectionPaths,
             (boost::python::arg("predicate")=boost::python::object(), boost::python::arg("recurseOnSources")=false))
        
        .def("CreateRelationship",
             (UsdRelationship (UsdPrim::*)(const TfToken &, bool) const)
             &UsdPrim::CreateRelationship, (boost::python::arg("name"), boost::python::arg("custom")=true))

        .def("CreateRelationship",
             (UsdRelationship (UsdPrim::*)(const std::vector<std::string> &, bool) const)
             &UsdPrim::CreateRelationship, (boost::python::arg("nameElts"), boost::python::arg("custom")=true))

        .def("GetRelationships", &UsdPrim::GetRelationships,
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("GetAuthoredRelationships", &UsdPrim::GetAuthoredRelationships,
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("GetRelationship", &UsdPrim::GetRelationship, boost::python::arg("relName"))
        .def("HasRelationship", &UsdPrim::HasRelationship, boost::python::arg("relName"))

        .def("FindAllRelationshipTargetPaths",
             &_FindAllRelationshipTargetPaths,
             (boost::python::arg("predicate")=boost::python::object(), boost::python::arg("recurseOnTargets")=false))

        .def("HasPayload", &UsdPrim::HasPayload)
        .def("SetPayload",
             (bool (UsdPrim::*)(const SdfPayload &) const)
             &UsdPrim::SetPayload, (boost::python::arg("payload")))
        .def("SetPayload",
             (bool (UsdPrim::*)(const std::string &, const SdfPath &) const)
             &UsdPrim::SetPayload, (boost::python::arg("assetPath"), boost::python::arg("primPath")))
        .def("SetPayload",
             (bool (UsdPrim::*)(const SdfLayerHandle &, const SdfPath &) const)
             &UsdPrim::SetPayload, (boost::python::arg("layer"), boost::python::arg("primPath")))
        .def("ClearPayload", &UsdPrim::ClearPayload)

        .def("GetPayloads", &UsdPrim::GetPayloads)
        .def("HasAuthoredPayloads", &UsdPrim::HasAuthoredPayloads)

        .def("Load", &UsdPrim::Load, (boost::python::arg("policy")=UsdLoadWithDescendants))
        .def("Unload", &UsdPrim::Unload)

        .def("GetReferences", &UsdPrim::GetReferences)
        .def("HasAuthoredReferences", &UsdPrim::HasAuthoredReferences)

        .def("GetInherits", &UsdPrim::GetInherits)
        .def("HasAuthoredInherits", &UsdPrim::HasAuthoredInherits)

        .def("GetSpecializes", &UsdPrim::GetSpecializes)
        .def("HasAuthoredSpecializes", &UsdPrim::HasAuthoredSpecializes)
    
        .def("RemoveProperty", &UsdPrim::RemoveProperty, boost::python::arg("propName"))
        .def("GetProperty", &UsdPrim::GetProperty, boost::python::arg("propName"))
        .def("HasProperty", &UsdPrim::HasProperty, boost::python::arg("propName"))

        .def("IsInstanceable", &UsdPrim::IsInstanceable)
        .def("SetInstanceable", &UsdPrim::SetInstanceable, boost::python::arg("instanceable"))
        .def("ClearInstanceable", &UsdPrim::ClearInstanceable)
        .def("HasAuthoredInstanceable", &UsdPrim::HasAuthoredInstanceable)

        .def("IsPrototypePath", &UsdPrim::IsPrototypePath, boost::python::arg("path"))
        .staticmethod("IsPrototypePath")
        .def("IsPathInPrototype", &UsdPrim::IsPathInPrototype, boost::python::arg("path"))
        .staticmethod("IsPathInPrototype")

        .def("IsInstance", &UsdPrim::IsInstance)
        .def("IsPrototype", &UsdPrim::IsPrototype)
        .def("IsInPrototype", &UsdPrim::IsInPrototype)
        .def("GetPrototype", &UsdPrim::GetPrototype)

        .def("IsInstanceProxy", &UsdPrim::IsInstanceProxy)
        .def("GetPrimInPrototype", &UsdPrim::GetPrimInPrototype)

        .def("GetPrimAtPath", &UsdPrim::GetPrimAtPath, boost::python::arg("path"))
        .def("GetObjectAtPath", &UsdPrim::GetObjectAtPath, boost::python::arg("path"))
        .def("GetPropertyAtPath", &UsdPrim::GetPropertyAtPath, boost::python::arg("path"))
        .def("GetAttributeAtPath", &UsdPrim::GetAttributeAtPath, boost::python::arg("path"))
        .def("GetRelationshipAtPath",
            &UsdPrim::GetRelationshipAtPath, boost::python::arg("path"))
        .def("GetInstances", &UsdPrim::GetInstances,
                boost::python::return_value_policy<TfPySequenceToList>())

        // Exposed only for testing and debugging.
        .def("_GetSourcePrimIndex", &Usd_PrimGetSourcePrimIndex,
             boost::python::return_value_policy<boost::python::return_by_value>())
        ;
    
    boost::python::to_python_converter<std::vector<UsdPrim>, 
                        TfPySequenceToPython<std::vector<UsdPrim>>>();

    TfPyRegisterStlSequencesFromPython<UsdPrim>();

    // This is wrapped in order to let python call an API that will get through
    // our usual Python API guards to access an invalid prim and throw an
    // exception.
    boost::python::def(
        "_UnsafeGetStageForTesting", &_UnsafeGetStageForTesting);
    
}
