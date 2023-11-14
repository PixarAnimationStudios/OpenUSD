//
// Copyright 2022 Pixar
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
#include "pxr/imaging/hgiWebGPU/capabilities.h"
#include "pxr/imaging/hgiWebGPU/hgi.h"

#include "pxr/base/arch/defines.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiWebGPUCapabilities::HgiWebGPUCapabilities(wgpu::Device)
{
    _maxUniformBlockSize          = 64 * 1024;
    _maxShaderStorageBlockSize    = 1 * 1024 * 1024 * 1024;

    // https://github.com/gfx-rs/wgpu/issues/158#issuecomment-490653129
    _uniformBufferOffsetAlignment = 256;
    _SetFlag(HgiDeviceCapabilitiesBitsPrimitiveIdEmulation, true);
    _SetFlag(HgiDeviceCapabilitiesBitsCppShaderPadding, false);
    _SetFlag(HgiDeviceCapabilitiesBitsGeometricStage, false);
    _SetFlag(HgiDeviceCapabilitiesBitsOSDSupport, false);
    _SetFlag(HgiDeviceCapabilitiesBitsClipDistanceSupport, false);

}

HgiWebGPUCapabilities::~HgiWebGPUCapabilities() = default;

int
HgiWebGPUCapabilities::GetAPIVersion() const
{
    return 0;
}

int
HgiWebGPUCapabilities::GetShaderVersion() const
{
    return 460;
}

bool
HgiWebGPUCapabilities::IsViewportYUp() const {
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
