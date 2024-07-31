//
// Copyright 2024 Pixar
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
#include "pxr/pxr.h"
#include "pxr/usd/sdf/transcodeUtils.h"

#include <boost/python.hpp>

#include <memory>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// This is expected to be dropped when using pybind11
std::optional<std::string>
Py_SdfEncodeIdentifier(const std::string& inputString, const SdfTranscodeFormat format)
{
    return SdfEncodeIdentifier(inputString, format);
}

// This is expected to be dropped when using pybind11
std::optional<std::string>
Py_SdfDecodeIdentifier(const std::string& inputString)
{
    return SdfDecodeIdentifier(inputString);
}

} // anonymous

void
wrapTranscodeUtils()
{
    enum_<SdfTranscodeFormat>("TranscodeFormat")
        .value("ASCII", SdfTranscodeFormat::ASCII)
        .value("UNICODE_XID", SdfTranscodeFormat::UNICODE_XID)
    ;

    def("EncodeIdentifier", 
        &Py_SdfEncodeIdentifier,
        (arg("inputString"), arg("format")))
    ;

    def("DecodeIdentifier", 
        &Py_SdfDecodeIdentifier,
        (arg("inputString")))
    ;
}
