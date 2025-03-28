//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/vt/array.h"
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

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/copy_const_reference.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/object.hpp"
#include "pxr/external/boost/python/operators.hpp"
#include "pxr/external/boost/python/return_arg.hpp"
#include "pxr/external/boost/python/type_id.hpp"
#include "pxr/external/boost/python/str.hpp"

#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <vector>

using std::string;
using std::map;

PXR_NAMESPACE_OPEN_SCOPE

using namespace pxr_boost::python;

TF_REGISTRY_FUNCTION(VtValue)
{
    // Allow converting python sequences to VtArray for certain types.  Note
    // that sequences of numeric types are handled in arrayNumpy.{cpp,h}.
    VtRegisterValueCastsFromPythonSequencesToArray<std::string>();
    VtRegisterValueCastsFromPythonSequencesToArray<TfToken>();
}

TfPyObjWrapper
Vt_GetPythonObjectFromHeldValue(VtValue const &self)
{
    return self._GetPythonObject();
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

// Lets python explicitly pass in values of C++ types which have no python
// equivalents.
struct Vt_ValueWrapper {
    template <class T>
    static Vt_ValueWrapper Create(T a) { return Vt_ValueWrapper(a); }

    template <typename T> explicit Vt_ValueWrapper(T val) : _val(val) {}

    VtValue const &GetValue() const { return _val; }

    bool operator==(const Vt_ValueWrapper &other) {
        return _val == other._val;
    }

    bool operator!=(const Vt_ValueWrapper &other) {
        return _val != other._val;
    }

    std::string GetAsString() {
        return TfStringPrintf(
            "%s(%s)", _val.GetTypeName().c_str(), TfStringify(_val).c_str());
    }
    
  private:
    VtValue _val;
};

struct Vt_ValueToPython {
    static PyObject *convert(VtValue const &val) {
        return incref(Vt_GetPythonObjectFromHeldValue(val).ptr());
    }
};

struct Vt_ValueWrapperFromPython {
    Vt_ValueWrapperFromPython() {
        converter::registry::
            push_back(&_convertible, &_construct,
                      pxr_boost::python::type_id<VtValue>());
    }

  private:

    static void *_convertible(PyObject *obj_ptr) {
        return extract<Vt_ValueWrapper>(obj_ptr).check() ? obj_ptr : NULL;
    }

    static void _construct(PyObject *obj_ptr, converter::
                           rvalue_from_python_stage1_data *data) {
        void *storage = ((converter::rvalue_from_python_storage<VtValue>*)data)
            ->storage.bytes;
        new (storage) VtValue(extract<Vt_ValueWrapper>(obj_ptr)().GetValue());
        data->convertible = storage;
    }
};  


struct Vt_ValueFromPython {

    Vt_ValueFromPython() {
        converter::registry::
            push_back(&_convertible, &_construct,
                      pxr_boost::python::type_id<VtValue>());
    }

  private:

    static void *_convertible(PyObject *obj_ptr) {
        // Can always make a VtValue, but disregard wrappers.  We let implicit
        // conversions handle those.
        if (extract<Vt_ValueWrapper>(obj_ptr).check())
            return 0;
        return obj_ptr;
    }
    
    static void _construct(PyObject *obj_ptr, converter::
                           rvalue_from_python_stage1_data *data) {
        void *storage = ((converter::rvalue_from_python_storage<VtValue>*)data)
            ->storage.bytes;

        // A big typeswitch.  Note that order matters here -- the first
        // one that works is the one that wins.

        // Certain python objects like 'None', bool, numbers and strings are
        // special-cased.
        if (obj_ptr == Py_None) {
            // None -> Empty VtValue.
            new (storage) VtValue();
            data->convertible = storage;
            return;
        }
        if (PyBool_Check(obj_ptr)) {
            // Python bool -> C++ bool.
            new (storage) VtValue(bool(PyLong_AsLong(obj_ptr)));
            data->convertible = storage;
            return;
        }
        if (PyLong_Check(obj_ptr)) {
            // Python long -> either c++ int or long or unsigned long or long
            // long or unsigned long long or fail, depending on range.
            long long val = PyLong_AsLongLong(obj_ptr);
            if (!PyErr_Occurred()) {
                if (std::numeric_limits<int>::min() <= val && 
                    val <= std::numeric_limits<int>::max()) {
                    new (storage) VtValue(int(val));
                } else if (std::numeric_limits<long>::min() <= val && 
                           val <= std::numeric_limits<long>::max()) {
                    new (storage) VtValue(long(val));
                } else {
                    new (storage) VtValue(val);
                }
                data->convertible = storage;
                return;
            } else {
                PyErr_Clear();
                // Try as unsigned long long.
                unsigned long long uval = PyLong_AsUnsignedLongLong(obj_ptr);
                if (!PyErr_Occurred()) {
                    new (storage) VtValue(uval);
                    data->convertible = storage;
                    return;
                } else {
                    PyErr_Clear();
                }
            }
        }
        if (PyFloat_Check(obj_ptr)) {
            // Py float -> c++ double.
            new (storage) VtValue(double(PyFloat_AS_DOUBLE(obj_ptr)));
            data->convertible = storage;
            return;
        }
        if (PyBytes_Check(obj_ptr) || PyUnicode_Check(obj_ptr)) {
            // Py string or unicode -> std::string.
            new (storage) VtValue(std::string(extract<std::string>(obj_ptr)));
            data->convertible = storage;
            return;
        }

        // Attempt a registered conversion via the registry.
        VtValue v = Vt_ValueFromPythonRegistry::Invoke(obj_ptr);

        if (!v.IsEmpty()) {
            new (storage) VtValue(v);
            data->convertible = storage;
            return;
        } else {
            // Fall back to generic python object.
            new (storage)
                VtValue(TfPyObjWrapper(extract<object>(obj_ptr)()));
            data->convertible = storage; 
            return;
        }
    }
};

} // anonymous namespace 

