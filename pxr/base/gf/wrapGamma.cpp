//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>

#include "pxr/pxr.h"
#include "pxr/base/gf/gamma.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4d.h"

#include "pxr/base/tf/wrapTypeHelpers.h"

PXR_NAMESPACE_USING_DIRECTIVE

void wrapGamma(nanobind::module_ &m)
{    
    m.def("ApplyGamma", (GfVec3d(*)(GfVec3d const &,double))GfApplyGamma);
    m.def("ApplyGamma", (GfVec3f(*)(GfVec3f const &,double))GfApplyGamma);
    m.def("ApplyGamma", (GfVec3h(*)(GfVec3h const &,double))GfApplyGamma);
    m.def("ApplyGamma", (GfVec4d(*)(GfVec4d const &,double))GfApplyGamma);
    m.def("ApplyGamma", (GfVec4f(*)(GfVec4f const &,double))GfApplyGamma);
    m.def("ApplyGamma", (GfVec4h(*)(GfVec4h const &,double))GfApplyGamma);
    m.def("GetDisplayGamma",GfGetDisplayGamma);
    m.def("ConvertDisplayToLinear",(GfVec3d(*)(GfVec3d const &))GfConvertDisplayToLinear);
    m.def("ConvertDisplayToLinear",(GfVec3f(*)(GfVec3f const &))GfConvertDisplayToLinear);
    m.def("ConvertDisplayToLinear",(GfVec3h(*)(GfVec3h const &))GfConvertDisplayToLinear);
    m.def("ConvertDisplayToLinear",(GfVec4d(*)(GfVec4d const &))GfConvertDisplayToLinear);
    m.def("ConvertDisplayToLinear",(GfVec4f(*)(GfVec4f const &))GfConvertDisplayToLinear);
    m.def("ConvertDisplayToLinear",(GfVec4h(*)(GfVec4h const &))GfConvertDisplayToLinear);
    m.def("ConvertLinearToDisplay",(GfVec3d(*)(GfVec3d const &))GfConvertLinearToDisplay);
    m.def("ConvertLinearToDisplay",(GfVec3f(*)(GfVec3f const &))GfConvertLinearToDisplay);
    m.def("ConvertLinearToDisplay",(GfVec3h(*)(GfVec3h const &))GfConvertLinearToDisplay);
    m.def("ConvertLinearToDisplay",(GfVec4d(*)(GfVec4d const &))GfConvertLinearToDisplay);
    m.def("ConvertLinearToDisplay",(GfVec4f(*)(GfVec4f const &))GfConvertLinearToDisplay);
    m.def("ConvertLinearToDisplay",(GfVec4h(*)(GfVec4h const &))GfConvertLinearToDisplay);
}
