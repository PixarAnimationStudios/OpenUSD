#include "pxr/usd/usdLux/distantLight.h"
#include "pxr/usd/sdf/wrapPathJs.h"
#include "pxr/usd/usd/emscriptenPtrRegistrationHelper.h"

#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_ENABLE_WEAK_PTR_CAST(UsdStage)

EMSCRIPTEN_BINDINGS(UsdLuxDistantLight) 
{
  class_<pxr::UsdLuxDistantLight>("UsdLuxDistantLight")
    .class_function("Define", &pxr::UsdLuxDistantLight::Define)
    ;
}

