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
/// \file wrapAttributeSpec.cpp

#define TF_MAX_ARITY 8

#include "pxr/pxr.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/mapperSpec.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/pyChildrenProxy.h"
#include "pxr/usd/sdf/pyMarkerProxy.h"
#include "pxr/usd/sdf/pySpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"

#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_OPEN_SCOPE

static 
std::vector<TfToken> 
_WrapGetAllowedTokens(
    const SdfAttributeSpec& spec)
{
    VtTokenArray tokenArray = spec.GetAllowedTokens();
    return std::vector<TfToken>(tokenArray.begin(), tokenArray.end());
}

static void 
_WrapSetAllowedTokens(
    SdfAttributeSpec& spec,
    const std::vector<TfToken>& tokens)
{
    VtTokenArray tokenArray;
    tokenArray.assign(tokens.begin(), tokens.end());
    spec.SetAllowedTokens(tokenArray);
}

static
SdfPyChildrenProxy<SdfConnectionMappersView>
_WrapGetConnectionMappersProxy(const SdfAttributeSpec& self)
{
    return SdfPyChildrenProxy<SdfConnectionMappersView>(
        self.GetConnectionMappers());
}

template <>
class Sdf_PyMarkerPolicy<SdfAttributeSpec> 
{
public:
    static SdfPathVector GetMarkerPaths(const SdfAttributeSpecHandle& spec)
    {
        return spec->GetConnectionMarkerPaths();
    }

    static std::string GetMarker(const SdfAttributeSpecHandle& spec,
                                 const SdfPath& path)
    {
        return spec->GetConnectionMarker(path);
    }

    static void SetMarker(const SdfAttributeSpecHandle& spec,
                          const SdfPath& path, const std::string& marker)
    {
        spec->SetConnectionMarker(path, marker);
    }

    static void SetMarkers(const SdfAttributeSpecHandle& spec,
                           const std::map<SdfPath, std::string>& markers)
    {
        SdfAttributeSpec::ConnectionMarkerMap m(markers.begin(), markers.end());
        spec->SetConnectionMarkers(m);
    }
};

static
SdfPyMarkerProxy<SdfAttributeSpec>
_WrapGetMarkers(const SdfAttributeSpec& spec)
{
    SdfAttributeSpecHandle attr(spec);
    return SdfPyMarkerProxy<SdfAttributeSpec>(attr);
}

static
void
_WrapSetMarkers(SdfAttributeSpec& attr, const dict& d)
{
    SdfAttributeSpec::ConnectionMarkerMap markers;

    list keys = d.keys();
    size_t numKeys = len(d);

    for (size_t i = 0; i != numKeys; i++) {
        SdfPath key = extract<SdfPath>(keys[i]);
        std::string val = extract<std::string>(d[keys[i]]);

        markers[key] = val;
    }
    attr.SetConnectionMarkers(markers);
}

