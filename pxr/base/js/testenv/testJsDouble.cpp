//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file testenv/testJsDouble.cpp

#include "pxr/pxr.h"
#include "pxr/base/js/json.h"
#include "pxr/base/tf/diagnosticLite.h"

#include <iostream>
#include <sstream>
#include <limits>

PXR_NAMESPACE_USING_DIRECTIVE

void TestStreamInterface(const double d)
{
    JsValue v(d);
    std::stringstream sstr;
    JsWriteToStream(v, sstr);
    std::cout << sstr.str() << std::endl;
    JsValue v2 = JsParseStream(sstr);
    TF_AXIOM(v2.IsReal());
    if (std::isnan(d)) {
        TF_AXIOM(std::isnan(v2.GetReal()));
    } else {
        TF_AXIOM(v2.GetReal() == d);
    }
}

void TestWriterInterface(const double d)
{
    std::stringstream sstr;
    JsWriter writer(sstr);
    writer.WriteValue(d);
    std::cout << sstr.str() << std::endl;
    JsValue v2 = JsParseStream(sstr);
    TF_AXIOM(v2.IsReal());
    if (std::isnan(d)) {
        TF_AXIOM(std::isnan(v2.GetReal()));
    } else {
        TF_AXIOM(v2.GetReal() == d);
    }
}

int main(int argc, char const *argv[])
{
    const double d = 0.42745098039215684;
    TestStreamInterface(d);
    TestWriterInterface(d);

    using limits = std::numeric_limits<double>;
    TestStreamInterface(limits::infinity());
    TestWriterInterface(limits::infinity());
    TestStreamInterface(-limits::infinity());
    TestWriterInterface(-limits::infinity());
    TestStreamInterface(limits::quiet_NaN());
    TestWriterInterface(limits::quiet_NaN());
}
