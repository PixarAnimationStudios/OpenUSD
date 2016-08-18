//
// Copyright 2016 Pixar
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
#ifndef TF_API_H
#define TF_API_H

#include "pxr/base/arch/export.h"

#if defined(TF_STATIC)
#   define TF_API
#   define TF_LOCAL
#   define TF_PY_API
#else
#   if defined(TF_EXPORTS)
#       define TF_API ARCH_EXPORT
#       define TF_API_TEMPLATE_CLASS(...)
#       define TF_API_TEMPLATE_STRUCT(...)
#   else
#       define TF_API ARCH_IMPORT
#       define TF_API_TEMPLATE_CLASS(...) extern template class TF_API __VA_ARGS__
#       define TF_API_TEMPLATE_STRUCT(...) extern template struct TF_API __VA_ARGS__
#   endif
#   if defined(_BUILDING_PYD)
#       define TF_PY_API __declspec(dllexport)
#   else
#       define TF_PY_API
#   endif
#   define TF_LOCAL TF_HIDDEN
#endif

#endif