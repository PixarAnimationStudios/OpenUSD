#include "pxr/usd/usd/references.h"
#include "pxr/usd/sdf/reference.h"

#include <emscripten/bind.h>
using namespace emscripten;

bool AddReference(pxr::UsdReferences& self, const std::string& ref) {
    return self.AddReference(ref);
}

EMSCRIPTEN_BINDINGS(UsdReferences) {
  class_<pxr::UsdReferences>("UsdReferences")
    .function("AddReference", &AddReference)
    ;
}
