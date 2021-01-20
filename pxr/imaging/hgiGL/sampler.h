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
#ifndef PXR_IMAGING_HGIGL_SAMPLER_H
#define PXR_IMAGING_HGIGL_SAMPLER_H

#include "pxr/imaging/hgi/sampler.h"

#include "pxr/imaging/hgiGL/api.h"


PXR_NAMESPACE_OPEN_SCOPE


///
/// \class HgiGLSampler
///
/// OpenGL implementation of HgiSampler
///
class HgiGLSampler final : public HgiSampler
{
public:
    HGIGL_API
    ~HgiGLSampler() override;

    HGIGL_API
    uint64_t GetRawResource() const override;

    /// Returns the gl resource id of the sampler.
    HGIGL_API
    uint32_t GetSamplerId() const;

protected:
    friend class HgiGL;

    HGIGL_API
    HgiGLSampler(HgiSamplerDesc const& desc);

private:
    HgiGLSampler() = delete;
    HgiGLSampler & operator=(const HgiGLSampler&) = delete;
    HgiGLSampler(const HgiGLSampler&) = delete;

private:
    uint32_t _samplerId;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif