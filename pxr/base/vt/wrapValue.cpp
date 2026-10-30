//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/vt/arrayEdit.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/vt/wrapArray.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/inttypes.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <vector>

using std::string;
using std::map;

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(VtValue)
{
    // Allow converting python sequences to VtArray for certain types.  Note
    // that sequences of numeric types are handled in arrayNumpy.{cpp,h}.
    VtRegisterValueCastsFromPythonSequencesToArray<std::string>();
    VtRegisterValueCastsFromPythonSequencesToArray<TfToken>();
}

PXR_NAMESPACE_CLOSE_SCOPE

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// This is only for testing and hitting code coverage.
static string _test_ValueTypeName(VtValue const &val) {
    return val.GetTypeName();
}
static VtValue _test_Ident(VtValue const &val) {
    return val;
}
static std::string _test_Str(VtValue const &val) {
    std::ostringstream stream;
    stream << val;
    return stream.str();
}

} // anonymous namespace 

void wrapValue(nanobind::module_& m)
{
    namespace nb = nanobind;
    m.def("_test_ValueTypeName", _test_ValueTypeName);
    m.def("_test_Ident", _test_Ident);
    m.def("_test_Str", _test_Str);

    nb::class_<Vt_ValueWrapper>(m, "_ValueWrapper")
        .def("__eq__", [](Vt_ValueWrapper &self, const Vt_ValueWrapper &other) {
            return self == other;
        })
        .def("__ne__", [](Vt_ValueWrapper &self, const Vt_ValueWrapper &other) {
            return self != other;
        })
        .def("__str__", &Vt_ValueWrapper::GetAsString)
        .def("__repr__", &Vt_ValueWrapper::GetAsString)
        ;

    static char const *funcDocString = "%s(value) -> _ValueWrapper\n\n"
        "value : %s\n\n"
        "Use this function to specify a value with the explicit C++ type %s "
        "when calling a C++ wrapped function that expects a VtValue. (There are "
        "some C++ types that have no equivalents in Python, such as short.)";

    m.def("Bool", Vt_ValueWrapper::Create<bool>, 
        TfStringPrintf(funcDocString, "Bool","bool","bool").c_str());
    m.def("UChar", Vt_ValueWrapper::Create<unsigned char>, 
        TfStringPrintf(funcDocString, "UChar","unsigned char","unsigned char").c_str());
    m.def("Short", Vt_ValueWrapper::Create<short>, 
        TfStringPrintf(funcDocString, "Short","short","short").c_str());
    m.def("UShort", Vt_ValueWrapper::Create<unsigned short>, 
        TfStringPrintf(funcDocString, "UShort","unsigned short","unsigned short").c_str());
    m.def("Int", Vt_ValueWrapper::Create<int>, 
        TfStringPrintf(funcDocString, "Int","int","int").c_str());
    m.def("UInt", Vt_ValueWrapper::Create<unsigned int>, 
        TfStringPrintf(funcDocString, "UInt","unsigned int","unsigned int").c_str());
    m.def("Long", Vt_ValueWrapper::Create<long>, 
        TfStringPrintf(funcDocString, "Long","long","long").c_str());
    m.def("ULong", Vt_ValueWrapper::Create<unsigned long>, 
        TfStringPrintf(funcDocString, "ULong","unsigned long","unsigned long").c_str());

    m.def("Int64", Vt_ValueWrapper::Create<int64_t>, 
        TfStringPrintf(funcDocString, "Int64","int64_t","int64_t").c_str());
    m.def("UInt64", Vt_ValueWrapper::Create<uint64_t>, 
        TfStringPrintf(funcDocString, "UInt64","uint64_t","uint64_t").c_str());

    m.def("Half", Vt_ValueWrapper::Create<GfHalf>, 
        TfStringPrintf(funcDocString, "Half","half","GfHalf").c_str());
    m.def("Float", Vt_ValueWrapper::Create<float>, 
        TfStringPrintf(funcDocString, "Float","float","float").c_str());
    m.def("Double", Vt_ValueWrapper::Create<double>, 
        TfStringPrintf(funcDocString, "Double","double","double").c_str());
    m.def("Duration", Vt_ValueWrapper::Create<GfDuration>,
        TfStringPrintf(funcDocString,
                       "Duration","duration","GfDuration").c_str());
    m.def("TimeCode", Vt_ValueWrapper::Create<GfTimeCode>,
        TfStringPrintf(funcDocString,
                       "TimeCode","timecode","GfTimeCode").c_str());

    // Since strings and tokens are indistiguishable in Python-land, users need to
    // manually declare when they want a VtValue with a token
    m.def("Token", Vt_ValueWrapper::Create<TfToken>,
        TfStringPrintf(funcDocString, "TfToken","TfToken","TfToken").c_str());

    // Register conversions for VtValue from python, but first make sure that
    // nobody's registered anything before us.

    if (Vt_ValueFromPythonRegistry::HasConversions()) {
        TF_FATAL_ERROR("Vt was not the first library to register VtValue "
                       "from-python conversions!");
    }

    // Register conversion types in reverse order, because the extractor
    // iterates through the registered list backwards.
#define REGISTER_VALUE_FROM_PYTHON(unused, elem) \
    VtValueFromPythonLValue< VT_TYPE(elem) >();
    TF_PP_SEQ_FOR_EACH(REGISTER_VALUE_FROM_PYTHON, ~,
                       VT_ARRAY_EDIT_VALUE_TYPES VT_ARRAY_VALUE_TYPES)
#undef REGISTER_VALUE_FROM_PYTHON

    // Register conversions for each known class value type.
#define REGISTER_VALUE_FROM_PYTHON(unused, elem) \
    VtValueFromPython< VT_TYPE(elem) >();
    TF_PP_SEQ_FOR_EACH(REGISTER_VALUE_FROM_PYTHON, ~,
                       VT_SCALAR_CLASS_VALUE_TYPES VT_NONARRAY_VALUE_TYPES)
#undef REGISTER_VALUE_FROM_PYTHON

    VtValueFromPython<string>();
    VtValueFromPython<double>();
    VtValueFromPython<int>();
    // GfTimeCode and GfDuration are treated like scalars but still need
    // explicit conversions.
    VtValueFromPython<GfDuration>();
    VtValueFromPython<GfTimeCode>();
    // XXX: Disable rvalue conversion of TfType.  It causes a mysterious
    //      crash and we don't need any implicit conversions.
    VtValueFromPythonLValue<TfType>();

    // Register conversions from sequences of VtValues
    TfPyContainerConversions::from_python_sequence<
        std::vector< VtValue >,
        TfPyContainerConversions::variable_capacity_policy >();

    // Conversions for nullary functions returning VtValue.
    TfPyFunctionFromPython<VtValue ()>();
    
}
