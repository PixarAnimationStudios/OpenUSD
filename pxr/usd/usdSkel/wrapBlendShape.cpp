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
#include "pxr/usd/usdSkel/blendShape.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>

#include <string>


PXR_NAMESPACE_USING_DIRECTIVE

namespace pxrUsdUsdSkelWrapBlendShape {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateOffsetsAttr(UsdSkelBlendShape &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateOffsetsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateNormalOffsetsAttr(UsdSkelBlendShape &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateNormalOffsetsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3fArray), writeSparsely);
}
        
static UsdAttribute
_CreatePointIndicesAttr(UsdSkelBlendShape &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreatePointIndicesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}

static std::string
_Repr(const UsdSkelBlendShape &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdSkel.BlendShape(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdSkelBlendShape()
{
    typedef UsdSkelBlendShape This;

    boost::python::class_<This, boost::python::bases<UsdTyped> >
        cls("BlendShape");

    cls
        .def(boost::python::init<UsdPrim>(boost::python::arg("prim")))
        .def(boost::python::init<UsdSchemaBase const&>(boost::python::arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (boost::python::arg("stage"), boost::python::arg("path")))
        .staticmethod("Get")

        .def("Define", &This::Define, (boost::python::arg("stage"), boost::python::arg("path")))
        .staticmethod("Define")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             boost::python::arg("includeInherited")=true,
             boost::python::return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!boost::python::self)

        
        .def("GetOffsetsAttr",
             &This::GetOffsetsAttr)
        .def("CreateOffsetsAttr",
             &pxrUsdUsdSkelWrapBlendShape::_CreateOffsetsAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetNormalOffsetsAttr",
             &This::GetNormalOffsetsAttr)
        .def("CreateNormalOffsetsAttr",
             &pxrUsdUsdSkelWrapBlendShape::_CreateNormalOffsetsAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetPointIndicesAttr",
             &This::GetPointIndicesAttr)
        .def("CreatePointIndicesAttr",
             &pxrUsdUsdSkelWrapBlendShape::_CreatePointIndicesAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))

        .def("__repr__", pxrUsdUsdSkelWrapBlendShape::_Repr)
    ;

    pxrUsdUsdSkelWrapBlendShape::_CustomWrapCode(cls);
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

namespace pxrUsdUsdSkelWrapBlendShape {


boost::python::tuple
_ValidatePointIndices(TfSpan<const int> pointIndices,
                      size_t numPoints)
{
    std::string reason;
    bool valid = UsdSkelBlendShape::ValidatePointIndices(
        pointIndices, numPoints, &reason);
    return boost::python::make_tuple(valid, reason);
}


WRAP_CUSTOM {

    using This = UsdSkelBlendShape;

    _class
        .def("CreateInbetween", &This::CreateInbetween, boost::python::arg("name"))
        .def("GetInbetween", &This::GetInbetween, boost::python::arg("name"))
        .def("HasInbetween", &This::HasInbetween, boost::python::arg("name"))
        
        .def("GetInbetweens", &This::GetInbetweens,
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("GetAuthoredInbetweens", &This::GetAuthoredInbetweens,
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("ValidatePointIndices", &_ValidatePointIndices,
             (boost::python::arg("pointIndices"), boost::python::arg("numPoints")))
        .staticmethod("ValidatePointIndices")
        ;
}

}
