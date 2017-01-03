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
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tracelite/trace.h"

#include <boost/python/dict.hpp>
#include <boost/python/def.hpp>
#include <boost/python/list.hpp>
#include <boost/python/to_python_converter.hpp>
#include <boost/python/converter/from_python.hpp>
#include <boost/python/converter/registered.hpp>
#include <boost/python/converter/rvalue_from_python_data.hpp>
#include <boost/python/detail/api_placeholder.hpp>

using namespace boost::python;

// Converter from std::vector<VtValue> to python list
struct VtValueArrayToPython
{
    static PyObject* convert(const std::vector<VtValue> &v)
    {
        // TODO Use result converter. TfPySequenceToList.
        list result;
        TF_FOR_ALL(i, v) {
            object o = TfPyObject(*i);
            result.append(o);
        }
        return incref(result.ptr());
    }
};

// Converter from VtDictionary to python dict.
struct VtDictionaryToPython
{
    static PyObject* convert(const VtDictionary &v)
    {
        TRACE_FUNCTION();

        // TODO Use result converter TfPyMapToDictionary??
        dict result;
        TF_FOR_ALL(i, v) {
            object o = TfPyObject(i->second);
            result.setdefault(i->first, o);
        }
        return incref(result.ptr());
    }
};

static bool
_CanVtValueFromPython(object pVal);

// Converts a python object to a VtValue, with some special behavior.
// If the python object is a dictionary, puts a VtDictionary in the
// result.  If the python object is a list, puts an std::vector<VtValue>
// in the result.  If the python object can be converted to something
// VtValue knows about, does that.  In each of these cases, returns true.
//
// Otherwise, returns false.
static bool _VtValueFromPython(object pVal, VtValue *result) {
    // Try to convert a nested dictionary into a VtDictionary.
    extract<VtDictionary> valDictProxy(pVal);
    if (valDictProxy.check()) {
        if (result) {
            VtDictionary dict = valDictProxy;
            result->Swap(dict);
        }
        return true;
    }

    // Try to convert a nested list into a vector.
    extract<std::vector<VtValue> > valArrayProxy(pVal);
    if (valArrayProxy.check()) {
        if (result) {
            std::vector<VtValue> array = valArrayProxy;
            result->Swap(array);
        }
        return true;
    }

    // Try to convert a value into a VtValue.
    extract<VtValue> valProxy(pVal);
    if (valProxy.check()) {
        VtValue v = valProxy();
        if (v.IsHolding<TfPyObjWrapper>()) {
            return false;
        }
        if (result) {
            result->Swap(v);
        }
        return true;
    }
    return false;
}

// Converter from python dict to VtValueArray.
struct _VtValueArrayFromPython {
    _VtValueArrayFromPython() {
        converter::registry::insert(
            &convertible, &construct, type_id<std::vector<VtValue> >());
    }

    // Returns p if p can convert to an array, NULL otherwise.
    // If result is non-NULL, does the conversion into *result.
    static PyObject *convert(PyObject *p, std::vector<VtValue> *result) {
        extract<list> dProxy(p);
        if (!dProxy.check()) {
            return NULL;
        }
        list d = dProxy();
        int numElts = len(d);

        if (result)
            result->reserve(numElts);
        for (int i = 0; i < numElts; i++) {
            object pVal = d[i];
            if (result) {
                result->push_back(VtValue());
                if (!_VtValueFromPython(pVal, &result->back()))
                    return NULL;
                // Fall through to return p.
            } else {
                // Test for convertibility.
            }
        }
        return p;
    }
    static void *convertible(PyObject *p) {
        return convert(p, NULL);
    }

    static void construct(PyObject* source, converter::
                          rvalue_from_python_stage1_data* data) {
        TfAutoMallocTag2
            tag("Vt", "_VtValueArrayFromPython::construct");
        void* storage = (
            (converter::rvalue_from_python_storage<std::vector<VtValue> >*)
            data)->storage.bytes;
        new (storage) std::vector<VtValue>();
        data->convertible = storage;
        convert(source, (std::vector<VtValue>*)storage);
    }
};

// Converter from python dict to VtDictionary.
struct _VtDictionaryFromPython {
    _VtDictionaryFromPython() {
        converter::registry::insert(
            &convertible, &construct, type_id<VtDictionary>());
    }

