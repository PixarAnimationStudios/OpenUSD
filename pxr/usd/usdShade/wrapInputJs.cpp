#include "pxr/base/tf/wrapTokenJs.h"
#include "pxr/usd/sdf/wrapPathJs.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/usd/emscriptenSdfToVtValue.h"
#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/input.h"
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/types.h"

#include <emscripten.h>
#include <emscripten/bind.h>
using namespace emscripten;

bool connectToSource(pxr::UsdShadeInput &self, pxr::UsdShadeShader const &source, pxr::TfToken const &sourceName) {
  return self.ConnectToSource(source.ConnectableAPI(), sourceName, pxr::UsdShadeAttributeType::Output, pxr::SdfValueTypeName());
}

EMSCRIPTEN_BINDINGS(UsdShadeInput) {
  class_<pxr::UsdShadeInput>("UsdShadeInput")
    .function("ConnectToSource", &connectToSource)
    .function("ConnectToSourceInput", select_overload<bool(const pxr::UsdShadeInput&)const>(&pxr::UsdShadeInput::ConnectToSource))
    .function("ConnectToSourcePath", select_overload<bool(const pxr::SdfPath&)const>(&pxr::UsdShadeInput::ConnectToSource))
    .function("Set", &::SetVtValueFromEmscriptenVal<pxr::UsdShadeInput>)
    ;
}
