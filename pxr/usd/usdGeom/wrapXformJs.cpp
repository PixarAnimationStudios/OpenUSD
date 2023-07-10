#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/sdf/wrapPathJs.h"
#include "pxr/usd/usd/emscriptenPtrRegistrationHelper.h"

#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_ENABLE_WEAK_PTR_CAST(UsdStage)

EMSCRIPTEN_BINDINGS(UsdGeomXform) {
    class_<pxr::UsdGeomXform, base<pxr::UsdGeomXformable>>("UsdGeomXform")
        .class_function("Define", &pxr::UsdGeomXform::Define)
    ;
}