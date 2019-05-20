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
#include "pxr/pxr.h"
#include "usdMaya/colorSpace.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4d.h"

#include <boost/python/def.hpp>
#include <boost/python.hpp>

#include "pxr/base/tf/pyResultConversions.h"

using namespace std;
using namespace boost::python;
using namespace boost;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapColorSpace()
{
    def("ConvertLinearToMaya", UsdMayaColorSpace::ConvertLinearToMaya<GfVec3f>);
    def("ConvertLinearToMaya", UsdMayaColorSpace::ConvertLinearToMaya<GfVec3d>);
    def("ConvertLinearToMaya", UsdMayaColorSpace::ConvertLinearToMaya<GfVec4f>);
    def("ConvertLinearToMaya", UsdMayaColorSpace::ConvertLinearToMaya<GfVec4d>);
    def("ConvertMayaToLinear", UsdMayaColorSpace::ConvertMayaToLinear<GfVec3f>);
    def("ConvertMayaToLinear", UsdMayaColorSpace::ConvertMayaToLinear<GfVec3d>);
    def("ConvertMayaToLinear", UsdMayaColorSpace::ConvertMayaToLinear<GfVec4f>);
    def("ConvertMayaToLinear", UsdMayaColorSpace::ConvertMayaToLinear<GfVec4d>);
}
