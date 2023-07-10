#ifndef PXR_USD_USD_EMSCRIPTEN_SDF_TO_VTVALUE_H
#define PXR_USD_USD_EMSCRIPTEN_SDF_TO_VTVALUE_H

#include <emscripten/val.h>

#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/usd/attribute.h"

#include <string>
#include <functional>
#include <iostream>

typedef std::function<pxr::VtValue (const emscripten::val& jsVal)> SdfToVtValueFunc;

SdfToVtValueFunc* UsdJsToSdfType(pxr::SdfValueTypeName const &targetType);

pxr::VtValue GetVtValueFromEmscriptenVal(const emscripten::val& value, pxr::SdfValueTypeName const &targetType, bool* const success=NULL);

template <typename T>
bool SetVtValueFromEmscriptenVal(T& self, const emscripten::val& value) {
    SdfToVtValueFunc* sdfToValue = UsdJsToSdfType(self.GetTypeName());
    bool result = false;
    if (sdfToValue != NULL) {
        pxr::VtValue vtValue = (*sdfToValue)(value);
        result = self.Set(vtValue);
    } else {
        std::cerr << "Couldn't find a VtValue mapping for " << self.GetTypeName() << std::endl;
    }
    return result;
}

template <typename T, pxr::UsdAttribute(T::*setter)(pxr::VtValue const &defaultValue, bool writeSparsely) const>
pxr::UsdAttribute SetCustomAttributeFromEmscriptenVal(T& self, const emscripten::val& value) {
    bool result = false;
    const pxr::SdfValueTypeName& typeName = (&self->*setter)(pxr::VtValue(), false).GetTypeName();
    pxr::VtValue vtValue = GetVtValueFromEmscriptenVal(value, typeName, &result);
    if (result) {
        return (&self->*setter)(vtValue, false);
    } else {
        std::cerr << "Couldn't find a VtValue mapping for " << typeName << std::endl;
    }
    return pxr::UsdAttribute();
}

#endif //PXR_USD_USD_EMSCRIPTEN_SDF_TO_VTVALUE_H