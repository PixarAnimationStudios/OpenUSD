#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/usd/emscriptenPtrRegistrationHelper.h"

#include <emscripten/bind.h>
using namespace emscripten;

PXR_NAMESPACE_USING_DIRECTIVE

EMSCRIPTEN_REGISTER_SDF_HANDLE(SdfRelationshipSpec)
EMSCRIPTEN_REGISTER_SMART_PTR(SdfLayer)
EMSCRIPTEN_ENABLE_WEAK_PTR_CAST(SdfLayer)

SdfRelationshipSpecHandle
_NewRelationshipSpec(
    const SdfPrimSpecHandle& owner,
    const std::string& name) {
      return pxr::SdfRelationshipSpec::New(owner, name);
}

EMSCRIPTEN_BINDINGS(SdfRelationshipSpec) {
  class_<pxr::SdfRelationshipSpec, base<pxr::SdfPropertySpec>>("SdfRelationshipSpec")
    .constructor(&pxr::SdfRelationshipSpec::New)
    .constructor(&_NewRelationshipSpec)
    .property("targetPathList", &pxr::SdfRelationshipSpec::GetTargetPathList)
    .smart_ptr<pxr::SdfHandle<pxr::SdfRelationshipSpec>>("EmUsdSdfRelationshipSpecHandle")
    ;
}
