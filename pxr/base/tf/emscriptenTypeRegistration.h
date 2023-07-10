//
// Copyright 2021 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef PXR_BASE_TF_EMSCRIPTEN_TYPE_REGISTRATION_H
#define PXR_BASE_TF_EMSCRIPTEN_TYPE_REGISTRATION_H

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
// Use this define to make a type without bindings known to Emscripten
#define EMSCRIPTEN_REGISTER_TYPE(TYPE) \
  namespace emscripten { \
      namespace internal { \
        template<>\
        struct TypeID<TYPE> {\
            static constexpr TYPEID get() {\
                return LightTypeID<val>::get();\
            }\
        };\
\
        template<>\
        struct TypeID<const TYPE> {\
            static constexpr TYPEID get() {\
                return LightTypeID<val>::get();\
            }\
        };\
\
        template<>\
        struct TypeID<TYPE&> {\
            static constexpr TYPEID get() {\
                return LightTypeID<val>::get();\
            }\
        };\
\
        template<>\
        struct TypeID<const TYPE&> {\
            static constexpr TYPEID get() {\
                return LightTypeID<val>::get();\
            }\
        };\
    }\
  }

// These two defines together allow you to define custom conversions to
// javascript and from javascript to cpp
#define EMSCRIPTEN_REGISTER_TYPE_CONVERSION(TYPE) \
  namespace emscripten { \
      namespace internal { \
          template<> \
          struct BindingType<TYPE> { \
              typedef EM_VAL WireType; \
              static WireType toWireType(const TYPE& value) { \

#define EMSCRIPTEN_REGISTER_TYPE_CONVERSION_END(TYPE) \
            } \
        }; \
        \
    }\
  }\
EMSCRIPTEN_REGISTER_TYPE(TYPE)

// This define is used to map std::vectors of ValueType as arrays in 
// Javascript
#define EMSCRIPTEN_REGISTER_VECTOR_TO_ARRAY_CONVERSION(ValueType) \
namespace emscripten { \
    namespace internal { \
        template<> \
        struct BindingType<std::vector<ValueType>> { \
            using ValBinding = BindingType<val>; \
            using WireType = ValBinding::WireType; \
        \
            static WireType toWireType(const std::vector<ValueType> &vec) { \
                return ValBinding::toWireType(val::array(vec)); \
            } \
        \
            static std::vector<ValueType> fromWireType(WireType value) { \
                return vecFromJSArray<ValueType>(ValBinding::fromWireType(value)); \
            } \
        }; \
    } \
}

#endif // __EMSCRIPTEN__
#endif // PXR_BASE_TF_EMSCRIPTEN_TYPE_REGISTRATION_H
