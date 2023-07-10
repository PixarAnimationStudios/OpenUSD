#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/spec.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/usd/emscriptenPtrRegistrationHelper.h"
#include "pxr/usd/sdf/wrapPathJs.h"
#include "pxr/base/tf/wrapTokenJs.h"

#include <iostream>

#include <emscripten/bind.h>
using namespace emscripten;

PXR_NAMESPACE_USING_DIRECTIVE

EMSCRIPTEN_REGISTER_SDF_HANDLE(SdfSpec)
EMSCRIPTEN_REGISTER_SMART_PTR(SdfLayer)
EMSCRIPTEN_ENABLE_WEAK_PTR_CAST(SdfLayer)

typedef std::function<pxr::VtValue (const emscripten::val& jsVal)> SdfToVtValueFunc;
SdfToVtValueFunc* UsdJsToSdfType(const std::string &targetType);

void _SetInfo(pxr::SdfSpec& self, const TfToken &name, const val& value) {
    try {
      VtValue fallback;
      if (!self.GetSchema().IsRegistered(name, &fallback)) {
          std::cerr <<  "Invalid info key: " << name.GetText() << std::endl;
          return;
      }

      if (fallback.IsEmpty()) {
          //SdfToVtValueFunc* sdfToValue = UsdJsToSdfType(fallback.GetTypeid());
          //fallback = sdfToValue(value);
          std::cerr <<  "Currently not implement: Assigning an info key without fallback, since the type information is missing." << std::endl;
          return;
      }
      else {
        SdfToVtValueFunc* sdfToValue = UsdJsToSdfType(fallback.GetType().GetTypeName());
        // std::cout << sdfToValue << std::endl;
        fallback = (*sdfToValue)(value);
      }

      self.SetInfo(name, fallback);
    } catch (std::exception e) {
      std::cerr << "Exception: " << e.what() << std::endl;
    }
}

EMSCRIPTEN_BINDINGS(SdfSpec) {
  class_<pxr::SdfSpec>("SdfSpec")
    .property("layer", &pxr::SdfSpec::GetLayer)
    .property("path", &pxr::SdfSpec::GetPath)
    .function("SetInfo", &_SetInfo)
    .function("ClearInfo", &pxr::SdfSpec::ClearInfo)
    .smart_ptr<pxr::SdfHandle<pxr::SdfSpec>>("EmUsdSdfSpecHandle")
    ;
}
