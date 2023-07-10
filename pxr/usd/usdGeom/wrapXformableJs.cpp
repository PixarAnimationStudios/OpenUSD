#include "pxr/usd/usdGeom/xformable.h"
#include <emscripten/bind.h>

#include <vector>

using namespace emscripten;


pxr::UsdGeomXformOp AddScaleOp(pxr::UsdGeomXformable& self) {
    return self.AddScaleOp();
}

pxr::UsdGeomXformOp AddTranslateOp(pxr::UsdGeomXformable& self) {
    return self.AddTranslateOp();
}

bool SetXformOpOrder(pxr::UsdGeomXformable& self, std::vector<pxr::UsdGeomXformOp> const &orderedXformOps) {
    return self.SetXformOpOrder(orderedXformOps);
}

EMSCRIPTEN_REGISTER_VECTOR_TO_ARRAY_CONVERSION(pxr::UsdGeomXformOp)
EMSCRIPTEN_REGISTER_TYPE(std::vector<pxr::UsdGeomXformOp>)

EMSCRIPTEN_BINDINGS(UsdGeomXformable) {
    class_<pxr::UsdGeomXformable>("UsdGeomXformable")
        .constructor<const pxr::UsdPrim &>()
        .function("AddScaleOp", AddScaleOp)
        .function("AddTranslateOp", AddTranslateOp)
        .function("SetXformOpOrder", SetXformOpOrder)
    ;
}