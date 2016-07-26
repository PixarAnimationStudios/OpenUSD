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
#include "pxr/base/tf/stringUtils.h"

#include <boost/python/def.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/handle.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/type_id.hpp>

#include "pxr/base/tf/pyResultConversions.h"

#include <string>
#include <limits>

using std::string;

using namespace boost::python;

static int DictionaryStrcmp(string const &l, string const &r) {
    TfDictionaryLessThan lt;
    return lt(l, r) ? -1 : (lt(r, l) ? 1 : 0);
}

// Register a from-python conversion that lets clients pass python unicode
// objects to bindings expecting std::strings.  We encode the unicode string as
// utf-8 to produce the std::string.
struct Tf_StdStringFromPythonUnicode
{
    Tf_StdStringFromPythonUnicode() {
        boost::python::converter::registry::insert
            (&convertible, &construct, boost::python::type_id<std::string>());
    }
    static void *convertible(PyObject *obj) {
        return PyUnicode_Check(obj) ? obj : 0;
    }
    static void construct(PyObject *src,
                          boost::python::converter::
                          rvalue_from_python_stage1_data *data) {
        boost::python::handle<> utf8(PyUnicode_AsUTF8String(src));
        std::string utf8String(boost::python::extract<std::string>(utf8.get()));
        void *storage =
            ((boost::python::converter::
              rvalue_from_python_storage<std::string> *)data)->storage.bytes;
        new (storage) std::string(utf8String);
        data->convertible = storage;
    }
};

static unsigned long
_StringToULong(char const *str) {
    bool outOfRange = false;
    unsigned long result = TfStringToULong(str, &outOfRange);
    if (outOfRange)
        TfPyThrowValueError("Out of range.");
    return result;
}

static long
_StringToLong(char const *str) {
    bool outOfRange = false;
    long result = TfStringToLong(str, &outOfRange);
    if (outOfRange)
        TfPyThrowValueError("Out of range.");
    return result;
}

static unsigned long
_GetULongMax() {
    return std::numeric_limits<unsigned long>::max();
}

static long
_GetLongMax() {
    return std::numeric_limits<long>::max();
}

static long
_GetLongMin() {
    return std::numeric_limits<long>::min();
}

void wrapStringUtils() {
    def("StringSplit", TfStringSplit, return_value_policy<TfPySequenceToList>());
    def("DictionaryStrcmp", DictionaryStrcmp);

    def("IsValidIdentifier", TfIsValidIdentifier);
    def("MakeValidIdentifier", TfMakeValidIdentifier);

    def("StringToDouble",
        (double (*)(const std::string &))TfStringToDouble);
    def("StringToLong", _StringToLong);
    def("StringToULong", _StringToULong);

    def("_GetULongMax", _GetULongMax);
    def("_GetLongMax", _GetLongMax);
    def("_GetLongMin", _GetLongMin);
    
    Tf_StdStringFromPythonUnicode();
}
