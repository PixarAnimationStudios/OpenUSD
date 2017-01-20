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
/// \file wrapPrimSpec.cpp

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/pyChildrenProxy.h"
#include "pxr/usd/sdf/pySpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/variantSetSpec.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"

#include <boost/python.hpp>
#include <boost/function.hpp>

using namespace boost::python;

////////////////////////////////////////////////////////////////////////
// Wrappers for constructors and proxy constructors

static
SdfPrimSpecHandle
_NewFromLayer(const SdfLayerHandle& parent,
          const std::string& name, SdfSpecifier spec,
          const std::string& typeName)
{
    return SdfPrimSpec::New(parent, name, spec, typeName);
}

static
SdfPrimSpecHandle
_NewTypelessFromLayer(const SdfLayerHandle& parent,
          const std::string& name, SdfSpecifier spec)
{
    return SdfPrimSpec::New(parent, name, spec);
}

static
SdfPrimSpecHandle
_NewPrim(const SdfPrimSpecHandle& parent,
         const std::string& name, SdfSpecifier spec,
         const std::string& typeName)
{
    return SdfPrimSpec::New(parent, name, spec, typeName);
}

static
SdfPrimSpecHandle
_NewTypelessPrim(const SdfPrimSpecHandle& parent,
                 const std::string& name, SdfSpecifier spec)
{
    return SdfPrimSpec::New(parent, name, spec);
}

typedef SdfPyChildrenProxy<SdfPrimSpec::NameChildrenView> NameChildrenProxy;

static
NameChildrenProxy
_WrapGetNameChildrenProxy(const SdfPrimSpec& prim)
{
    return NameChildrenProxy(prim.GetNameChildren(), "prim");
}

typedef SdfPyChildrenProxy<SdfPrimSpec::PropertySpecView> PropertiesProxy;

static
PropertiesProxy
_WrapGetPropertiesProxy(const SdfPrimSpec& prim)
{
    return PropertiesProxy(prim.GetProperties(), "property");
}

typedef SdfPyChildrenProxy<SdfVariantSetView> VariantSetProxy;

static
VariantSetProxy
_WrapGetVariantSetsProxy(const SdfPrimSpec& prim)
{
    return VariantSetProxy(prim.GetVariantSets());
}

static void
_SetSymmetryArguments(const SdfPrimSpec& self,
                      VtDictionary const &dictionary)
{
    self.GetSymmetryArguments() = dictionary;
}

static void
_SetCustomData(const SdfPrimSpec& self,
               VtDictionary const &dictionary)
{
    self.GetCustomData() = dictionary;
}

static void
_SetAssetInfo(const SdfPrimSpec& self,
              VtDictionary const &dictionary)
{
    self.GetAssetInfo() = dictionary;
}

static void
_SetRelocates(SdfPrimSpec& self, const dict &d)
{
    SdfRelocatesMap reloMap;

    list keys = d.keys();
    int numKeys = len(d);

    for (int i = 0; i < numKeys; i++) {
        SdfPath key = extract<SdfPath>(keys[i]);
        SdfPath val = extract<SdfPath>(d[keys[i]]);

        reloMap[key] = val;
    }

    self.SetRelocates(reloMap);
}

////////////////////////////////////////////////////////////////////////

static
void
_WrapSetName(SdfPrimSpec& self, const std::string& newName)
{
    // Always validate the new name from python.
    self.SetName(newName, true);
}

static
bool
_WrapCanSetName(SdfPrimSpec& self, const std::string& newName)
{
    std::string errStr;
    return self.CanSetName(newName, &errStr);
}

static
std::vector<TfToken>
_ApplyNameChildrenOrder(
    const SdfPrimSpec& self,
    const std::vector<TfToken>& names)
{
    std::vector<TfToken> result = names;
    self.ApplyNameChildrenOrder(&result);
    return result;
}

static
std::vector<TfToken>
_ApplyPropertyOrder(
    const SdfPrimSpec& self,
    const std::vector<TfToken>& names)
{
    std::vector<TfToken> result = names;
    self.ApplyPropertyOrder(&result);
    return result;
}

