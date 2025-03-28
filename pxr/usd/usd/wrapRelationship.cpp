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

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/tuple.hpp"

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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

void wrapUsdRelationship()
{
    class_<UsdRelationship, bases<UsdProperty> >("Relationship")
        .def(Usd_ObjectSubclass())
        .def("__repr__", __repr__)
        .def("AddTarget", &UsdRelationship::AddTarget,
             (arg("target"),
              arg("position")=UsdListPositionBackOfPrependList))
        .def("RemoveTarget", &UsdRelationship::RemoveTarget, arg("target"))
        .def("SetTargets", &UsdRelationship::SetTargets, arg("targets"))
        .def("ClearTargets", &UsdRelationship::ClearTargets, arg("removeSpec"))
        .def("GetTargets", _GetTargets,
             return_value_policy<TfPySequenceToList>())
        .def("GetForwardedTargets", _GetForwardedTargets,
             return_value_policy<TfPySequenceToList>())
        .def("HasAuthoredTargets", &UsdRelationship::HasAuthoredTargets)
        ;
    TfPyRegisterStlSequencesFromPython<UsdRelationship>();
    to_python_converter<std::vector<UsdRelationship>,
                        TfPySequenceToPython<std::vector<UsdRelationship>>>();
}
