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
#ifndef PXR_USD_USD_EMSCRIPTEN_PTR_REGISTRATION_HELPER_H
#define PXR_USD_USD_EMSCRIPTEN_PTR_REGISTRATION_HELPER_H

#include "pxr/base/tf/declarePtrs.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#include "pxr/base/tf/emscriptenTypeRegistration.h"

// TYPE should be the class type, without the pxr scope
#define EMSCRIPTEN_REGISTER_SMART_PTR(TYPE) \
    namespace emscripten { \
      template<> \
      struct smart_ptr_trait<pxr::TfRefPtr<pxr::TYPE>> : public default_smart_ptr_trait<pxr::TfRefPtr<pxr::TYPE>> { \
          typedef pxr::TYPE element_type; \
          static pxr::TYPE* get(const pxr::TfRefPtr<pxr::TYPE>& p) { \
              if (p) { \
              return p.operator->(); \
              } else { \
                return NULL; \
              } \
          } \
      }; \
      \
      template<> \
      struct smart_ptr_trait<pxr::TfWeakPtr<pxr::TYPE>> : public default_smart_ptr_trait<pxr::TfWeakPtr<pxr::TYPE>> { \
          typedef pxr::TYPE element_type; \
          static pxr::TYPE* get(const pxr::TfWeakPtr<pxr::TYPE>& p) { \
              if (p) { \
                return p.operator->(); \
              } else { \
                  return NULL; \
              } \
          } \
      }; \
    }

#define EMSCRIPTEN_REGISTER_SDF_HANDLE(TYPE) \
    namespace emscripten { \
      template<> \
      struct smart_ptr_trait<pxr::SdfHandle<pxr::TYPE>> : public default_smart_ptr_trait<pxr::SdfHandle<pxr::TYPE>> { \
          typedef pxr::TYPE element_type; \
          static pxr::TYPE* get(const pxr::SdfHandle<pxr::TYPE>& p) { \
              if (p) { \
              return p.operator->(); \
              } else { \
                return NULL; \
              } \
          } \
      }; \
    }

// TODO: This is not a great solution yet. We shouldn't need to convert weak pointers to ref pointers and back.
// It probably also interferes with the above traits, or emscripten's '.smart_ptr' construct, respectively.
// TYPE should be the class type, without the pxr scope
#define EMSCRIPTEN_ENABLE_WEAK_PTR_CAST(TYPE) \
    EMSCRIPTEN_REGISTER_TYPE_CONVERSION(pxr::TfWeakPtr<pxr::TYPE>) \
        return BindingType<val>::toWireType(val(pxr::TfCreateRefPtrFromProtectedWeakPtr(value))); \
    } \
    static pxr::TfWeakPtr<pxr::TYPE> fromWireType(WireType value) { \
        return pxr::TfWeakPtr<pxr::TYPE>(BindingType<val>::fromWireType(value).as<pxr::TfRefPtr<pxr::TYPE>>()); \
    EMSCRIPTEN_REGISTER_TYPE_CONVERSION_END(pxr::TfWeakPtr<pxr::TYPE>)

#endif // __EMSCRIPTEN__
#endif // PXR_USD_USD_EMSCRIPTEN_PTR_REGISTRATION_HELPER_H