void wrapAttributeSpec()
{
    typedef SdfAttributeSpec This;
    typedef SdfAttributeSpecHandle ThisHandle;

    // Get function pointers to static New methods.
    ThisHandle (*wrapNewPrimAttr)(const SdfPrimSpecHandle&,
                                  const std::string&,
                                  const SdfValueTypeName&,
                                  SdfVariability,
                                  bool) = &This::New;
                                
    ThisHandle (*wrapNewRelAttr)(const SdfRelationshipSpecHandle&,
                                 const SdfPath&,
                                 const std::string&,
                                 const SdfValueTypeName&,
                                 SdfVariability,
                                 bool) = &This::New;

    class_<This, SdfHandle<This>, 
           bases<SdfPropertySpec>, boost::noncopyable>
        ("AttributeSpec", no_init)
        
        .def(SdfPySpec())
        .def("__unused__",
            SdfMakePySpecConstructor(wrapNewPrimAttr,
                "__init__(ownerPrimSpec, name, typeName, "
                "variability = Sd.VariabilityVarying, "
                "declaresCustom = False)\n"
                "ownerPrimSpec : PrimSpec\n"
                "name : string\n"
                "typeName : SdfValueTypeName\n"
                "variability : SdfVariability\n"
                "declaresCustom : bool\n\n"
                "Create a custom attribute spec that is an attribute of "
                "ownerPrimSpec with the given name and type."),
                (arg("ownerPrimSpec"),
                 arg("name"),
                 arg("typeName"),
                 arg("variability") = SdfVariabilityVarying,
                 arg("declaresCustom") = false))

        .def("__unused__",
            SdfMakePySpecConstructor(wrapNewRelAttr,
                "__init__(ownerRelationshipSpec, targetPath, name, typeName, "
                "variability = Sd.VariabilityVarying, "
                "declaresCustom = False)\n"
                "ownerRelationshipSpec : RelationshipSpec\n"
                "targetPath : Path\n"
                "name : string\n"
                "typeName : SdfValueTypeName\n"
                "variability : SdfVariability\n"
                "declaresCustom : bool\n\n"
                "Create a custom attribute spec that is a relational attribute "
                "of targetPath in ownerRelationshipSpec with the given name "
                "and type."),
                (arg("ownerRelationshipSpec"),
                 arg("targetPath"),
                 arg("name"),
                 arg("typeName"),
                 arg("variability") = SdfVariabilityVarying,
                 arg("declaresCustom") = false))
        
        .def("GetConnectionPathForMapper", &This::GetConnectionPathForMapper)
        .def("ChangeMapperPath", &This::ChangeMapperPath)
        
        // XXX valueType and typeName are actually implemented on PropertySpec,
        //     but are only exposed on AttributeSpec for some reason
        .add_property("valueType",
            &This::GetValueType,
            "The value type of this attribute.")
        .add_property("typeName",
            &This::GetTypeName,
            "The typename of this attribute.")

        .add_property("roleName",
            &This::GetRoleName,
            "The roleName for this attribute's typeName.")

        .add_property("displayUnit",
            &This::GetDisplayUnit,
            &This::SetDisplayUnit,
            "The display unit for this attribute.")

        .add_property("connectionPathList",
            &This::GetConnectionPathList,
            "A PathListEditor for the attribute's connection paths.\n\n"
            "The list of the connection paths for this attribute may be "
            "modified with this PathListEditor.\n\n"
            "A PathListEditor may express a list either as an explicit "
            "value or as a set of list editing operations.  See GdListEditor "
            "for more information.")

        .add_property("connectionMappers",
            &_WrapGetConnectionMappersProxy,
            "The mappers for this attribute in a map proxy keyed by "
            "connection path.\n\n"
            "The returned proxy can be used to remove the mapper for a given "
            "path (using erase) or to access the mappers.  Create a mapper "
            "with this attribute as its owner and the desired connection path "
            "to assign a mapper.")
              
        .add_property("connectionMarkers",
            &_WrapGetMarkers,
            &_WrapSetMarkers,
            "The markers for this attribute in a map proxy keyed by "
            "connection path.\n\n"
            "The returned proxy can be used to set or remove the marker for a "
            "given path or to access the markers.")

	.add_property("allowedTokens",
	    &_WrapGetAllowedTokens,
	    &_WrapSetAllowedTokens,
	    "The allowed value tokens for this property")

        .def("GetConnectionMarker", &This::GetConnectionMarker)
        .def("SetConnectionMarker", &This::SetConnectionMarker)
        .def("ClearConnectionMarker", &This::ClearConnectionMarker)
        .def("GetConnectionMarkerPaths", &This::GetConnectionMarkerPaths)

        // property keys
        // XXX DefaultValueKey are actually
        //     implemented on PropertySpec, but are only exposed on
        //     AttributeSpec for some reason
        .setattr("DefaultValueKey", SdfFieldKeys->Default)
        
        .setattr("ConnectionPathsKey", SdfFieldKeys->ConnectionPaths)
        .setattr("DisplayUnitKey", SdfFieldKeys->DisplayUnit)
        ;
}

PXR_NAMESPACE_CLOSE_SCOPE
