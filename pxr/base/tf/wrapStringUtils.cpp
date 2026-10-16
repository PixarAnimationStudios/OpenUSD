//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/stringUtils.h"


#include "pxr/base/tf/pyResultConversions.h"

#include <string>
#include <limits>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE


namespace {

static int DictionaryStrcmp(string const &l, string const &r) {
    TfDictionaryLessThan lt;
    return lt(l, r) ? -1 : (lt(r, l) ? 1 : 0);
}

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

} // anonymous namespace 

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

void wrapStringUtils(nanobind::module_& m) {
    m.def("StringSplit", TfStringSplit);
    m.def("DictionaryStrcmp", DictionaryStrcmp);

    m.def("IsValidIdentifier", TfIsValidIdentifier);
    m.def("MakeValidIdentifier", TfMakeValidIdentifier);

    m.def("StringToDouble",
        (double (*)(const std::string &))TfStringToDouble);
    m.def("StringToLong", _StringToLong);
    m.def("StringToULong", _StringToULong);

    m.def("_GetULongMax", _GetULongMax);
    m.def("_GetLongMax", _GetLongMax);
    m.def("_GetLongMin", _GetLongMin);
}
