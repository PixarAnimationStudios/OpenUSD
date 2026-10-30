//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>

#include "pxr/pxr.h"
#include "pxr/base/gf/homogeneous.h"

PXR_NAMESPACE_USING_DIRECTIVE

void wrapHomogeneous(nanobind::module_ &m)
{    
    m.def("GetHomogenized", (GfVec4d (*)(const GfVec4d &)) GfGetHomogenized);
    m.def("GetHomogenized", (GfVec4f (*)(const GfVec4f &)) GfGetHomogenized);

    m.def("HomogeneousCross", (GfVec4d (*)(const GfVec4d &, const GfVec4d &)) GfHomogeneousCross);
    m.def("HomogeneousCross", (GfVec4f (*)(const GfVec4f &, const GfVec4f &)) GfHomogeneousCross);

    m.def("Project", (GfVec3d (*)(const GfVec4d &)) GfProject);
    m.def("Project", (GfVec3f (*)(const GfVec4f &)) GfProject);
}
