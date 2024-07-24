//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file testenv/testJsUtils.cpp

#include "pxr/pxr.h"
#include "pxr/base/js/utils.h"
#include "pxr/base/tf/diagnosticLite.h"

PXR_NAMESPACE_USING_DIRECTIVE

int main(int argc, char const *argv[])
{
    JsObject emptyObject;
    TF_AXIOM(!JsFindValue(emptyObject, "key"));
    JsOptionalValue v = JsFindValue(emptyObject, "key", JsValue("value"));
    TF_AXIOM(v);
    TF_AXIOM(v->IsString());
    TF_AXIOM(v->GetString() == "value");

    JsObject object;
    object["key"] = JsValue(42);
    JsOptionalValue jsInt = JsFindValue(object, "key", JsValue(43));
    TF_AXIOM(jsInt);
    TF_AXIOM(jsInt->IsInt());
    TF_AXIOM(jsInt->GetInt() == 42);

    return 0;
}

