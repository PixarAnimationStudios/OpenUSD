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
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/pyChildrenProxy.h"
#include "pxr/usd/sdf/pySpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include "pxr/base/ts/spline.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

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
nb::list
_ListTimeSamples(SdfAttributeSpec const &self)
{
    nb::list result;
    for (double t : self.ListTimeSamples()) {
        result.append(t);
    }
    return result;
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
nb::tuple
_GetBracketingTimeSamples(SdfAttributeSpec const &self, double time)
{
    double tLower = 0, tUpper = 0;
    bool found = self.GetBracketingTimeSamples(time, &tLower, &tUpper);
    return nb::make_tuple(found, tLower, tUpper);
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

void wrapAttributeSpec(nb::module_& m)
{
    m.def("CreatePrimAttributeInLayer", SdfCreatePrimAttributeInLayer,
        nb::arg("layer"), nb::arg("attrPath"), nb::arg("typeName"),
        nb::arg("variability")=SdfVariabilityVarying, nb::arg("isCustom")=false);
    m.def("JustCreatePrimAttributeInLayer", SdfJustCreatePrimAttributeInLayer,
        nb::arg("layer"), nb::arg("attrPath"), nb::arg("typeName"),
        nb::arg("variability")=SdfVariabilityVarying, nb::arg("isCustom")=false);

    typedef SdfAttributeSpec This;
    typedef SdfAttributeSpecHandle ThisHandle;

    // Get function pointers to static New methods.
    ThisHandle (*wrapNewPrimAttr)(const SdfPrimSpecHandle&,
                                  const std::string&,
                                  const SdfValueTypeName&,
                                  SdfVariability,
                                  bool) = &This::New;
                                
    nb::class_<This, SdfPropertySpec> cls(m, "AttributeSpec");
    cls
        .def(SdfPySpec())
        .def(SdfMakePySpecConstructor(wrapNewPrimAttr,
                "__init__(ownerPrimSpec, name, typeName, "
                "variability = Sdf.VariabilityVarying, "
                "declaresCustom = False)\n"
                "ownerPrimSpec : PrimSpec\n"
                "name : string\n"
                "typeName : SdfValueTypeName\n"
                "variability : SdfVariability\n"
                "declaresCustom : bool\n\n"
                "Create a custom attribute spec that is an attribute of "
                "ownerPrimSpec with the given name and type."),
             nb::arg("ownerPrimSpec"),
             nb::arg("name"),
             nb::arg("typeName"),
             nb::arg("variability") = SdfVariabilityVarying,
             nb::arg("declaresCustom") = false)

        // XXX valueType and typeName are actually implemented on PropertySpec,
        //     but are only exposed on AttributeSpec for some reason
        .def_prop_ro("valueType",
            &This::GetValueType,
            "The value type of this attribute.")
        .def_prop_ro("typeName",
            &This::GetTypeName,
            "The typename of this attribute.")

        .def_prop_ro("roleName",
            &This::GetRoleName,
            "The roleName for this attribute's typeName.")

        .def_prop_rw("displayUnit",
            &This::GetDisplayUnit,
            &This::SetDisplayUnit,
            "The display unit for this attribute.")

        .def_prop_ro("connectionPathList",
            &This::GetConnectionPathList,
            "A PathListEditor for the attribute's connection paths.\n\n"
            "The list of the connection paths for this attribute may be "
            "modified with this PathListEditor.\n\n"
            "A PathListEditor may express a list either as an explicit "
            "value or as a set of list editing operations.  See GdListEditor "
            "for more information.")

        .def_prop_rw("allowedTokens",
            &_WrapGetAllowedTokens,
            &_WrapSetAllowedTokens,
            "The allowed value tokens for this property")

        .def_prop_rw("colorSpace",
            &This::GetColorSpace,
            &This::SetColorSpace,
            "The color-space in which the attribute value is authored.")

        .def("HasColorSpace", &This::HasColorSpace)
        .def("ClearColorSpace", &This::ClearColorSpace)

        .def_prop_rw("limits",
            &This::GetLimits,
            &This::SetLimits,
            "The limits dictionary for this attribute.")

        .def("HasLimits", &This::HasLimits)
        .def("ClearLimits", &This::ClearLimits)

        .def_prop_rw("arraySizeConstraint",
            &This::GetArraySizeConstraint,
            &This::SetArraySizeConstraint,
            "The array size constraint value of this attribute.")
        .def("HasArraySizeConstraint", &This::HasArraySizeConstraint)
        .def("ClearArraySizeConstraint", &This::ClearArraySizeConstraint)

        .def("ListTimeSamples", &_ListTimeSamples)
        .def("GetNumTimeSamples", &_GetNumTimeSamples)
        .def("GetBracketingTimeSamples",
             &_GetBracketingTimeSamples)
        .def("QueryTimeSample",
             &_QueryTimeSample)
        .def("SetTimeSample", &_SetTimeSample)
        .def("EraseTimeSample", &_EraseTimeSample)

        .def("HasSpline", &This::HasSpline)
        .def("GetSpline",
             &This::GetSpline,
             nb::rv_policy::copy)
        .def("SetSpline",
             &This::SetSpline, nb::arg("spline"))
        .def("ClearSpline",
             &This::ClearSpline)
        ;

    // property keys
    cls.attr("ArraySizeConstraintKey") = nb::cast(SdfFieldKeys->ArraySizeConstraint);
    cls.attr("ConnectionPathsKey") = nb::cast(SdfFieldKeys->ConnectionPaths);
    // XXX DefaultValueKey are actually
    //     implemented on PropertySpec, but are only exposed on
    //     AttributeSpec for some reason
    cls.attr("DefaultValueKey") = nb::cast(SdfFieldKeys->Default);
    cls.attr("DisplayUnitKey") = nb::cast(SdfFieldKeys->DisplayUnit);
    cls.attr("LimitsKey") = nb::cast(SdfFieldKeys->Limits);
}
