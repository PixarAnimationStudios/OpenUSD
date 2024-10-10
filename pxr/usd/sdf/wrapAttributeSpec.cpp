//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file wrapAttributeSpec.cpp

#ifndef TF_MAX_ARITY
#define TF_MAX_ARITY 8
#endif

#include "pxr/pxr.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/pyChildrenProxy.h"
#include "pxr/usd/sdf/pySpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/external/boost/python.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

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
std::set<double>
_ListTimeSamples(SdfAttributeSpec const &self)
{
    return self.ListTimeSamples();
}

static
size_t
_GetNumTimeSamples(SdfAttributeSpec const &self)
{
    return self.GetNumTimeSamples();
}

static
VtValue
_QueryTimeSample(SdfAttributeSpec const &self, double time)
{
    VtValue value;
    self.QueryTimeSample(time, &value);
    return value;
}

static
tuple
_GetBracketingTimeSamples(SdfAttributeSpec const &self, double time)
{
    double tLower = 0, tUpper = 0;
    bool found = self.GetBracketingTimeSamples(time, &tLower, &tUpper);
    return pxr_boost::python::make_tuple(found, tLower, tUpper);
}

static
void
_SetTimeSample(SdfAttributeSpec &self,
               double time, const VtValue& value)
{
    self.SetTimeSample(time, value);
}

static
void
_EraseTimeSample(SdfAttributeSpec &self, double time)
{
    self.EraseTimeSample(time);
}

} // anonymous namespace 

void wrapAttributeSpec()
{
    def("JustCreatePrimAttributeInLayer", SdfJustCreatePrimAttributeInLayer,
        (arg("layer"), arg("attrPath"), arg("typeName"),
         arg("variability")=SdfVariabilityVarying, arg("isCustom")=false));

    typedef SdfAttributeSpec This;
    typedef SdfAttributeSpecHandle ThisHandle;

    TfPyContainerConversions::from_python_sequence<
        std::vector< SdfAttributeSpecHandle >,
        TfPyContainerConversions::variable_capacity_policy >();

    // Get function pointers to static New methods.
    ThisHandle (*wrapNewPrimAttr)(const SdfPrimSpecHandle&,
                                  const std::string&,
                                  const SdfValueTypeName&,
                                  SdfVariability,
                                  bool) = &This::New;
                                
    class_<This, SdfHandle<This>, 
           bases<SdfPropertySpec>, noncopyable>
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

	.add_property("allowedTokens",
	    &_WrapGetAllowedTokens,
	    &_WrapSetAllowedTokens,
	    "The allowed value tokens for this property")

        .add_property("colorSpace",
            &This::GetColorSpace,
            &This::SetColorSpace,
            "The color-space in which the attribute value is authored.")

        .def("HasColorSpace", &This::HasColorSpace)
        .def("ClearColorSpace", &This::ClearColorSpace)

        .def("ListTimeSamples", &_ListTimeSamples,
             return_value_policy<TfPySequenceToList>())
        .def("GetNumTimeSamples", &_GetNumTimeSamples)
        .def("GetBracketingTimeSamples",
             &_GetBracketingTimeSamples)
        .def("QueryTimeSample",
             &_QueryTimeSample)
        .def("SetTimeSample", &_SetTimeSample)
        .def("EraseTimeSample", &_EraseTimeSample)

        // property keys
        // XXX DefaultValueKey are actually
        //     implemented on PropertySpec, but are only exposed on
        //     AttributeSpec for some reason
        .setattr("DefaultValueKey", SdfFieldKeys->Default)
        
        .setattr("ConnectionPathsKey", SdfFieldKeys->ConnectionPaths)
        .setattr("DisplayUnitKey", SdfFieldKeys->DisplayUnit)
        ;
}
