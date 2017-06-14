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
#include "pxr/usd/usdGeom/faceSetAPI.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python.hpp>

#include <string>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

using std::string;

static  TfPyObjWrapper 
_GetFaceCounts(const UsdGeomFaceSetAPI &self, const UsdTimeCode &time)
{
    VtIntArray faceCounts;
    self.GetFaceCounts(&faceCounts, time);
    return UsdVtValueToPython(VtValue(faceCounts));
}

static  TfPyObjWrapper 
_GetFaceIndices(const UsdGeomFaceSetAPI &self, const UsdTimeCode &time)
{
    VtIntArray faceIndices;
    self.GetFaceIndices(&faceIndices, time);
    return UsdVtValueToPython(VtValue(faceIndices));
}

static bool 
_SetFaceCounts(const UsdGeomFaceSetAPI &self, object faceCounts, 
                    const UsdTimeCode &time)
{
    return self.SetFaceCounts(UsdPythonToSdfType(faceCounts, 
        SdfValueTypeNames->IntArray).UncheckedGet<VtIntArray>(), time);
}

static bool 
_SetFaceIndices(const UsdGeomFaceSetAPI &self, object faceIndices, 
                    const UsdTimeCode &time)
{
    return self.SetFaceIndices(UsdPythonToSdfType(faceIndices, 
        SdfValueTypeNames->IntArray).UncheckedGet<VtIntArray>(), time);
}

static SdfPathVector 
_GetBindingTargets(const UsdGeomFaceSetAPI &self)
{
    SdfPathVector bindings;
    self.GetBindingTargets(&bindings);
    return bindings;
}

static boost::python::tuple 
_Validate(const UsdGeomFaceSetAPI &self) {
    std::string reason;
    bool result = self.Validate(&reason);
    return boost::python::make_tuple(result, reason);
}

static std::vector<UsdGeomFaceSetAPI> 
_GetFaceSets1(const UsdPrim &prim) {
    return UsdGeomFaceSetAPI::GetFaceSets(prim);
}

static std::vector<UsdGeomFaceSetAPI> 
_GetFaceSets2(const UsdSchemaBase &schemaObj) {
    return UsdGeomFaceSetAPI::GetFaceSets(schemaObj);
}

static UsdGeomFaceSetAPI 
_Create1(const UsdPrim &prim, const TfToken &setName, bool isPartition=true) {
    return UsdGeomFaceSetAPI::Create(prim, setName, isPartition);
}

static UsdGeomFaceSetAPI 
_Create2(const UsdSchemaBase &schemaObj, const TfToken &setName, 
         bool isPartition=true) {
    return UsdGeomFaceSetAPI::Create(schemaObj, setName, isPartition);
}

static UsdAttribute
_CreateIsPartitionAttr(UsdGeomFaceSetAPI &self, object defaultValue, bool writeSparsely)
{
    return self.CreateIsPartitionAttr(
        UsdPythonToSdfType(defaultValue, SdfValueTypeNames->Bool), 
        writeSparsely);
}

static UsdAttribute
_CreateFaceCountsAttr(UsdGeomFaceSetAPI &self, object defaultValue, bool writeSparsely)
{
    return self.CreateFaceCountsAttr(
        UsdPythonToSdfType(defaultValue, SdfValueTypeNames->IntArray), 
        writeSparsely);
}

static UsdAttribute
_CreateFaceIndicesAttr(UsdGeomFaceSetAPI &self, object defaultValue, bool writeSparsely)
{
    return self.CreateFaceIndicesAttr(
        UsdPythonToSdfType(defaultValue, SdfValueTypeNames->IntArray), 
        writeSparsely); 
}

static bool 
_AppendFaceGroup(const UsdGeomFaceSetAPI &self, 
                 object indices,
                 const SdfPath &bindingTarget,
                 const UsdTimeCode &time)
{
    VtValue indicesVal = UsdPythonToSdfType(indices, SdfValueTypeNames->IntArray);
    if (indicesVal.IsHolding<VtIntArray>()) {
        return self.AppendFaceGroup(indicesVal.UncheckedGet<VtIntArray>(),
            bindingTarget, time);
    }
    return false;
}

} // anonymous namespace 

void wrapUsdGeomFaceSetAPI()
{
    typedef UsdGeomFaceSetAPI This;

    class_<UsdGeomFaceSetAPI, bases<UsdSchemaBase> > cls("FaceSetAPI");

    cls
        .def(init<const UsdPrim &, const TfToken &>())
        .def(init<const UsdSchemaBase &, const TfToken &>())

        .def(!self)

        .def("GetFaceSetName", &This::GetFaceSetName)
        
        .def("Validate", &_Validate)

        .def("SetIsPartition", &This::SetIsPartition)
        .def("GetIsPartition", &This::GetIsPartition)

        .def("SetFaceCounts", &_SetFaceCounts,
            (arg("faceCounts"),
             arg("time")=UsdTimeCode::Default()))
        .def("GetFaceCounts", &_GetFaceCounts,
            (arg("time")=UsdTimeCode::Default()))

        .def("SetFaceIndices", &_SetFaceIndices,
            (arg("faceIndices"),
             arg("time")=UsdTimeCode::Default()))
        .def("GetFaceIndices", &_GetFaceIndices,
            (arg("time")=UsdTimeCode::Default()))

        .def("SetBindingTargets", &This::SetBindingTargets)
        .def("GetBindingTargets", &_GetBindingTargets)

        .def("AppendFaceGroup", &_AppendFaceGroup,
            (arg("faceIndices"),
             arg("bindingTarget")=SdfPath(),
             arg("time")=UsdTimeCode::Default()))

        .def("GetIsPartitionAttr", &This::GetIsPartitionAttr)
        .def("CreateIsPartitionAttr", &_CreateIsPartitionAttr,
            (arg("defaultValue")=object(),
             arg("writeSparsely")=false))


        .def("GetFaceCountsAttr", &This::GetFaceCountsAttr)
        .def("CreateFaceCountsAttr", &_CreateFaceCountsAttr,
            (arg("defaultValue")=object(),
             arg("writeSparsely")=false))

        .def("GetFaceIndicesAttr", &This::GetFaceIndicesAttr)
        .def("CreateFaceIndicesAttr", &_CreateFaceIndicesAttr,
            (arg("defaultValue")=object(),
             arg("writeSparsely")=false))

        .def("GetBindingTargetsRel", &This::GetBindingTargetsRel)
        .def("CreateBindingTargetsRel", &This::CreateBindingTargetsRel)

        .def("Create", &_Create1,
            (arg("prim"),
             arg("setName"),
             arg("isPartition")=true))
        .def("Create", &_Create2,
            (arg("schemaObj"),
             arg("setName"),
             arg("isPartition")=true))
            .staticmethod("Create")
        
        .def("GetFaceSets", _GetFaceSets1,
            return_value_policy<TfPySequenceToList>())
        .def("GetFaceSets", _GetFaceSets2,
            return_value_policy<TfPySequenceToList>())
        .staticmethod("GetFaceSets")
    ;
}
