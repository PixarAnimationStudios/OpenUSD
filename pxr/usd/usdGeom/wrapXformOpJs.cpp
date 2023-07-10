#include "pxr/usd/usdGeom/xformOp.h"
#include "pxr/usd/usd/emscriptenSdfToVtValue.h"
#include <emscripten/bind.h>

using namespace emscripten;

EMSCRIPTEN_BINDINGS(UsdGeomXformOp) {
    class_<pxr::UsdGeomXformOp>("UsdGeomXformOp")
        .function("Set", ::SetVtValueFromEmscriptenVal<pxr::UsdGeomXformOp>)
    ;
}