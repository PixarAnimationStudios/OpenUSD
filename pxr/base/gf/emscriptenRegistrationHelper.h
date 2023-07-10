/// \file gf/emscriptenRegistrationHelper.h

#ifndef PXR_BASE_GF_EMSCRIPTEN_REGISTRATION_HELPER_H
#define PXR_BASE_GF_EMSCRIPTEN_REGISTRATION_HELPER_H

#ifdef __EMSCRIPTEN__
  #include "pxr/base/tf/emscriptenTypeRegistration.h"
  #define REGISTER_GLVECTOR(VectorType) \
    EMSCRIPTEN_REGISTER_TYPE_CONVERSION(VectorType) \
      emscripten::val arrayVal = emscripten::val::array(); \
      for(size_t i = 0; i < VectorType::dimension; ++i) { \
          arrayVal.set(i, emscripten::val(value[i])); \
      } \
      return BindingType<val>::toWireType(arrayVal); \
    } \
    \
    static VectorType fromWireType(WireType value) { \
      VectorType vec; \
      emscripten::val inputVal = BindingType<val>::fromWireType(value); \
      for(size_t i = 0; i < VectorType::dimension; ++i) { \
          vec[i] = inputVal[i].as<VectorType::ScalarType>(); \
      } \
      return vec; \
    EMSCRIPTEN_REGISTER_TYPE_CONVERSION_END(VectorType)
#else // __EMSCRIPTEN__
  #define REGISTER_GLVECTOR(VectorType)
#endif // __EMSCRIPTEN__
#endif // PXR_BASE_GF_EMSCRIPTEN_REGISTRATION_HELPER_H
