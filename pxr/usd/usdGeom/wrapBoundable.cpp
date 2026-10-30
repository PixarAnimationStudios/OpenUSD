//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/boundable.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/wrapTypeHelpers.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/tuple.h>

namespace nb = nanobind;

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateExtentAttr(UsdGeomBoundable &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateExtentAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3Array), writeSparsely);
}

static std::string
_Repr(const UsdGeomBoundable &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdGeom.Boundable(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdGeomBoundable(nb::module_ &m)
{
    typedef UsdGeomBoundable This;

    nb::class_<This, UsdGeomXformable> cls(m, "Boundable");

    cls
        .def(nb::init<UsdPrim>(), nb::arg("prim")=UsdPrim())
        .def(nb::init<UsdSchemaBase const&>(), nb::arg("schemaObj"))
        .def(TfTypePythonClass())

        .def_static("Get", &This::Get, nb::arg("stage"), nb::arg("path"))

        .def_static("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             nb::arg("includeInherited")=true,
             nb::rv_policy::copy)

        .def_static("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             nb::rv_policy::copy)

        .def("__bool__", [](const This &self) { return bool(self); })

        
        .def("GetExtentAttr",
             &This::GetExtentAttr)
        .def("CreateExtentAttr",
             &_CreateExtentAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)

        .def("__repr__", ::_Repr)
    ;

    _CustomWrapCode(cls);
}

// ===================================================================== //
// Feel free to add custom code below this line, it will be preserved by 
// the code generator.  The entry point for your custom code should look
// minimally like the following:
//
// WRAP_CUSTOM {
//     _class
//         .def("MyCustomMethod", ...)
//     ;
// }
//
// Of course any other ancillary or support code may be provided.
// 
// Just remember to wrap code in the appropriate delimiters:
// 'namespace {', '}'.
//
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

namespace {

static nb::object
_ComputeExtent(
    UsdGeomBoundable &boundable,
    const UsdTimeCode &time)
{
    VtVec3fArray extent;
    if (!boundable.ComputeExtent(time, &extent)) {
        return nb::none();
    }
    return nb::cast(extent);
}

static nb::object
_ComputeExtentFromPlugins(
    const UsdGeomBoundable &boundable,
    const UsdTimeCode &time)
{
    VtVec3fArray extent;
    if (!UsdGeomBoundable::ComputeExtentFromPlugins(boundable,
                                                    time,
                                                    &extent)) {
        return nb::none();
    }
    return nb::cast(extent);
}

static nb::object
_ComputeExtentFromPluginsWithTransform(
    const UsdGeomBoundable &boundable,
    const UsdTimeCode &time,
    const GfMatrix4d &transform)
{
    VtVec3fArray extent;
    if (!UsdGeomBoundable::ComputeExtentFromPlugins(boundable,
                                                    time,
                                                    transform,
                                                    &extent)) {
        return nb::none();
    }
    return nb::cast(extent);
}

WRAP_CUSTOM {
    _class
        .def("ComputeExtent",
             &_ComputeExtent,
             nb::arg("time"))
        .def_static("ComputeExtentFromPlugins", &_ComputeExtentFromPlugins,
             nb::arg("boundable"), nb::arg("time"))
        .def_static("ComputeExtentFromPlugins", &_ComputeExtentFromPluginsWithTransform,
             nb::arg("boundable"), nb::arg("time"), nb::arg("transform"))
    ;
}

} // anonymous namespace 
