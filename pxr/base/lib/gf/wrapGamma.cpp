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
#include <boost/python/def.hpp>

#include "pxr/pxr.h"
#include "pxr/base/gf/gamma.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4d.h"

#include "pxr/base/tf/wrapTypeHelpers.h"

using namespace boost::python;

PXR_NAMESPACE_OPEN_SCOPE

void wrapGamma()
{    
    def("ApplyGamma", (GfVec3f(*)(GfVec3f const &,double))GfApplyGamma);
    def("ApplyGamma", (GfVec3d(*)(GfVec3d const &,double))GfApplyGamma);
    def("ApplyGamma", (GfVec4f(*)(GfVec4f const &,double))GfApplyGamma);
    def("ApplyGamma", (GfVec4d(*)(GfVec4d const &,double))GfApplyGamma);
    def("GetDisplayGamma",GfGetDisplayGamma);
    def("ConvertDisplayToLinear",GfConvertDisplayToLinear<GfVec3f>);
    def("ConvertDisplayToLinear",GfConvertDisplayToLinear<GfVec3d>);
    def("ConvertDisplayToLinear",GfConvertDisplayToLinear<GfVec4f>);
    def("ConvertDisplayToLinear",GfConvertDisplayToLinear<GfVec4d>);
    def("ConvertLinearToDisplay",GfConvertLinearToDisplay<GfVec3f>);
    def("ConvertLinearToDisplay",GfConvertLinearToDisplay<GfVec3d>);
    def("ConvertLinearToDisplay",GfConvertLinearToDisplay<GfVec4f>);
    def("ConvertLinearToDisplay",GfConvertLinearToDisplay<GfVec4d>);
}

PXR_NAMESPACE_CLOSE_SCOPE
