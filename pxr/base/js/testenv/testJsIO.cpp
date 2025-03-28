//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file testenv/testJsIO.cpp

#include "pxr/pxr.h"
#include "pxr/base/js/json.h"

#include <iostream>
#include <fstream>

PXR_NAMESPACE_USING_DIRECTIVE

int main(int argc, char const *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s inputFile outputFile\n", argv[0]);
        return 1;
    }

    std::ifstream ifs(argv[1]);
    if (!ifs) {
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
        if (!ofs) {
            fprintf(stderr, "Error: failed to open output file '%s'", argv[2]);
            return 2;
        }
        JsWriteToStream(value, ofs);
    }

    return 0;
}