void wrapValue()
{
    def("_test_ValueTypeName", _test_ValueTypeName);
    def("_test_Ident", _test_Ident);
    def("_test_Str", _test_Str);

    to_python_converter<VtValue, Vt_ValueToPython>();
    Vt_ValueFromPython();
    Vt_ValueWrapperFromPython();

    class_<Vt_ValueWrapper>("_ValueWrapper", no_init)
        .def(self == self)
        .def(self != self)
        .def("__str__", &Vt_ValueWrapper::GetAsString)
        .def("__repr__", &Vt_ValueWrapper::GetAsString)
        ;

    static char const *funcDocString = "%s(value) -> _ValueWrapper\n\n"
        "value : %s\n\n"
        "Use this function to specify a value with the explicit C++ type %s "
        "when calling a C++ wrapped function that expects a VtValue. (There are "
        "some C++ types that have no equivalents in Python, such as short.)";

    def("Bool", Vt_ValueWrapper::Create<bool>, 
        TfStringPrintf(funcDocString, "Bool","bool","bool").c_str());
    def("UChar", Vt_ValueWrapper::Create<unsigned char>, 
        TfStringPrintf(funcDocString, "UChar","unsigned char","unsigned char").c_str());
    def("Short", Vt_ValueWrapper::Create<short>, 
        TfStringPrintf(funcDocString, "Short","short","short").c_str());
    def("UShort", Vt_ValueWrapper::Create<unsigned short>, 
        TfStringPrintf(funcDocString, "UShort","unsigned short","unsigned short").c_str());
    def("Int", Vt_ValueWrapper::Create<int>, 
        TfStringPrintf(funcDocString, "Int","int","int").c_str());
    def("UInt", Vt_ValueWrapper::Create<unsigned int>, 
        TfStringPrintf(funcDocString, "UInt","unsigned int","unsigned int").c_str());
    def("Long", Vt_ValueWrapper::Create<long>, 
        TfStringPrintf(funcDocString, "Long","long","long").c_str());
    def("ULong", Vt_ValueWrapper::Create<unsigned long>, 
        TfStringPrintf(funcDocString, "ULong","unsigned long","unsigned long").c_str());

    def("Int64", Vt_ValueWrapper::Create<int64_t>, 
        TfStringPrintf(funcDocString, "Int64","int64_t","int64_t").c_str());
    def("UInt64", Vt_ValueWrapper::Create<uint64_t>, 
        TfStringPrintf(funcDocString, "UInt64","uint64_t","uint64_t").c_str());

    def("Half", Vt_ValueWrapper::Create<GfHalf>, 
        TfStringPrintf(funcDocString, "Half","half","GfHalf").c_str());
    def("Float", Vt_ValueWrapper::Create<float>, 
        TfStringPrintf(funcDocString, "Float","float","float").c_str());
    def("Double", Vt_ValueWrapper::Create<double>, 
        TfStringPrintf(funcDocString, "Double","double","double").c_str());

    // Since strings and tokens are indistiguishable in Python-land, users need to
    // manually declare when they want a VtValue with a token
    def("Token", Vt_ValueWrapper::Create<TfToken>,
        TfStringPrintf(funcDocString, "TfToken","TfToken","TfToken").c_str());

    // Register conversions for VtValue from python, but first make sure that
    // nobody's registered anything before us.

    if (Vt_ValueFromPythonRegistry::HasConversions()) {
        TF_FATAL_ERROR("Vt was not the first library to register VtValue "
                       "from-python conversions!");
    }

    // register conversion types in reverse order, because the extractor
    // iterates through the registered list backwards
    // Repetitively register conversions for each known class value type.
#define REGISTER_VALUE_FROM_PYTHON(unused, elem) \
    VtValueFromPythonLValue< VT_TYPE(elem) >();
    TF_PP_SEQ_FOR_EACH(REGISTER_VALUE_FROM_PYTHON, ~, VT_ARRAY_VALUE_TYPES)
#undef REGISTER_VALUE_FROM_PYTHON

#define REGISTER_VALUE_FROM_PYTHON(unused, elem) \
    VtValueFromPython< VT_TYPE(elem) >();
    TF_PP_SEQ_FOR_EACH(REGISTER_VALUE_FROM_PYTHON, ~,
                       VT_SCALAR_CLASS_VALUE_TYPES VT_NONARRAY_VALUE_TYPES)
#undef REGISTER_VALUE_FROM_PYTHON

    VtValueFromPython<string>();
    VtValueFromPython<double>();
    VtValueFromPython<int>();
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
