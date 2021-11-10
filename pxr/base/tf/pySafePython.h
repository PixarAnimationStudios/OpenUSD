//
// Copyright 2020 Pixar
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
#ifndef PXR_BASE_TF_PY_SAFE_PYTHON_H
#define PXR_BASE_TF_PY_SAFE_PYTHON_H

#include "pxr/base/arch/pragmas.h"

/// \file tf/pySafePython.h
/// Intended to replace a direct include of Python.h, which causes several
/// build problems with certain configurations and platforms (e.g., debug 
/// builds on Windows, Qt slots keyword, etc.)

// This include is a hack to avoid build errors due to incompatible
// macro definitions in pyport.h on MacOS for older versions of Python.
// See: https://bugs.python.org/issue10910
#include <locale>

// Python 3 has a conflict with the slots macro defined by the Qt library,
// so we're undef'ing here temporarily before including Python.
ARCH_PRAGMA_PUSH_MACRO(slots)
#undef slots

#include <boost/python/detail/wrap_python.hpp>

ARCH_PRAGMA_POP_MACRO(slots)

#endif // PXR_BASE_TF_PY_SAFE_PYTHON_H
