#include "pxr/base/vt/value.h"
#include "pxr/base/tf/wrapTokenJs.h"
#include "pxr/usd/sdf/wrapPathJs.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/emscriptenPtrRegistrationHelper.h"
#include "pxr/usd/usd/emscriptenSdfToVtValue.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdShade/shader.h"

#include <emscripten.h>
#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_ENABLE_WEAK_PTR_CAST(UsdStage)

pxr::UsdAttribute createIdAttr(pxr::UsdShadeShader &self, const emscripten::val& value) {
    SdfToVtValueFunc* sdfToValue = UsdJsToSdfType(pxr::SdfValueTypeNames->Token);
    pxr::VtValue vtValue = (*sdfToValue)(value);
    return self.CreateIdAttr(vtValue);
}

EMSCRIPTEN_BINDINGS(UsdShadeShader) {
  class_<pxr::UsdShadeShader>("UsdShadeShader")
    .class_function("Define", &pxr::UsdShadeShader::Define)
    .function("CreateIdAttr", &createIdAttr)
    .function("CreateInput", &pxr::UsdShadeShader::CreateInput)
    .function("CreateOutput", &pxr::UsdShadeShader::CreateOutput)
    .function("GetIdAttr", &pxr::UsdShadeShader::GetIdAttr)
    .function("GetInput", &pxr::UsdShadeShader::GetInput)
    .function("GetOutput", &pxr::UsdShadeShader::GetOutput)
    ;
}
