//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file wrapPrimSpec.cpp

#include "pxr/pxr.h"
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
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

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
_SetClips(const SdfPrimSpec& self,
          VtDictionary const &dictionary)
{
    self.GetClips() = dictionary;
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

} // anonymous namespace 

void wrapPrimSpec(nb::module_& m)
{
    m.def("CreatePrimInLayer", SdfCreatePrimInLayer);
    m.def("JustCreatePrimInLayer", SdfJustCreatePrimInLayer);

    typedef SdfPrimSpec This;

    nb::class_<This, SdfSpec> cls(m, "PrimSpec");
    cls
        .def(SdfPySpec())

        .def(SdfMakePySpecConstructor(&_NewFromLayer))
        .def(SdfMakePySpecConstructor(&_NewTypelessFromLayer))
        .def(SdfMakePySpecConstructor(&_NewPrim))
        .def(SdfMakePySpecConstructor(&_NewTypelessPrim))

        .def_prop_rw("name",
            [](const This& self) { return self.GetName(); },
            &_WrapSetName,
            "The prim's name.")

        .def_prop_rw("comment",
            &This::GetComment,
            &This::SetComment,
            "The prim's comment string.")

        .def_prop_rw("documentation",
            &This::GetDocumentation,
            &This::SetDocumentation,
            "The prim's documentation string.")

        .def_prop_rw("active",
            &This::GetActive,
            &This::SetActive,
            "Whether this prim spec is active.\n"
            "The default value is true.")

        .def("HasActive", &This::HasActive)
        .def("ClearActive", &This::ClearActive)

        .def_prop_rw("hidden",
            &This::GetHidden,
            &This::SetHidden,
            "Whether this prim spec will be hidden in browsers.\n"
            "The default value is false.")

        .def_prop_rw("kind",
            &This::GetKind,
            &This::SetKind,
            "What kind of model this prim spec represents, if any.\n"
            "The default is an empty string")

        .def("HasKind", &This::HasKind)
        .def("ClearKind", &This::ClearKind)

        .def_prop_rw("instanceable",
            &This::GetInstanceable,
            &This::SetInstanceable,
            "Whether this prim spec is flagged as instanceable.\n"
            "The default value is false.")

        .def("HasInstanceable", &This::HasInstanceable)
        .def("ClearInstanceable", &This::ClearInstanceable)

        .def_prop_rw("permission",
            &This::GetPermission,
            &This::SetPermission,
            "The prim's permission restriction.\n"
            "The default value is SdfPermissionPublic.")

        .def_prop_ro("clipSetsList",
            &This::GetClipSetsList,
            "A StringListEditor for the prim's value clip sets.\n\n"
            "The list of the clip sets for this prim may be "
            "modified with this NameListEditor.\n\n"
            "A NameListEditor may express a list either as an explicit "
            "value or as a set of list editing operations.  See NameListEditor "
            "for more information.")

        .def_prop_rw("clips",
            &This::GetClips,
            &_SetClips,
            "Dictionary with value clips information.")

        .def_prop_ro("hasClipSets",
            &This::HasClipSets,
            "Returns true if this prim has clipSets set.")

        .def_prop_rw("symmetryFunction",
            &This::GetSymmetryFunction,
            &This::SetSymmetryFunction,
            "The prim's symmetry function.")

        .def_prop_rw("symmetryArguments",
            &This::GetSymmetryArguments,
            &_SetSymmetryArguments,
            "Dictionary with prim symmetry arguments.\n\n"
            "Although this property is marked read-only, you can "
            "modify the contents to add, change, and clear symmetry "
            "arguments.")

        .def_prop_rw("symmetricPeer",
            &This::GetSymmetricPeer,
            &This::SetSymmetricPeer,
            "The prims's symmetric peer.")

        .def_prop_rw("customData",
            &This::GetCustomData,
            &_SetCustomData,
            "The custom data for this prim.\n\n"
            "The default value for custom data is an empty dictionary.\n\n"
            "Custom data is for use by plugins or other non-tools supplied \n"
            "extensions that need to be able to store data attached to arbitrary\n"
            "scene objects.  Note that if the only objects you want to store data\n"
            "on are prims, using custom attributes is probably a better choice.\n"
            "But if you need to possibly store this data on attributes or \n"
            "relationships or as annotations on reference arcs, then custom data\n"
            "is an appropriate choice.")

        .def_prop_rw("assetInfo",
            &This::GetAssetInfo,
            &_SetAssetInfo,
            "Returns the asset info dictionary for this prim.\n\n"
            "The default value is an empty dictionary.\n\n"
            "The asset info dictionary is used to annotate prims representing "
            "the root-prims of assets (generally organized as models) with "
            "various data related to asset management. For example, asset "
            "name, root layer identifier, asset version etc.")

        .def_prop_rw("specifier",
            &This::GetSpecifier,
            &This::SetSpecifier,
            "The prim's specifier (SpecifierDef or SpecifierOver).\n"
            "The default value is SpecifierOver.")

        .def_prop_ro("nameRoot",
            &This::GetNameRoot,
            "The name pseudo-root of this prim.")

        .def_prop_ro("nameParent",
            &This::GetNameParent,
            "The name parent of this prim.")

        .def_prop_ro("realNameParent",
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
        .def("BlockVariantSelection", &This::BlockVariantSelection)

        .def_prop_ro("variantSelections",
            &This::GetVariantSelections,
            "Dictionary whose keys are variant set names and whose values are "
            "the variants chosen for each set.\n\n"
            "Although this property is marked read-only, you can "
            "modify the contents to add, change, and clear variants.")

        .def_prop_rw("prefix",
            &This::GetPrefix,
            &This::SetPrefix,
            "The prim's prefix.")

        .def_prop_rw("prefixSubstitutions",
            &This::GetPrefixSubstitutions,
            &This::SetPrefixSubstitutions,
            "Dictionary of prefix substitutions.")

        .def_prop_rw("suffix",
            &This::GetSuffix,
            &This::SetSuffix,
            "The prim's suffix.")

        .def_prop_rw("suffixSubstitutions",
            &This::GetSuffixSubstitutions,
            &This::SetSuffixSubstitutions,
            "Dictionary of prefix substitutions.")

        .def_prop_ro("variantSetNameList",
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

        .def_prop_ro("variantSets",
            &_WrapGetVariantSetsProxy,
            "The VariantSetSpecs for this prim indexed by name.\n\n"
            "Although this property is marked as read-only, you can \n"
            "modify the contents to remove variant sets.  New variant sets \n"
            "are created by creating them with the prim as the owner.\n\n"
            "Although this property is marked as read-only, the returned object\n"
            "is modifiable.")

        .def_prop_rw("typeName",
            &This::GetTypeName,
            &This::SetTypeName,
            "The type of this prim.")

        .def_prop_ro("nameChildren",
            &_WrapGetNameChildrenProxy,
            "The prim name children of this prim, as an ordered "
            "dictionary.\n\n"
            "Note that although this property is described as being "
            "read-only, you can modify the contents to add, "
            "remove, or reorder children.")

        .def_prop_rw("nameChildrenOrder",
            &This::GetNameChildrenOrder,
            &This::SetNameChildrenOrder,
            "Get/set the list of child names for this prim's 'reorder "
            "nameChildren' statement.")

        .def_prop_ro("properties",
            &_WrapGetPropertiesProxy,
            "The properties of this prim, as an ordered dictionary.\n\n"
            "Note that although this property is described as being "
            "read-only, you can modify the contents to add, "
            "remove, or reorder properties.")

        .def_prop_ro("attributes",
            &This::GetAttributes,
            "The attributes of this prim, as an ordered dictionary.")

        .def_prop_ro("relationships",
            &This::GetRelationships,
            "The relationships of this prim, as an ordered dictionary.")

        .def_prop_rw("propertyOrder",
            &This::GetPropertyOrder,
            &This::SetPropertyOrder,
            "Get/set the list of property names for this prim's 'reorder "
            "properties' statement.")

        .def_prop_ro("inheritPathList",
            &This::GetInheritPathList,
            "A PathListEditor for the prim's inherit paths.\n\n"
            "The list of the inherit paths for this prim may be "
            "modified with this PathListEditor.\n\n"
            "A PathListEditor may express a list either as an explicit "
            "value or as a set of list editing operations.  See PathListEditor "
            "for more information.")

        .def_prop_ro("specializesList",
            &This::GetSpecializesList,
            "A PathListEditor for the prim's specializes.\n\n"
            "The list of the specializes for this prim may be "
            "modified with this PathListEditor.\n\n"
            "A PathListEditor may express a list either as an explicit "
            "value or as a set of list editing operations.  See PathListEditor "
            "for more information.")

        .def_prop_ro("referenceList",
            &This::GetReferenceList,
            "A ReferenceListEditor for the prim's references.\n\n"
            "The list of the references for this prim may be "
            "modified with this ReferenceListEditor.\n\n"
            "A ReferenceListEditor may express a list either as an explicit "
            "value or as a set of list editing operations.  See "
            "ReferenceListEditor for more information.")

        .def_prop_ro("payloadList",
            &This::GetPayloadList,
            "A PayloadListEditor for the prim's payloads.\n\n"
            "The list of the payloads for this prim may be "
            "modified with this PayloadListEditor.\n\n"
            "A PayloadListEditor may express a list either as an explicit "
            "value or as a set of list editing operations.  See "
            "PayloadListEditor for more information.")

        .def_prop_ro("hasReferences",
            &This::HasReferences,
            "Returns true if this prim has references set.")

        .def_prop_ro("hasPayloads",
            &This::HasPayloads,
            "Returns true if this prim has payloads set.")

        .def_prop_ro("hasInheritPaths",
            &This::HasInheritPaths,
            "Returns true if this prim has inherits set.")

        .def_prop_ro("hasSpecializes",
            &This::HasSpecializes,
            "Returns true if this prim has specializes set.")

        .def("ClearReferenceList",
            &This::ClearReferenceList,
            "Clears the references for this prim.")

        .def("ClearPayloadList",
            &This::ClearPayloadList,
            "Clears the payloads for this prim.")

        .def("CanSetName", &_WrapCanSetName)

        .def("ApplyNameChildrenOrder", &_ApplyNameChildrenOrder)
        .def("ApplyPropertyOrder", &_ApplyPropertyOrder)
        ;

    cls.attr("ActiveKey") = nb::cast(SdfFieldKeys->Active);
    cls.attr("AnyTypeToken") = nb::cast(SdfTokens->AnyTypeToken);
    cls.attr("CommentKey") = nb::cast(SdfFieldKeys->Comment);
    cls.attr("CustomDataKey") = nb::cast(SdfFieldKeys->CustomData);
    cls.attr("DisplayName") = nb::cast(SdfFieldKeys->DisplayName);
    cls.attr("DocumentationKey") = nb::cast(SdfFieldKeys->Documentation);
    cls.attr("HiddenKey") = nb::cast(SdfFieldKeys->Hidden);
    cls.attr("InheritPathsKey") = nb::cast(SdfFieldKeys->InheritPaths);
    cls.attr("KindKey") = nb::cast(SdfFieldKeys->Kind);
    cls.attr("PrimOrderKey") = nb::cast(SdfFieldKeys->PrimOrder);
    cls.attr("PayloadKey") = nb::cast(SdfFieldKeys->Payload);
    cls.attr("PermissionKey") = nb::cast(SdfFieldKeys->Permission);
    cls.attr("PrefixKey") = nb::cast(SdfFieldKeys->Prefix);
    cls.attr("PrefixSubstitutionsKey") = nb::cast(SdfFieldKeys->PrefixSubstitutions);
    cls.attr("PropertyOrderKey") = nb::cast(SdfFieldKeys->PropertyOrder);
    cls.attr("ReferencesKey") = nb::cast(SdfFieldKeys->References);
    cls.attr("SpecializesKey") = nb::cast(SdfFieldKeys->Specializes);
    cls.attr("SpecifierKey") = nb::cast(SdfFieldKeys->Specifier);
    cls.attr("SymmetricPeerKey") = nb::cast(SdfFieldKeys->SymmetricPeer);
    cls.attr("SymmetryArgumentsKey") = nb::cast(SdfFieldKeys->SymmetryArguments);
    cls.attr("SymmetryFunctionKey") = nb::cast(SdfFieldKeys->SymmetryFunction);
    cls.attr("TypeNameKey") = nb::cast(SdfFieldKeys->TypeName);
    cls.attr("VariantSelectionKey") = nb::cast(SdfFieldKeys->VariantSelection);
    cls.attr("VariantSetNamesKey") = nb::cast(SdfFieldKeys->VariantSetNames);
}
