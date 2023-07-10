#ifndef PXR_USD_SDF_WRAPASSETPATH_H
#define PXR_USD_SDF_WRAPASSETPATH_H

#include "pxr/usd/sdf/assetPath.h"
#include "pxr/base/tf/emscriptenTypeRegistration.h"
  
EMSCRIPTEN_REGISTER_TYPE_CONVERSION(pxr::SdfAssetPath)
  return BindingType<val>::toWireType(val(value.GetAssetPath()));
}

static pxr::SdfAssetPath fromWireType(WireType value) {
    return pxr::SdfAssetPath(BindingType<val>::fromWireType(value).as<std::string>());
EMSCRIPTEN_REGISTER_TYPE_CONVERSION_END(pxr::SdfAssetPath)
#endif // PXR_USD_SDF_WRAPASSETPATH_H