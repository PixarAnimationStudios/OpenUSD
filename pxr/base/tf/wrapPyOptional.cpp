//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file tf/wrapOptional.cpp

#include "pxr/pxr.h"
#include "pxr/base/tf/pyOptional.h"

#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

void wrapPyOptional() {
    TfPyOptional::python_optional<std::string>();
    TfPyOptional::python_optional<std::vector<std::string> >();
    TfPyOptional::python_optional<double>();
    TfPyOptional::python_optional<float>();
    TfPyOptional::python_optional<long>();
    TfPyOptional::python_optional<unsigned long>();
    TfPyOptional::python_optional<int>();
    TfPyOptional::python_optional<unsigned int>();
    TfPyOptional::python_optional<short>();
    TfPyOptional::python_optional<unsigned short>();
    TfPyOptional::python_optional<char>();
    TfPyOptional::python_optional<unsigned char>();
    TfPyOptional::python_optional<bool>();
}
