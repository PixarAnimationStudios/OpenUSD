//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/wrapUtils.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

using std::string;

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static SdfPathVector
_GetTargets(const UsdRelationship &self)
{
    SdfPathVector result;
    self.GetTargets(&result);
    return result;
}

static SdfPathVector
_GetForwardedTargets(const UsdRelationship &self)
{
    SdfPathVector result;
    self.GetForwardedTargets(&result);
    return result;
}

static string
__repr__(const UsdRelationship &self)
{
    if (self) {
        return TfStringPrintf("%s.GetRelationship(%s)",
                              TfPyRepr(self.GetPrim()).c_str(),
                              TfPyRepr(self.GetName()).c_str());
    } else {
        return "invalid " + self.GetDescription();
    }
}

} // anonymous namespace 

void wrapUsdRelationship(nb::module_ &m)
{
    nb::class_<UsdRelationship, UsdProperty>(m, "Relationship")
        .def(nb::init<>())
        .def(Usd_ObjectSubclass())
        .def("__repr__", __repr__)
        .def("AddTarget", &UsdRelationship::AddTarget,
             nb::arg("target"),
             nb::arg("position")=UsdListPositionBackOfPrependList)
        .def("RemoveTarget", &UsdRelationship::RemoveTarget, nb::arg("target"))
        .def("SetTargets", &UsdRelationship::SetTargets, nb::arg("targets"))
        .def("ClearTargets", &UsdRelationship::ClearTargets, nb::arg("removeSpec"))
        .def("GetTargets", _GetTargets)
        .def("GetForwardedTargets", _GetForwardedTargets)
        .def("HasAuthoredTargets", &UsdRelationship::HasAuthoredTargets)
        ;
    TfPyRegisterStlSequencesFromPython<UsdRelationship>();
}
