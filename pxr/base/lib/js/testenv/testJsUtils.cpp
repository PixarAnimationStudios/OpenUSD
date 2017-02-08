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

