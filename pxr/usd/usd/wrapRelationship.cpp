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
#include "pxr/pxr.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/wrapUtils.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"

#include <boost/python/class.hpp>
#include <boost/python/tuple.hpp>

using std::string;

using namespace boost::python;

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

void wrapUsdRelationship()
{
    class_<UsdRelationship, bases<UsdProperty> >("Relationship")
        .def(Usd_ObjectSubclass())
        .def("__repr__", __repr__)
        .def("AddTarget", &UsdRelationship::AddTarget,
             (arg("target"),
              arg("position")=UsdListPositionBackOfPrependList))
        .def("RemoveTarget", &UsdRelationship::RemoveTarget, arg("target"))
        .def("BlockTargets", &UsdRelationship::BlockTargets)
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
