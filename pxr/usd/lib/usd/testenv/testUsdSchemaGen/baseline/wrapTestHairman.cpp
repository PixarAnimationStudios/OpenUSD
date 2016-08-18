#include "pxr/usd/usdContrived/testHairman.h"

#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/conversions.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>

#include <string>

using namespace boost::python;

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateTempAttr(UsdContrivedTestHairman &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTempAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateGofur_GeomOnHairdensityAttr(UsdContrivedTestHairman &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateGofur_GeomOnHairdensityAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}

void wrapUsdContrivedTestHairman()
{
    typedef UsdContrivedTestHairman This;

    class_<This, bases<UsdSchemaBase> >
        cls("TestHairman");

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

        
        .def("GetTempAttr",
             &This::GetTempAttr)
        .def("CreateTempAttr",
             &_CreateTempAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetGofur_GeomOnHairdensityAttr",
             &This::GetGofur_GeomOnHairdensityAttr)
        .def("CreateGofur_GeomOnHairdensityAttr",
             &_CreateGofur_GeomOnHairdensityAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        
        .def("GetGofur_GeomOnHairdensityRel",
             &This::GetGofur_GeomOnHairdensityRel)
        .def("CreateGofur_GeomOnHairdensityRel",
             &This::CreateGofur_GeomOnHairdensityRel)
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
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

WRAP_CUSTOM {
}
