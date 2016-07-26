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
#ifndef ARCH_PRAGMAS_H
#define ARCH_PRAGMAS_H

#include "pxr/base/arch/defines.h"

///
/// \file arch/pragmas.h
/// \brief Pragmas for controlling compiler-specific behaviors.

/// This header contains pragmas used to control compiler-specific behaviors.
/// Behaviors that are not supported or required by a certain compiler should
/// be implemented as a no-op.
///

#if defined(ARCH_COMPILER_GCC)

/// Convert errors about variables that may be used before initialization into
/// warnings. A warning is emitted, but won't cause a build failure with
/// -Werror enabled.
#define ARCH_PRAGMA_PUSH_NOERROR_MAYBE_UNINITIALIZED \
    _Pragma("GCC diagnostic push"); \
    _Pragma("GCC diagnostic warning \"-Wmaybe-uninitialized\"");
#define ARCH_PRAGMA_POP_NOERROR_MAYBE_UNINITIALIZED \
    _Pragma("GCC diagnostic pop");

#else
#define ARCH_PRAGMA_PUSH_NOERROR_MAYBE_UNINITIALIZED
#define ARCH_PRAGMA_POP_NOERROR_MAYBE_UNINITIALIZED
#endif

#endif // ARCH_PRAGMAS_H
