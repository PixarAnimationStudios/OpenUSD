//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_FOR_EACH_COMMON_TYPE_H
#define PXR_EXEC_VDF_FOR_EACH_COMMON_TYPE_H

// This header includes definitions for all of the "common" types.
//
// Common types do not have any special runtime properties in the system.
// They are only used to emit templated execution structures for the purpose
// improve build times in downstream libraries.
//
// Because all clients of execution will transitively include this header,
// care must be taken when adding includes.

#include "pxr/pxr.h"

#include "pxr/exec/vdf/indexedWeights.h"

#include "pxr/base/gf/half.h"
#include "pxr/base/gf/matrix2d.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/quatd.h"
#include "pxr/base/gf/quatf.h"
#include "pxr/base/gf/quath.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2h.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3h.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4h.h"
#include "pxr/base/gf/vec4i.h"

#include "pxr/base/tf/token.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// VDF_FOR_EACH_COMMON_TYPE expands \p macro for each common type.
#define VDF_FOR_EACH_COMMON_TYPE(macro)          \
    macro(bool)                                  \
    macro(char)                                  \
    macro(unsigned char)                         \
    macro(int)                                   \
    macro(unsigned int)                          \
    macro(long)                                  \
    macro(unsigned long)                         \
    macro(long long)                             \
    macro(unsigned long long)                    \
                                                 \
    macro(float)                                 \
    macro(double)                                \
                                                 \
    macro(GfHalf)                                \
    macro(GfMatrix2d)                            \
    macro(GfMatrix3d)                            \
    macro(GfMatrix4d)                            \
    macro(GfQuatd)                               \
    macro(GfQuatf)                               \
    macro(GfQuath)                               \
    macro(GfVec2d)                               \
    macro(GfVec2f)                               \
    macro(GfVec2h)                               \
    macro(GfVec2i)                               \
    macro(GfVec3d)                               \
    macro(GfVec3f)                               \
    macro(GfVec3h)                               \
    macro(GfVec3i)                               \
    macro(GfVec4d)                               \
    macro(GfVec4f)                               \
    macro(GfVec4h)                               \
    macro(GfVec4i)                               \
                                                 \
    macro(VdfIndexedWeights)                     \
                                                 \
    macro(TfToken)                               \
                                                 \
    macro(std::string)

PXR_NAMESPACE_CLOSE_SCOPE

#endif
