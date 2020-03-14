//
// Copyright 2018 Pixar
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
///
/// \file testenv/testJsDouble.cpp

#include "pxr/pxr.h"
#include "pxr/base/js/json.h"
#include "pxr/base/tf/diagnosticLite.h"

#include <iostream>
#include <sstream>

PXR_NAMESPACE_USING_DIRECTIVE

void TestStreamInterface(const double d)
{
    JsValue v(d);
    std::stringstream sstr;
    JsWriteToStream(v, sstr);
    std::cout << sstr.str() << std::endl;
    JsValue v2 = JsParseStream(sstr);
    TF_AXIOM(v2.IsReal());
    TF_AXIOM(v2.GetReal() == d);
}

void TestWriterInterface(const double d)
{
    std::stringstream sstr;
    JsWriter writer(sstr);
    writer.WriteValue(d);
    std::cout << sstr.str() << std::endl;
    JsValue v2 = JsParseStream(sstr);
    TF_AXIOM(v2.IsReal());
    TF_AXIOM(v2.GetReal() == d);
}

int main(int argc, char const *argv[])
{
    const double d = 0.42745098039215684;
    TestStreamInterface(d);
    TestWriterInterface(d);
}