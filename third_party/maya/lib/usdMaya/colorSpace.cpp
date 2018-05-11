//
// Copyright 2018 Pixar
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
#include "usdMaya/colorSpace.h"
#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_ENV_SETTING(PIXMAYA_LINEAR_COLORS, false,
        "If colors from maya should be treated as linear.  "
        "When false, colors are assumed to be gamma-corrected.");

bool
PxrUsdMayaColorSpace::IsColorManaged()
{
    // in theory this could vary per scene, but we think mixing that within any
    // given pipeline is likely confusing.  Also, we want to avoid this function
    // calling out to mel.
    return TfGetEnvSetting(PIXMAYA_LINEAR_COLORS);
}
