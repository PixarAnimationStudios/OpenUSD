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
#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/pyObjWrapper.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/vt/streamOut.h"

#include <double-conversion/double-conversion.h>
#include <double-conversion/utils.h>

#include <iostream>

std::ostream &
Vt_StreamOutGeneric(std::type_info const &type,
                    void const *addr,
                    std::ostream &stream)
{
    return stream <<
        TfStringPrintf("<'%s' @ %p>", ArchGetDemangled(type).c_str(), addr);
}

std::ostream &
VtStreamOut(bool const &val, std::ostream &stream)
{
    return stream << static_cast<int>(val);
}

std::ostream &
VtStreamOut(char const &val, std::ostream &stream)
{
    return stream << static_cast<int>(val);
}

std::ostream &
VtStreamOut(unsigned char const &val, std::ostream &stream)
{
    return stream << static_cast<unsigned int>(val);
}

std::ostream &
VtStreamOut(signed char const &val, std::ostream &stream)
{
    return stream << static_cast<int>(val);
}

static double_conversion::DoubleToStringConverter const &
_GetDoubleToStringConverter() {
    static double_conversion::DoubleToStringConverter
        converter(double_conversion::DoubleToStringConverter::NO_FLAGS,
                  "inf", 
                  "nan",
                  'e',
                  /* decimal_in_shortest_low */ -6,
                  /* deciaml_in_shortest_high */ 21,
                  /* max_leading_padding_zeroes_in_precision_mode */ 0,
                  /* max_trailing_padding_zeroes_in_precision_mode */ 0);
    return converter;
}

std::ostream &
VtStreamOut(float const &val, std::ostream &stream)
{
    double_conversion::DoubleToStringConverter const &conv =
        _GetDoubleToStringConverter();
    static const int bufSize = 128;
    char buf[bufSize];
    double_conversion::StringBuilder builder(buf, bufSize);
    // This should only fail if we provide an insufficient buffer.
    TF_VERIFY(conv.ToShortestSingle(val, &builder), "double_conversion failed");
    return stream << builder.Finalize();
}

std::ostream &
VtStreamOut(double const &val, std::ostream &stream)
{
    double_conversion::DoubleToStringConverter const &conv =
        _GetDoubleToStringConverter();
    static const int bufSize = 128;
    char buf[bufSize];
    double_conversion::StringBuilder builder(buf, bufSize);
    // This should only fail if we provide an insufficient buffer.
    TF_VERIFY(conv.ToShortest(val, &builder), "double_conversion failed");
    return stream << builder.Finalize();
}

std::ostream &
VtStreamOut(TfPyObjWrapper const &obj, std::ostream &stream)
{
    return stream << TfPyObjectRepr(obj.Get());
}

namespace {

void
_StreamArray(
    VtStreamOutIterator* i,
    size_t size,
    const Vt_Reserved* reserved,
    std::ostream& out)
{
    out << '[';
    for (size_t j = 0; j != size; ++j) {
        if (j) {
            out << ", ";
        }
        i->Next(out);
    }
    out << ']';
}

}

void (*_vtStreamArray)(VtStreamOutIterator*, size_t,
                       const Vt_Reserved*, std::ostream&) = _StreamArray;

VtStreamOutIterator::~VtStreamOutIterator() { }

void
VtStreamOutArray(
    VtStreamOutIterator* i,
    size_t size,
    const Vt_Reserved* reserved,
    std::ostream& out)
{
    _vtStreamArray(i, size, reserved, out);
}
