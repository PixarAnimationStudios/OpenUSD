#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/usd/emscriptenPtrRegistrationHelper.h"
#include "pxr/usd/sdf/wrapPathJs.h"
#include "pxr/base/tf/wrapTokenJs.h"

#include <emscripten/bind.h>
using namespace emscripten;

PXR_NAMESPACE_USING_DIRECTIVE

EMSCRIPTEN_REGISTER_SDF_HANDLE(SdfPrimSpec)
EMSCRIPTEN_REGISTER_SMART_PTR(SdfLayer)
EMSCRIPTEN_ENABLE_WEAK_PTR_CAST(SdfLayer)

EMSCRIPTEN_BINDINGS(SdfPrimSpec) {
  function("SdfCreatePrimInLayer", pxr::SdfCreatePrimInLayer);

  class_<pxr::SdfPrimSpec, base<pxr::SdfSpec>>("SdfPrimSpec")
    .property("typeName", &pxr::SdfPrimSpec::GetTypeName, &pxr::SdfPrimSpec::SetTypeName)
    .property("specifier", &pxr::SdfPrimSpec::GetSpecifier, &pxr::SdfPrimSpec::SetSpecifier)
    .function("GetRealNameParent", &pxr::SdfPrimSpec::GetRealNameParent)
    .function("RemoveNameChild", &pxr::SdfPrimSpec::RemoveNameChild)
    .smart_ptr<pxr::SdfHandle<pxr::SdfPrimSpec>>("EmUsdSdfPrimSpecHandle")
    ;
}
