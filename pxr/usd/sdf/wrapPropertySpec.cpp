//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file wrapPropertySpec.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/propertySpec.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/pySpec.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static
void
_WrapSetName(SdfPropertySpec &self,
             const std::string &newName)
{
    // Always validate the new name from python.
    self.SetName(newName, true);
}

static
void
_SetSymmetryArguments(SdfPropertySpec const &self,
                      VtDictionary const &dictionary)
{
    self.GetSymmetryArguments() = dictionary;
}

static
void
_SetCustomData(SdfPropertySpec const &self,
               VtDictionary const &dictionary)
{
    self.GetCustomData() = dictionary;
}

static
void
_SetAssetInfo(SdfPropertySpec const &self,
               VtDictionary const &dictionary)
{
    self.GetAssetInfo() = dictionary;
}

} // anonymous namespace 

void wrapPropertySpec(nb::module_& m)
{
    typedef SdfPropertySpec This;

    nb::class_<This, SdfSpec> cls(m, "PropertySpec");
    cls.def(SdfPyAbstractSpec())

        .def_prop_rw("name",
            [](const This& self) { return self.GetName(); },
            &_WrapSetName,
            "The name of the property.")

        .def_prop_rw("comment",
            &This::GetComment,
            &This::SetComment,
            "A comment describing the property.")

        .def_prop_rw("documentation",
            &This::GetDocumentation,
            &This::SetDocumentation,
            "Documentation for the property.")

        .def_prop_rw("displayGroup",
            &This::GetDisplayGroup,
            &This::SetDisplayGroup,
            "DisplayGroup for the property.")
        .def_prop_rw("displayName",
            &This::GetDisplayName,
            &This::SetDisplayName,
            "DisplayName for the property.")
        .def_prop_rw("prefix",
            &This::GetPrefix,
            &This::SetPrefix,
            "Prefix for the property.")

        .def_prop_ro("variability",
            &This::GetVariability,
            "Returns the variability of the property.\n\n"
            "An attribute's variability may be Varying\n"
            "Uniform, Config or Computed.\n"
            "For an attribute, the default is Varying, for a relationship "
            "the default is Uniform.\n\n"
            "Varying relationships may be directly authored 'animating' target"
            "paths over time.\n"
            "Varying attributes may be directly authored, animated and \n"
            "affected on by Actions.  They are the most flexible.\n\n"
            "Uniform attributes may be authored only with non-animated values\n"
            "(default values).  They cannot be affected by Actions, but they\n"
            "can be connected to other Uniform attributes.\n\n"
            "Config attributes are the same as Uniform except that a Prim\n"
            "can choose to alter its collection of built-in properties based\n"
            "on the values of its Config attributes.\n\n"
            "Computed attributes may not be authored in scene description.\n"
            "Prims determine the values of their Computed attributes through\n"
            "Prim-specific computation.  They may not be connected.")

        .def_prop_rw("hidden",
            &This::GetHidden,
            &This::SetHidden,
            "Whether this property will be hidden in browsers.")

        .def_prop_rw("permission",
            &This::GetPermission,
            &This::SetPermission,
            "The property's permission restriction.")

        .def_prop_rw("custom",
            &This::IsCustom,
            &This::SetCustom,
            "Whether this property spec declares a custom attribute.")

        .def_prop_rw("symmetryFunction",
            &This::GetSymmetryFunction,
            &This::SetSymmetryFunction,
            "The property's symmetry function.")

        .def_prop_rw("symmetryArguments",
            &This::GetSymmetryArguments,
            &_SetSymmetryArguments,
            "Dictionary with property symmetry arguments.\n\n"
            "Although this property is marked read-only, you can "
            "modify the contents to add, change, and clear symmetry arguments.")

        .def_prop_rw("symmetricPeer",
            &This::GetSymmetricPeer,
            &This::SetSymmetricPeer,
            "The property's symmetric peer.")

        .def_prop_rw("customData",
            &This::GetCustomData,
            &_SetCustomData,
            "The property's custom data.\n\n"
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
            "Returns the asset info dictionary for this property.\n\n"
            "The default value is an empty dictionary.\n\n"
            "The asset info dictionary is used to annotate SdfAssetPath-valued "
            "attributes pointing to the root-prims of assets (generally "
            "organized as models) with various data related to asset "
            "management. For example, asset name, root layer identifier, "
            "asset version etc.\n\n"
            "Note: It is only valid to author assetInfo on attributes that "
            "are of type SdfAssetPath.\n")

        .def_prop_ro("owner",
            &This::GetOwner,
            "The owner of this property.  Either a relationship or a prim.")

        .def_prop_rw("default",
            &This::GetDefaultValue,
            &This::SetDefaultValue,
            "The default value of this property.")

        .def("HasDefaultValue", &This::HasDefaultValue)
        .def("ClearDefaultValue", &This::ClearDefaultValue)

        .def_prop_ro("hasOnlyRequiredFields", &This::HasOnlyRequiredFields,
              "Indicates whether this spec has any significant data other \n"
              "than just what is necessary for instantiation.\n\n"
              "This is a less strict version of isInert, returning True if \n"
              "the spec contains as much as the type and name.")
        ;

    // static properties
    cls.attr("AssetInfoKey") = nb::cast(SdfFieldKeys->AssetInfo);
    cls.attr("CommentKey") = nb::cast(SdfFieldKeys->Comment);
    cls.attr("CustomDataKey") = nb::cast(SdfFieldKeys->CustomData);
    cls.attr("CustomKey") = nb::cast(SdfFieldKeys->Custom);
    cls.attr("DisplayGroupKey") = nb::cast(SdfFieldKeys->DisplayGroup);
    cls.attr("DisplayNameKey") = nb::cast(SdfFieldKeys->DisplayName);
    cls.attr("DocumentationKey") = nb::cast(SdfFieldKeys->Documentation);
    cls.attr("HiddenKey") = nb::cast(SdfFieldKeys->Hidden);
    cls.attr("PermissionKey") = nb::cast(SdfFieldKeys->Permission);
    cls.attr("PrefixKey") = nb::cast(SdfFieldKeys->Prefix);
    cls.attr("SymmetricPeerKey") = nb::cast(SdfFieldKeys->SymmetricPeer);
    cls.attr("SymmetryArgumentsKey") = nb::cast(SdfFieldKeys->SymmetryArguments);
    cls.attr("SymmetryFunctionKey") = nb::cast(SdfFieldKeys->SymmetryFunction);
}
