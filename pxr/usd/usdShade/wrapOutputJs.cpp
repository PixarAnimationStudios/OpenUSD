#include "pxr/usd/usdShade/output.h"

#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_BINDINGS(UsdShadeOutput) {
  class_<pxr::UsdShadeOutput>("UsdShadeOutput")
    ;
}
