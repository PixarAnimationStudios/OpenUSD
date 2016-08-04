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
#include <boost/python.hpp>

#include "pxr/usd/usdGeom/collectionAPI.h"
#include "pxr/usd/usd/conversions.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"

using namespace boost::python;

#include <string>
using std::string;

static TfPyObjWrapper 
_GetTargetFaceCounts(const UsdGeomCollectionAPI &self, const UsdTimeCode &time)
{
    VtIntArray targetFaceCounts;
    self.GetTargetFaceCounts(&targetFaceCounts, time);
    return UsdVtValueToPython(VtValue(targetFaceCounts));
}

static TfPyObjWrapper 
_GetTargetFaceIndices(const UsdGeomCollectionAPI &self, const UsdTimeCode &time)
{
    VtIntArray targetFaceIndices;
    self.GetTargetFaceIndices(&targetFaceIndices, time);
    return UsdVtValueToPython(VtValue(targetFaceIndices));
}

static SdfPathVector 
_GetTargets(const UsdGeomCollectionAPI &self)
{
    SdfPathVector targets;
    self.GetTargets(&targets);
    return targets;
}

static boost::python::tuple 
_Validate(const UsdGeomCollectionAPI &self) {
    std::string reason;
    bool result = self.Validate(&reason);
    return boost::python::make_tuple(result, reason);
}

static std::vector<UsdGeomCollectionAPI> 
_GetCollections1(const UsdPrim &prim) {
    return UsdGeomCollectionAPI::GetCollections(prim);
}

static std::vector<UsdGeomCollectionAPI> 
_GetCollections2(const UsdSchemaBase &schemaObj) {
    return UsdGeomCollectionAPI::GetCollections(schemaObj);
}

static  UsdGeomCollectionAPI 
_Create1(const UsdPrim &prim, const TfToken &name, 
    const SdfPathVector &targets, object targetFaceCounts,
    object targetFaceIndices) 
{
    return UsdGeomCollectionAPI::Create(prim, name, targets, 
        UsdPythonToSdfType(targetFaceCounts, 
                           SdfValueTypeNames->IntArray).Get<VtIntArray>(),
        UsdPythonToSdfType(targetFaceIndices, 
                           SdfValueTypeNames->IntArray).Get<VtIntArray>());
}

static  UsdGeomCollectionAPI 
_Create2(const UsdSchemaBase &schemaObj, 
    const TfToken &name, const SdfPathVector &targets, 
    object targetFaceCounts, object targetFaceIndices)
{
    return UsdGeomCollectionAPI::Create(schemaObj, name, targets, 
        UsdPythonToSdfType(targetFaceCounts, 
                           SdfValueTypeNames->IntArray).Get<VtIntArray>(),
        UsdPythonToSdfType(targetFaceIndices, 
                           SdfValueTypeNames->IntArray).Get<VtIntArray>());
}

static UsdAttribute
_CreateTargetFaceCountsAttr(const UsdGeomCollectionAPI &self, 
                            object defaultValue, 
                            bool writeSparsely)
{
    return self.CreateTargetFaceCountsAttr(
        UsdPythonToSdfType(defaultValue, SdfValueTypeNames->IntArray), 
        writeSparsely);
}

static UsdAttribute
_CreateTargetFaceIndicesAttr(const UsdGeomCollectionAPI &self, 
                             object defaultValue, 
                             bool writeSparsely)
{
    return self.CreateTargetFaceIndicesAttr(
        UsdPythonToSdfType(defaultValue, SdfValueTypeNames->IntArray), 
        writeSparsely);
}

static bool 
_AppendTarget(const UsdGeomCollectionAPI &self, 
              const SdfPath &target, 
              object faceIndices,
              const UsdTimeCode &time=UsdTimeCode::Default())
{
    return self.AppendTarget(target, UsdPythonToSdfType(faceIndices, 
        SdfValueTypeNames->IntArray).Get<VtIntArray>(), time);
}

void 
wrapUsdGeomCollectionAPI()
{
    typedef UsdGeomCollectionAPI This;

    class_<UsdGeomCollectionAPI, bases<UsdSchemaBase> > cls("CollectionAPI");

    cls
        .def(init<const UsdPrim &, const TfToken &>())
        .def(init<const UsdSchemaBase &, const TfToken &>())

        .def(!self)

        .def("GetCollectionName", &This::GetCollectionName)

        .def("IsEmpty", &This::IsEmpty)

        .def("Validate", &_Validate)

        .def("SetTargetFaceCounts", &This::SetTargetFaceCounts,
            (arg("targetFaceCounts"),
             arg("time")=UsdTimeCode::Default()))
        .def("GetTargetFaceCounts", &_GetTargetFaceCounts,
            (arg("time")=UsdTimeCode::Default()))

        .def("SetTargetFaceIndices", &This::SetTargetFaceIndices,
            (arg("targetFaceIndices"),
             arg("time")=UsdTimeCode::Default()))
        .def("GetTargetFaceIndices", &_GetTargetFaceIndices,
            (arg("time")=UsdTimeCode::Default()))

        .def("SetTargets", &This::SetTargets)
        .def("GetTargets", &_GetTargets)

        .def("AppendTarget", &_AppendTarget,
            (arg("target"),
             arg("targetFaceIndices")=VtIntArray(),
             arg("time")=UsdTimeCode::Default()))

        .def("GetTargetFaceCountsAttr", &This::GetTargetFaceCountsAttr)
        .def("CreateTargetFaceCountsAttr", &_CreateTargetFaceCountsAttr,
            (arg("defaultValue")=object(),
             arg("writeSparsely")=false))

        .def("GetTargetFaceIndicesAttr", &This::GetTargetFaceIndicesAttr)
        .def("CreateTargetFaceIndicesAttr", &_CreateTargetFaceIndicesAttr,
            (arg("defaultValue")=object(),
             arg("writeSparsely")=false))

        .def("GetTargetsRel", &This::GetTargetsRel)
        .def("CreateTargetsRel", &This::CreateTargetsRel)

        .def("Create", &_Create1,
            (arg("prim"),
             arg("name"),
             arg("targets")=SdfPathVector(),
             arg("targetFaceCounts")=VtIntArray(), 
             arg("targetFaceIndices")=VtIntArray()))

        .def("Create", &_Create2,
            (arg("schemaObj"),
             arg("name"),
             arg("targets")=SdfPathVector(),
             arg("targetFaceCounts")=VtIntArray(), 
             arg("targetFaceIndices")=VtIntArray()))
            .staticmethod("Create")
        
        .def("GetCollections", _GetCollections1,
            return_value_policy<TfPySequenceToList>())
        .def("GetCollections", _GetCollections2,
            return_value_policy<TfPySequenceToList>())
        .staticmethod("GetCollections")
    ;
}
