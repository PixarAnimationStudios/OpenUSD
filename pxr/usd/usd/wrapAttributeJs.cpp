#include "pxr/base/vt/value.h"
#include "pxr/usd/usd/attribute.h"

#include "pxr/usd/usd/emscriptenSdfToVtValue.h"

#include <emscripten/bind.h>
using namespace emscripten;

val _Get(pxr::UsdAttribute& self) {
    pxr::VtValue value;
    self.Get(&value);
    return value._GetJsVal();
};

std::string GetTypeName(pxr::UsdAttribute& self) {
    return self.GetTypeName().GetType().GetTypeName();
}

EMSCRIPTEN_BINDINGS(UsdAttribute) {
    register_vector<pxr::UsdAttribute>("vector<UsdAttribute>");

    class_<pxr::UsdAttribute>("UsdAttribute")
        .function("Get", &_Get)
        .function("Set", &::SetVtValueFromEmscriptenVal<pxr::UsdAttribute>)
        .function("GetTypeName", GetTypeName)
        ;
}
