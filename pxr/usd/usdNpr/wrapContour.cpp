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
#include "pxr/usd/usdNpr/contour.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>

#include <string>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateDrawSilhouetteAttr(UsdNprContour &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDrawSilhouetteAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateDrawBoundaryAttr(UsdNprContour &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDrawBoundaryAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateDrawCreaseAttr(UsdNprContour &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDrawCreaseAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateSilhouetteWidthAttr(UsdNprContour &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateSilhouetteWidthAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateBoundaryWidthAttr(UsdNprContour &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateBoundaryWidthAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateCreaseWidthAttr(UsdNprContour &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCreaseWidthAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}

static std::string
_Repr(const UsdNprContour &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdNpr.Contour(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdNprContour()
{
    typedef UsdNprContour This;

    class_<This, bases<UsdGeomGprim> >
        cls("Contour");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("Define", &This::Define, (arg("stage"), arg("path")))
        .staticmethod("Define")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)

        
        .def("GetDrawSilhouetteAttr",
             &This::GetDrawSilhouetteAttr)
        .def("CreateDrawSilhouetteAttr",
             &_CreateDrawSilhouetteAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetDrawBoundaryAttr",
             &This::GetDrawBoundaryAttr)
        .def("CreateDrawBoundaryAttr",
             &_CreateDrawBoundaryAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetDrawCreaseAttr",
             &This::GetDrawCreaseAttr)
        .def("CreateDrawCreaseAttr",
             &_CreateDrawCreaseAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetSilhouetteWidthAttr",
             &This::GetSilhouetteWidthAttr)
        .def("CreateSilhouetteWidthAttr",
             &_CreateSilhouetteWidthAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetBoundaryWidthAttr",
             &This::GetBoundaryWidthAttr)
        .def("CreateBoundaryWidthAttr",
             &_CreateBoundaryWidthAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCreaseWidthAttr",
             &This::GetCreaseWidthAttr)
        .def("CreateCreaseWidthAttr",
             &_CreateCreaseWidthAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        
        .def("GetContourViewPointRel",
             &This::GetContourViewPointRel)
        .def("CreateContourViewPointRel",
             &This::CreateContourViewPointRel)
        
        .def("GetContourSurfacesRel",
             &This::GetContourSurfacesRel)
        .def("CreateContourSurfacesRel",
             &This::CreateContourSurfacesRel)
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

WRAP_CUSTOM {
    _class
        .def("GetContourSurfacesCollectionAPI",
             &UsdNprContour::GetContourSurfacesCollectionAPI)
        ;
}

}
