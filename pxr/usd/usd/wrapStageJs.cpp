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
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/sdf/wrapPathJs.h"
#include "pxr/base/tf/wrapTokenJs.h"
#include "pxr/usd/usd/emscriptenPtrRegistrationHelper.h"
#include <functional>

#include <iostream>
#include <string>
#include <emscripten.h>
#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_REGISTER_SMART_PTR(UsdStage)
EMSCRIPTEN_REGISTER_SMART_PTR(SdfLayer)
EMSCRIPTEN_ENABLE_WEAK_PTR_CAST(SdfLayer)

EM_JS(void, downloadJS, (const char *data, const char *filenamedata), {
  const text = UTF8ToString(data);
  const filename = UTF8ToString(filenamedata);

  let element = document.createElement('a');
  element.setAttribute('href', 'data:text/plain;charset=utf-8,' + encodeURIComponent(text));
  element.setAttribute('download', filename);
  element.style.display = 'none';
  document.body.appendChild(element);
  element.click();

  document.body.removeChild(element);
});

template<typename T>
std::string exportToString(T& self)
{
    std::string output;
    self.ExportToString(&output);
    return output;
}

void download(pxr::UsdStage& self, const std::string& filename)
{
    const std::string& data = exportToString<pxr::UsdStage>(self);
    downloadJS(data.c_str(), filename.c_str());
}

pxr::UsdStageRefPtr CreateNew(const std::string& identifier)
{
    return pxr::UsdStage::CreateNew(identifier, pxr::UsdStage::InitialLoadSet::LoadAll);
}

pxr::UsdStageRefPtr Open(const val& value)
{
    if (value.typeOf().as<std::string>() == "string") {
        return pxr::UsdStage::Open(value.as<std::string>());
    } else {
        return pxr::UsdStage::Open(value.as<pxr::SdfLayerHandle>());
    }   
}

void Export(pxr::UsdStage& self, const std::string &fileName, bool addFileFormatComments)
{
    pxr::SdfLayer::FileFormatArguments arguments;
    self.Export(fileName, addFileFormatComments, arguments);
}

void MyExit()
{
    emscripten_force_exit(0);
}

EMSCRIPTEN_BINDINGS(UsdStage) {
    register_vector<float>("VectorFloat");
    register_vector<pxr::SdfLayerHandle>("VectorSdfLayerHandle");

    enum_<pxr::UsdStage::InitialLoadSet>("InitialLoadSet")
        .value("LoadAll", pxr::UsdStage::InitialLoadSet::LoadAll)
        .value("LoadNone", pxr::UsdStage::InitialLoadSet::LoadNone)
        ;

  using namespace std::placeholders;

  class_<pxr::UsdStage>("UsdStage")
    .smart_ptr_constructor("UsdStageRefPtr", &CreateNew)
    .class_function("CreateNew", &CreateNew)
    .class_function("CreateNew", select_overload<pxr::UsdStageRefPtr(const std::string&, pxr::UsdStage::InitialLoadSet load)>(&pxr::UsdStage::CreateNew))

    .class_function("Open", &Open)
    .class_function("Open", select_overload<pxr::UsdStageRefPtr(const pxr::SdfLayerHandle &layer, pxr::UsdStage::InitialLoadSet)>(&pxr::UsdStage::Open))

    .class_function("Exit", &MyExit)
    .function("ExportToString", &exportToString<pxr::UsdStage>)
    .function("DefinePrim", &pxr::UsdStage::DefinePrim)
    .function("Download", &download)
    .function("Export", &Export)
    .function("GetPrimAtPath", &pxr::UsdStage::GetPrimAtPath)
    .function("SetDefaultPrim", &pxr::UsdStage::SetDefaultPrim)
    .function("OverridePrim", &pxr::UsdStage::OverridePrim)
    .function("GetRootLayer", &pxr::UsdStage::GetRootLayer)
    .function("GetLayerStack", &pxr::UsdStage::GetLayerStack)
    .function("GetStartTimeCode", &pxr::UsdStage::GetStartTimeCode)
    .function("GetEndTimeCode", &pxr::UsdStage::GetEndTimeCode)
    .function("GetTimeCodesPerSecond", &pxr::UsdStage::GetTimeCodesPerSecond)
    ;
}
