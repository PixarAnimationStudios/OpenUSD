#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/usd/emscriptenPtrRegistrationHelper.h"
#include "pxr/base/tf/wrapTokenJs.h"

#include <emscripten/bind.h>
using namespace emscripten;

PXR_NAMESPACE_USING_DIRECTIVE

EMSCRIPTEN_REGISTER_SDF_HANDLE(SdfAttributeSpec)
EMSCRIPTEN_REGISTER_SMART_PTR(SdfLayer)
EMSCRIPTEN_ENABLE_WEAK_PTR_CAST(SdfLayer)

SdfAttributeSpecHandle
_NewAttributeSpec(
    const SdfPrimSpecHandle& owner,
    const std::string& name,
    const SdfValueTypeName& typeName) {
      return pxr::SdfAttributeSpec::New(owner, name, typeName);
}

EMSCRIPTEN_BINDINGS(SdfAttributeSpec) {
  class_<pxr::SdfAttributeSpec, base<pxr::SdfPropertySpec>>("SdfAttributeSpec")
    .constructor(&pxr::SdfAttributeSpec::New)
    .constructor(&_NewAttributeSpec)
    .property("connectionPathList", &pxr::SdfAttributeSpec::GetConnectionPathList)
    //.property("typeName", &pxr::SdfPrimSpec::GetTypeName, &pxr::SdfPrimSpec::SetTypeName)
    //.property("specifier", &pxr::SdfPrimSpec::GetSpecifier, &pxr::SdfPrimSpec::SetSpecifier)
    .smart_ptr<pxr::SdfHandle<pxr::SdfAttributeSpec>>("EmUsdSdfAttributeSpecHandle")
    ;
}
