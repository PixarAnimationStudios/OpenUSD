#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/propertySpec.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/usd/emscriptenPtrRegistrationHelper.h"
#include "pxr/base/tf/wrapTokenJs.h"

#include <emscripten/bind.h>
using namespace emscripten;

PXR_NAMESPACE_USING_DIRECTIVE

EMSCRIPTEN_REGISTER_SDF_HANDLE(SdfPropertySpec)
EMSCRIPTEN_REGISTER_SMART_PTR(SdfLayer)
EMSCRIPTEN_ENABLE_WEAK_PTR_CAST(SdfLayer)


val _GetDefault(const pxr::SdfPropertySpec& self) {
    const pxr::VtValue& value = self.GetDefaultValue();
    return value._GetJsVal();
};

typedef std::function<pxr::VtValue (const emscripten::val& jsVal)> SdfToVtValueFunc;
SdfToVtValueFunc* UsdJsToSdfType(pxr::SdfValueTypeName const &targetType);

bool _SetDefault(pxr::SdfPropertySpec& self, val value) {
    if (value == val::undefined()) {
      self.ClearDefaultValue();
      return true;
    }

    SdfToVtValueFunc* sdfToValue = UsdJsToSdfType(self.GetTypeName());
    bool result = false;
    if (sdfToValue != NULL) {
        pxr::VtValue vtValue = (*sdfToValue)(value);
        result = self.SetDefaultValue(vtValue);
    } else {
        std::cerr << "Couldn't find a VtValue mapping for " << self.GetTypeName() << std::endl;
    }
    return result;
}

EMSCRIPTEN_BINDINGS(SdfPropertySpec) {
  class_<pxr::SdfPropertySpec, base<pxr::SdfSpec>>("SdfPropertySpec")
    .property("default", &_GetDefault, &_SetDefault)
    .smart_ptr<pxr::SdfHandle<pxr::SdfPropertySpec>>("EmUsdSdfPropertySpecHandle")
    ;
}
