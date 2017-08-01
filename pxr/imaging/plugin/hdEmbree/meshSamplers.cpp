//
// Copyright 2017 Pixar
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
#include "pxr/imaging/hdEmbree/meshSamplers.h"

#include "pxr/imaging/hd/meshUtil.h"
#include "pxr/imaging/hd/vtBufferSource.h"

PXR_NAMESPACE_OPEN_SCOPE

// HdEmbreeRTCBufferAllocator

RTCBufferType
HdEmbreeRTCBufferAllocator::Allocate()
{
    for (size_t i = 0; i < _bitset.size(); ++i) {
        if (!_bitset.test(i)) {
            _bitset.set(i);
            return static_cast<RTCBufferType>(
                static_cast<size_t>(RTC_USER_VERTEX_BUFFER) + i);
        }
    }
    return static_cast<RTCBufferType>(-1);
}

void
HdEmbreeRTCBufferAllocator::Free(RTCBufferType buffer)
{
    _bitset.reset(buffer - RTC_USER_VERTEX_BUFFER);
}

// HdEmbreeConstantSampler

bool
HdEmbreeConstantSampler::Sample(unsigned int element, float u, float v,
    void* value, int componentType, short numComponents) const
{
    return _sampler.Sample(0, value, componentType, numComponents);
}

// HdEmbreeUniformSampler

bool
HdEmbreeUniformSampler::Sample(unsigned int element, float u, float v,
    void* value, int componentType, short numComponents) const
{
    if (_primitiveParams.empty()) {
        return _sampler.Sample(element, value, componentType,
            numComponents);
    }
    if (element >= _primitiveParams.size()) {
        return false;
    }
    return _sampler.Sample(
        HdMeshUtil::DecodeFaceIndexFromCoarseFaceParam(
            _primitiveParams[element]),
        value, componentType, numComponents);
}

// HdEmbreeTriangleVertexSampler

bool
HdEmbreeTriangleVertexSampler::Sample(unsigned int element, float u, float v,
    void* value, int componentType, short numComponents) const
{
    if (element >= _indices.size()) {
        return false;
    }
    HdEmbreeTypeHelper::PrimvarTypeContainer corners[3];
    if (!_sampler.Sample(_indices[element][0], &corners[0], componentType,
            numComponents) ||
        !_sampler.Sample(_indices[element][1], &corners[1], componentType,
            numComponents) ||
        !_sampler.Sample(_indices[element][2], &corners[2], componentType,
            numComponents)) {
        return false;
    }
    void* samples[3] = { static_cast<void*>(&corners[0]),
                         static_cast<void*>(&corners[1]),
                         static_cast<void*>(&corners[2]) };
    // Embree specification of triangle interpolation:
    // t_uv = (1-u-v)*t0 + u*t1 + v*t2
    float weights[3] = { 1.0f - u - v, u, v };
    return _Interpolate(value, samples, weights, 3, componentType,
        numComponents);
}

// HdEmbreeTriangleFaceVaryingSampler

bool
HdEmbreeTriangleFaceVaryingSampler::Sample(unsigned int element, float u,
    float v, void* value, int componentType, short numComponents) const
{
    HdEmbreeTypeHelper::PrimvarTypeContainer corners[3];
    if (!_sampler.Sample(element*3 + 0, &corners[0], componentType,
            numComponents) ||
        !_sampler.Sample(element*3 + 1, &corners[1], componentType,
            numComponents) ||
        !_sampler.Sample(element*3 + 2, &corners[2], componentType,
            numComponents)) {
        return false;
    }
    void* samples[3] = { static_cast<void*>(&corners[0]),
                         static_cast<void*>(&corners[1]),
                         static_cast<void*>(&corners[2]) };
    // Embree specification of triangle interpolation:
    // t_uv = (1-u-v)*t0 + u*t1 + v*t2
    float weights[3] = { 1.0f - u - v, u, v };
    return _Interpolate(value, samples, weights, 3, componentType,
        numComponents);
}

/* static */ VtValue
HdEmbreeTriangleFaceVaryingSampler::_Triangulate(TfToken const& name,
    VtValue const& value, HdMeshUtil &meshUtil)
{
    HdVtBufferSource buffer(name, value);
    VtValue triangulated;
    if (!meshUtil.ComputeTriangulatedFaceVaryingPrimvar(
            buffer.GetData(), buffer.GetNumElements(),
            buffer.GetGLElementDataType(),
            &triangulated)) {
        TF_CODING_ERROR("[%s] Could not triangulate face-varying data.",
            name.GetText());
        return VtValue();
    }
    return triangulated;
}

// HdEmbreeSubdivVertexSampler

HdEmbreeSubdivVertexSampler::HdEmbreeSubdivVertexSampler(TfToken const& name,
    VtValue const& value, RTCScene meshScene, unsigned meshId,
    HdEmbreeRTCBufferAllocator *allocator)
    : _embreeBufferId(static_cast<RTCBufferType>(-1))
    , _buffer(name, value)
    , _meshScene(meshScene)
    , _meshId(meshId)
    , _allocator(allocator)
{
    // The embree API only supports float primvars.
    if (_buffer.GetGLComponentDataType() != GL_FLOAT) {
        TF_CODING_ERROR("Embree subdivision meshes only support float-based"
            " primvars for vertex interpolation mode");
        return;
    }
    _embreeBufferId = _allocator->Allocate();
    // The embree API has a constant number of primvar slots (16 at last
    // count), shared between vertex and face-varying modes.
    if (_embreeBufferId == static_cast<RTCBufferType>(-1)) {
        TF_CODING_ERROR("Embree subdivision meshes only support %d primvars"
            " in vertex interpolation mode", RTC_MAX_USER_VERTEX_BUFFERS);
        return;
    }
    // Tag the embree mesh object with the primvar buffer, for use by
    // rtcInterpolate.
    rtcSetBuffer(_meshScene, _meshId, _embreeBufferId,
        _buffer.GetData(), 0, _buffer.GetNumComponents() * sizeof(float));
}

HdEmbreeSubdivVertexSampler::~HdEmbreeSubdivVertexSampler()
{
    if (_embreeBufferId != static_cast<RTCBufferType>(-1)) {
        _allocator->Free(_embreeBufferId);
    }
}

bool
HdEmbreeSubdivVertexSampler::Sample(unsigned int element, float u, float v,
    void* value, int componentType, short numComponents) const
{
    // Make sure the buffer type and sample type have the same arity.
    // _embreeBufferId of -1 indicates this sampler failed to initialize.
    if (_embreeBufferId == static_cast<RTCBufferType>(-1) ||
        componentType != GL_FLOAT ||
        numComponents != _buffer.GetNumComponents()) {
        return false;
    }
    rtcInterpolate(_meshScene, _meshId, element, u, v, _embreeBufferId,
        static_cast<float*>(value), nullptr, nullptr, numComponents);
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
