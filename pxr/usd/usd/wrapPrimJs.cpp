//
// Copyright 2021 Pixar
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
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/wrapTokenJs.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/references.h"

#include <emscripten/bind.h>
using namespace emscripten;

pxr::TfTokenVector
GetPropertyNames(pxr::UsdPrim& self)
{
    return self.GetPropertyNames();
};

EMSCRIPTEN_BINDINGS(UsdPrim) {
  class_<pxr::UsdPrim>("UsdPrim")
    .function("GetAttribute", &pxr::UsdPrim::GetAttribute)
    .function("GetTypeName", &pxr::UsdPrim::GetTypeName)
    .function("GetAttributes", &pxr::UsdPrim::GetAttributes)
    .function("GetPropertyNames", &GetPropertyNames)
    .function("GetReferences", &pxr::UsdPrim::GetReferences)
    ;
}
