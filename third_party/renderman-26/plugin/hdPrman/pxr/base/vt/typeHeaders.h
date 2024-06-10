//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_PXR_BASE_VT_TYPE_HEADERS_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_PXR_BASE_VT_TYPE_HEADERS_H

// XXX: Delete this file after hdPrman drops support for USD versions
// older than 22.08.

#include "pxr/pxr.h" // PXR_VERSION
// IWYU pragma: begin_exports
#if PXR_VERSION >= 2208
#include <pxr/base/vt/typeHeaders.h>
#else
#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/half.h"
#include "pxr/base/gf/interval.h"
#include "pxr/base/gf/matrix2d.h"
#include "pxr/base/gf/matrix2f.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/multiInterval.h"
#include "pxr/base/gf/quatd.h"
#include "pxr/base/gf/quaternion.h"
#include "pxr/base/gf/quatf.h"
#include "pxr/base/gf/quath.h"
#include "pxr/base/gf/range1d.h"
#include "pxr/base/gf/range1f.h"
#include "pxr/base/gf/range2d.h"
#include "pxr/base/gf/range2f.h"
#include "pxr/base/gf/range3d.h"
#include "pxr/base/gf/range3f.h"
#include "pxr/base/gf/rect2i.h"
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
#include "pxr/base/vt/array.h"
#include <string>
#endif // PXR_VERSION >= 2211
// IWYU pragma: end_exports

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_PXR_BASE_VT_TYPE_HEADERS_H
