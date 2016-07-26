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
///
/// \file testenv/testJsIO.cpp

#include "pxr/base/js/json.h"
#include <iostream>
#include <fstream>

int main(int argc, char const *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s inputFile outputFile\n", argv[0]);
        return 1;
    }

    std::ifstream ifs(argv[1]);
    if (not ifs) {
        fprintf(stderr, "Error: failed to open input file '%s'", argv[1]);
        return 2;
    }

    JsParseError error;
    const JsValue value = JsParseStream(ifs, &error);

    if (value.IsNull()) {
        fprintf(stderr, "Error: parse error at %s:%d:%d: %s\n",
            argv[1], error.line, error.column, error.reason.c_str());
        return 2;
    }

    if (argv[2][0] == '-') {
        JsWriteToStream(value, std::cout);
    } else {
        std::ofstream ofs(argv[2]);
        if (not ofs) {
            fprintf(stderr, "Error: failed to open output file '%s'", argv[2]);
            return 2;
        }
        JsWriteToStream(value, ofs);
    }

    return 0;
}
