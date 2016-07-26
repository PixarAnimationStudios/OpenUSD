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
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/references.h"
#include "pxr/usd/usd/inherits.h"
#include "pxr/usd/usd/specializes.h"
#include "pxr/usd/usd/variantSets.h"
#include "pxr/usd/usd/wrapUtils.h"

#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/sdf/payload.h"
#include "pxr/usd/sdf/primSpec.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>

#include <string>
#include <vector>

using std::string;
using std::vector;

using namespace boost::python;

static SdfPayload
_GetPayload(const UsdPrim &self)
{
    SdfPayload result;
    self.GetPayload(&result);
    return result;
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

void wrapUsdPrim()
{
    class_<UsdPrim, bases<UsdObject> >("Prim")
        .def(Usd_ObjectSubclass())
        .def("__repr__", __repr__)

        .def("GetPrimDefinition", &UsdPrim::GetPrimDefinition)
        .def("GetPrimStack", &UsdPrim::GetPrimStack)

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

        .def("IsLoaded", &UsdPrim::IsLoaded)
        .def("IsModel", &UsdPrim::IsModel)
        .def("IsGroup", &UsdPrim::IsGroup)
        .def("IsAbstract", &UsdPrim::IsAbstract)
        .def("IsDefined", &UsdPrim::IsDefined)
        .def("HasDefiningSpecifier", &UsdPrim::HasDefiningSpecifier)

        .def("GetPropertyNames", &UsdPrim::GetPropertyNames,
             return_value_policy<TfPySequenceToList>())
        .def("GetAuthoredPropertyNames", &UsdPrim::GetAuthoredPropertyNames,
             return_value_policy<TfPySequenceToList>())
        
        .def("GetProperties", &UsdPrim::GetProperties,
             return_value_policy<TfPySequenceToList>())
        .def("GetAuthoredProperties", &UsdPrim::GetAuthoredProperties,
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

        .def("GetPropertyOrder", &UsdPrim::GetPropertyOrder,
             return_value_policy<TfPySequenceToList>())

        .def("SetPropertyOrder", &UsdPrim::SetPropertyOrder, arg("order"))

        .def("IsA", &UsdPrim::_IsA, arg("schemaType"))

        .def("GetChild", &UsdPrim::GetChild, arg("name"))

        .def("GetChildren", &UsdPrim::GetChildren,
             return_value_policy<TfPySequenceToList>())
        .def("GetAllChildren", &UsdPrim::GetAllChildren,
             return_value_policy<TfPySequenceToList>())
        .def("GetFilteredChildren", &UsdPrim::GetFilteredChildren,
             arg("predicate"),
             return_value_policy<TfPySequenceToList>())

        .def("GetParent", &UsdPrim::GetParent)
        .def("GetNextSibling", (UsdPrim (UsdPrim::*)() const)
             &UsdPrim::GetNextSibling)
        .def("GetFilteredNextSibling",
             (UsdPrim (UsdPrim::*)(const Usd_PrimFlagsPredicate &) const)
             &UsdPrim::GetFilteredNextSibling)

        .def("HasVariantSets", &UsdPrim::HasVariantSets)
        .def("GetVariantSets", &UsdPrim::GetVariantSets)

        .def("GetVariantSet", &UsdPrim::GetVariantSet)

        .def("GetPrimIndex", &UsdPrim::GetPrimIndex,
             return_value_policy<return_by_value>())

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


        .def("HasPayload", &UsdPrim::HasPayload)
        .def("GetPayload", _GetPayload)
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

        .def("Load", &UsdPrim::Load)
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

        .def("IsInstance", &UsdPrim::IsInstance)
        .def("IsMaster", &UsdPrim::IsMaster)
        .def("IsInMaster", &UsdPrim::IsInMaster)
        .def("GetMaster", &UsdPrim::GetMaster)

        // Exposed only for testing and debugging.
        .def("_GetSourcePrimIndex", &UsdPrim::_GetSourcePrimIndex,
             return_value_policy<return_by_value>())
        ;

    TfPyRegisterStlSequencesFromPython<UsdPrim>();
}