void
wrapPrimSpec()
{
    def("CreatePrimInLayer", SdfCreatePrimInLayer);

    typedef SdfPrimSpec This;

    // Register python conversions for vector<SdfPrimSpecHandle>
    to_python_converter< SdfPrimSpecHandleVector,
                         TfPySequenceToPython<SdfPrimSpecHandleVector> >();
    TfPyContainerConversions::from_python_sequence<
        SdfPrimSpecHandleVector,
        TfPyContainerConversions::variable_capacity_policy >();

    // Register python conversions for vector<SdfPrimSpecConstHandle>
    to_python_converter< SdfPrimSpecConstHandleVector,
                         TfPySequenceToPython<SdfPrimSpecConstHandleVector> >();
    TfPyContainerConversions::from_python_sequence<
        SdfPrimSpecConstHandleVector,
        TfPyContainerConversions::variable_capacity_policy >();

    // Register python coversions for SdfVariantSets
    typedef SdfVariantSetSpecHandleMap::value_type VSSHVT;
    to_python_converter<VSSHVT,
                        TfPyContainerConversions::to_tuple<VSSHVT> >();
    to_python_converter<SdfVariantSetSpecHandleMap,
                        TfPySequenceToPython<SdfVariantSetSpecHandleMap> >();

    class_<This, SdfHandle<This>, bases<SdfSpec>, boost::noncopyable>
        ("PrimSpec", no_init)
        .def(SdfPySpec())

        .def(SdfMakePySpecConstructor(&::_NewFromLayer))
        .def(SdfMakePySpecConstructor(&::_NewTypelessFromLayer))
        .def(SdfMakePySpecConstructor(&::_NewPrim))
        .def(SdfMakePySpecConstructor(&::_NewTypelessPrim))

        .add_property("name",
            make_function(&This::GetName,
                          return_value_policy<return_by_value>()),
            &::_WrapSetName,
            "The prim's name.")

        .add_property("comment",
            &This::GetComment,
            &This::SetComment,
            "The prim's comment string.")

        .add_property("documentation",
            &This::GetDocumentation,
            &This::SetDocumentation,
            "The prim's documentation string.")

        .add_property("active",
            &This::GetActive,
            &This::SetActive,
            "Whether this prim spec is active.\n"
            "The default value is true.")

        .def("HasActive", &This::HasActive)
        .def("ClearActive", &This::ClearActive)

        .add_property("hidden",
            &This::GetHidden,
            &This::SetHidden,
            "Whether this prim spec will be hidden in browsers.\n"
            "The default value is false.")

        .add_property("kind",
            &This::GetKind,
            &This::SetKind,
            "What kind of model this prim spec represents, if any.\n"
            "The default is an empty string")

        .def("HasKind", &This::HasKind)
        .def("ClearKind", &This::ClearKind)

        .add_property("instanceable",
            &This::GetInstanceable,
            &This::SetInstanceable,
            "Whether this prim spec is flagged as instanceable.\n"
            "The default value is false.")

        .def("HasInstanceable", &This::HasInstanceable)
        .def("ClearInstanceable", &This::ClearInstanceable)

        .add_property("permission",
            &This::GetPermission,
            &This::SetPermission,
            "The prim's permission restriction.\n"
            "The default value is SdfPermissionPublic.")

        .add_property("symmetryFunction",
            &This::GetSymmetryFunction,
            &This::SetSymmetryFunction,
            "The prim's symmetry function.")

        .add_property("symmetryArguments",
            &This::GetSymmetryArguments,
            &::_SetSymmetryArguments,
            "Dictionary with prim symmetry arguments.\n\n"
            "Although this property is marked read-only, you can "
            "modify the contents to add, change, and clear symmetry "
            "arguments.")

        .add_property("symmetricPeer",
            &This::GetSymmetricPeer,
            &This::SetSymmetricPeer,
            "The prims's symmetric peer.")

        .add_property("customData",
            &This::GetCustomData,
            &::_SetCustomData,
            "The custom data for this prim.\n\n"
            "The default value for custom data is an empty dictionary.\n\n"
            "Custom data is for use by plugins or other non-tools supplied \n"
            "extensions that need to be able to store data attached to arbitrary\n"
            "scene objects.  Note that if the only objects you want to store data\n"
            "on are prims, using custom attributes is probably a better choice.\n"
            "But if you need to possibly store this data on attributes or \n"
            "relationships or as annotations on reference arcs, then custom data\n"
            "is an appropriate choice.")

        .add_property("assetInfo",
            &This::GetAssetInfo,
            &::_SetAssetInfo,
            "Returns the asset info dictionary for this prim.\n\n"
            "The default value is an empty dictionary.\n\n"
            "The asset info dictionary is used to annotate prims representing "
            "the root-prims of assets (generally organized as models) with "
            "various data related to asset management. For example, asset "
            "name, root layer identifier, asset version etc.")

        .add_property("specifier",
            &This::GetSpecifier,
            &This::SetSpecifier,
            "The prim's specifier (SpecifierDef or SpecifierOver).\n"
            "The default value is SpecifierOver.")

        .add_property("nameRoot",
            &This::GetNameRoot,
            "The name pseudo-root of this prim.")

        .add_property("nameParent",
            &This::GetNameParent,
            "The name parent of this prim.")

        .add_property("realNameParent",
            &This::GetRealNameParent,
            "The name parent of this prim.")

        .def("GetObjectAtPath",
            &This::GetObjectAtPath,
            "GetObjectAtPath(path) -> object\n\n"
            "path: Path\n\n"
            "Returns a prim or property given its namespace path.\n\n"
            "If path is relative then it will be interpreted as relative "
            "to this prim.  If it is absolute then it will be "
            "interpreted as absolute in this prim's layer. The "
            "return type can be either PrimSpecPtr or "
            "PropertySpecPtr.")
        .def("GetPrimAtPath", &This::GetPrimAtPath)
        .def("GetPropertyAtPath", &This::GetPropertyAtPath)
        .def("RemoveProperty", &This::RemoveProperty)
        .def("GetAttributeAtPath", &This::GetAttributeAtPath)
        .def("GetRelationshipAtPath", &This::GetRelationshipAtPath)
        .def("GetVariantNames", &This::GetVariantNames)

        .add_property("variantSelections",
            &This::GetVariantSelections,
            "Dictionary whose keys are variant set names and whose values are "
            "the variants chosen for each set.\n\n"
            "Although this property is marked read-only, you can "
            "modify the contents to add, change, and clear variants.")

        .add_property("prefix",
            &This::GetPrefix,
            &This::SetPrefix,
            "The prim's prefix.")

        .add_property("prefixSubstitutions",
            &This::GetPrefixSubstitutions,
            &This::SetPrefixSubstitutions,
            "Dictionary of prefix substitutions.")

        .add_property("suffix",
            &This::GetSuffix,
            &This::SetSuffix,
            "The prim's suffix.")

        .add_property("suffixSubstitutions",
            &This::GetSuffixSubstitutions,
            &This::SetSuffixSubstitutions,
            "Dictionary of prefix substitutions.")

        .add_property("variantSetNameList",
            &This::GetVariantSetNameList,
            "A StringListEditor for the names of the variant \n"
            "sets for this prim.\n\n"
            "The list of the names of the variants sets of this prim may be\n"
            "modified with this StringListEditor.\n\n"
            "A StringListEditor may express a list either as an explicit "
            "value or as a set of list editing operations.  See StringListEditor "
            "for more information.\n\n"
            "Although this property is marked as read-only, the returned object "
            "is modifiable.")

        .add_property("variantSets",
            &::_WrapGetVariantSetsProxy,
            "The VariantSetSpecs for this prim indexed by name.\n\n"
            "Although this property is marked as read-only, you can \n"
            "modify the contents to remove variant sets.  New variant sets \n"
            "are created by creating them with the prim as the owner.\n\n"
            "Although this property is marked as read-only, the returned object\n"
            "is modifiable.")

        .add_property("typeName",
            &This::GetTypeName,
            &This::SetTypeName,
            "The type of this prim.")

        .add_property("nameChildren",
            &::_WrapGetNameChildrenProxy,
            "The prim name children of this prim, as an ordered "
            "dictionary.\n\n"
            "Note that although this property is described as being "
            "read-only, you can modify the contents to add, "
            "remove, or reorder children.")

        .add_property("nameChildrenOrder",
            &This::GetNameChildrenOrder,
            &This::SetNameChildrenOrder,
            "Get/set the list of child names for this prim's 'reorder "
            "nameChildren' statement.")

        .add_property("properties",
            &::_WrapGetPropertiesProxy,
            "The properties of this prim, as an ordered dictionary.\n\n"
            "Note that although this property is described as being "
            "read-only, you can modify the contents to add, "
            "remove, or reorder properties.")

        .add_property("attributes",
            &This::GetAttributes,
            "The attributes of this prim, as an ordered dictionary.")

        .add_property("relationships",
            &This::GetRelationships,
            "The relationships of this prim, as an ordered dictionary.")

        .add_property("propertyOrder",
            &This::GetPropertyOrder,
            &This::SetPropertyOrder,
            "Get/set the list of property names for this prim's 'reorder "
            "properties' statement.")

        .add_property("payload",
            &This::GetPayload,
            &This::SetPayload,
            "The payload for this prim")

        .add_property("inheritPathList",
            &This::GetInheritPathList,
            "A PathListEditor for the prim's inherit paths.\n\n"
            "The list of the inherit paths for this prim may be "
            "modified with this PathListEditor.\n\n"
            "A PathListEditor may express a list either as an explicit "
            "value or as a set of list editing operations.  See PathListEditor "
            "for more information.")

        .add_property("specializesList",
            &This::GetSpecializesList,
            "A PathListEditor for the prim's specializes.\n\n"
            "The list of the specializes for this prim may be "
            "modified with this PathListEditor.\n\n"
            "A PathListEditor may express a list either as an explicit "
            "value or as a set of list editing operations.  See PathListEditor "
            "for more information.")

        .add_property("referenceList",
            &This::GetReferenceList,
            "A ReferenceListEditor for the prim's references.\n\n"
            "The list of the references for this prim may be "
            "modified with this ReferenceListEditor.\n\n"
            "A ReferenceListEditor may express a list either as an explicit "
            "value or as a set of list editing operations.  See "
            "ReferenceListEditor for more information.")

        .add_property("hasReferences",
            &This::HasReferences,
            "Returns true if this prim has references set.")

        .add_property("relocates",
            &This::GetRelocates,
            &::_SetRelocates,
            "An editing proxy for the prim's map of relocation paths.\n\n"
            "The map of source-to-target paths specifying namespace "
            "relocation may be set or cleared whole, or individual map "
            "entries may be added, removed, or edited.")

        .def("ClearReferenceList",
            &This::ClearReferenceList,
            "Clears the references for this prim.")

        .def("CanSetName", &::_WrapCanSetName)

        .def("ApplyNameChildrenOrder", &::_ApplyNameChildrenOrder,
             return_value_policy<TfPySequenceToList>())
        .def("ApplyPropertyOrder", &::_ApplyPropertyOrder,
             return_value_policy<TfPySequenceToList>())

        .setattr("ActiveKey", SdfFieldKeys->Active)
        .setattr("AnyTypeToken", SdfTokens->AnyTypeToken)
        .setattr("CommentKey", SdfFieldKeys->Comment)
        .setattr("CustomDataKey", SdfFieldKeys->CustomData)
        .setattr("DocumentationKey", SdfFieldKeys->Documentation)
        .setattr("HiddenKey", SdfFieldKeys->Hidden)
        .setattr("InheritPathsKey", SdfFieldKeys->InheritPaths)
        .setattr("KindKey", SdfFieldKeys->Kind)
        .setattr("PrimOrderKey", SdfFieldKeys->PrimOrder)
        .setattr("PayloadKey", SdfFieldKeys->Payload)
        .setattr("PermissionKey", SdfFieldKeys->Permission)
        .setattr("PrefixKey", SdfFieldKeys->Prefix)
        .setattr("PrefixSubstitutionsKey", SdfFieldKeys->PrefixSubstitutions)
        .setattr("PropertyOrderKey", SdfFieldKeys->PropertyOrder)
        .setattr("ReferencesKey", SdfFieldKeys->References)
        .setattr("RelocatesKey", SdfFieldKeys->Relocates)
        .setattr("SpecializesKey", SdfFieldKeys->Specializes)
        .setattr("SpecifierKey", SdfFieldKeys->Specifier)
        .setattr("SymmetricPeerKey", SdfFieldKeys->SymmetricPeer)
        .setattr("SymmetryArgumentsKey", SdfFieldKeys->SymmetryArguments)
        .setattr("SymmetryFunctionKey", SdfFieldKeys->SymmetryFunction)
        .setattr("TypeNameKey", SdfFieldKeys->TypeName)
        .setattr("VariantSelectionKey", SdfFieldKeys->VariantSelection)
        .setattr("VariantSetNamesKey", SdfFieldKeys->VariantSetNames)
        ;
}
