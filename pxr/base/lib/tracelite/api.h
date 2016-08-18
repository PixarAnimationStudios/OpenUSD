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
#ifndef TRACELITE_API_H
#define TRACELITE_API_H

#include "pxr/base/arch/export.h"

#if defined(TRACELITE_STATIC)
#   define TRACELITE_API
#   define TRACELITE_LOCAL
#else
#   if defined(TRACELITE_EXPORTS)
#       define TRACELITE_API ARCH_EXPORT
#       define TRACELITE_API_TEMPLATE_CLASS(...)
#       define TRACELITE_API_TEMPLATE_STRUCT(...)
#   else
#       define TRACELITE_API ARCH_IMPORT
#       define TRACELITE_API_TEMPLATE_CLASS(...) extern template class TRACELITE_API __VA_ARGS__
#       define TRACELITE_API_TEMPLATE_STRUCT(...) extern template struct TRACELITE_API __VA_ARGS__
#   endif
#   define TRACELITE_LOCAL TRACELITE_HIDDEN
#endif

#endif