    // Returns p if p can convert to a dictionary, NULL otherwise.
    // If result is non-NULL, does the conversion into *result.
    static PyObject *convert(PyObject *p, VtDictionary *result) {
        if (!PyDict_Check(p)) {
            return NULL;
        }

        Py_ssize_t pos = 0;
        PyObject *pyKey = NULL, *pyVal = NULL;
        while (PyDict_Next(p, &pos, &pyKey, &pyVal)) {
            extract<std::string> keyProxy(pyKey);
            if (!keyProxy.check())
                return NULL;
            object pVal(handle<>(borrowed(pyVal)));
            if (result) {
                VtValue &val = (*result)[keyProxy()];
                if (!_VtValueFromPython(pVal, &val))
                    return NULL;
            } else {
                if (!_CanVtValueFromPython(pVal))
                    return NULL;
            }
        }
        return p;
    }
    static void *convertible(PyObject *p) {
        TRACE_FUNCTION();
        return convert(p, NULL);
    }

    static void construct(PyObject* source, converter::
                          rvalue_from_python_stage1_data* data) {
        TRACE_FUNCTION();
        TfAutoMallocTag2
            tag("Vt", "_VtDictionaryFromPython::construct");
        void* storage = (
            (converter::rvalue_from_python_storage<VtDictionary>*)
            data)->storage.bytes;
        new (storage) VtDictionary(0);
        data->convertible = storage;
        convert(source, (VtDictionary*)storage);
    }
};

// Converter from python list to VtValue holding VtValueArray.
struct _VtValueHoldingVtValueArrayFromPython {
    _VtValueHoldingVtValueArrayFromPython() {
        converter::registry::insert(&_VtValueArrayFromPython::convertible,
                                    &construct, type_id<VtValue>());
    }

    static void construct(PyObject* source, converter::
                           rvalue_from_python_stage1_data* data) {
        TfAutoMallocTag2
            tag("Vt", "_VtValueHoldingVtValueArrayFromPython::construct");
        std::vector<VtValue> arr;
        _VtValueArrayFromPython::convert(source, &arr);
        void* storage = (
            (converter::rvalue_from_python_storage<VtValue>*)
            data)->storage.bytes;
        new (storage) VtValue();
        ((VtValue *)storage)->Swap(arr);
        data->convertible = storage;
    }
};

// Converter from python dict to VtValue holding VtDictionary.
struct _VtValueHoldingVtDictionaryFromPython {
    _VtValueHoldingVtDictionaryFromPython() {
        converter::registry::insert(&_VtDictionaryFromPython::convertible,
                                    &construct, type_id<VtValue>());
    }

    static void construct(PyObject* source, converter::
                           rvalue_from_python_stage1_data* data) {
        TfAutoMallocTag2
            tag("Vt", "_VtValueHoldingVtDictionaryFromPython::construct");
        VtDictionary dictionary;
        _VtDictionaryFromPython::convert(source, &dictionary);
        void* storage = (
            (converter::rvalue_from_python_storage<VtValue>*)
            data)->storage.bytes;
        new (storage) VtValue();
        ((VtValue *)storage)->Swap(dictionary);
        data->convertible = storage;
    }
};


static bool
_CanVtValueFromPython(object pVal)
{
    if (_VtDictionaryFromPython::convertible(pVal.ptr()))
        return true;

    if (_VtValueArrayFromPython::convertible(pVal.ptr()))
        return true;

    extract<VtValue> e(pVal);
    return e.check() && !e().IsHolding<TfPyObjWrapper>();
}



static VtDictionary
_ReturnDictionary(VtDictionary const &x) {
    return x;
}


void wrapDictionary()
{

    def("_ReturnDictionary", _ReturnDictionary);

    to_python_converter<VtDictionary, VtDictionaryToPython>();
    to_python_converter<std::vector<VtValue>, VtValueArrayToPython>();
    _VtValueArrayFromPython();
    _VtDictionaryFromPython();
    _VtValueHoldingVtValueArrayFromPython();
    _VtValueHoldingVtDictionaryFromPython();

    def("DictionaryPrettyPrint",
        (std::string(*)(const VtDictionary &)) VtDictionaryPrettyPrint);
}
