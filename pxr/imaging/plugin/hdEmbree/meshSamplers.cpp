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
#include "pxr/imaging/plugin/hdEmbree/meshSamplers.h"

#include "pxr/imaging/hd/meshUtil.h"
#include "pxr/imaging/hd/vtBufferSource.h"

PXR_NAMESPACE_OPEN_SCOPE

// HdEmbreeRTCBufferAllocator

int
HdEmbreeRTCBufferAllocator::Allocate()
{
    for (size_t i = 0; i < _bitset.size(); ++i) {
        if (!_bitset.test(i)) {
            _bitset.set(i);
            return static_cast<int>(i);
        }
    }
    return -1;
}

void
HdEmbreeRTCBufferAllocator::Free(int bufferIndex)
{
    _bitset.reset(bufferIndex);
}


unsigned int
HdEmbreeRTCBufferAllocator::NumBuffers()
{
    // Technically this may overcount, since a buffer may have been freed
    // but we don't move back to fill the slot, however it will be filled
    // before more are allocated. Now that there are possible a "large"
    // number of buffers it might want to be handled differently in the
    // future.
    for (int i = _bitset.size() - 1; i >= 0; i--) {
        if (_bitset.test(i)) {
            return i+1;
        }
    }
    return 0;
}

// HdEmbreeConstantSampler

bool
HdEmbreeConstantSampler::Sample(unsigned int element, float u, float v,
    void* value, HdTupleType dataType) const
{
    return _sampler.Sample(0, value, dataType);
}

// HdEmbreeUniformSampler

bool
HdEmbreeUniformSampler::Sample(unsigned int element, float u, float v,
    void* value, HdTupleType dataType) const
{
    if (_primitiveParams.empty()) {
        return _sampler.Sample(element, value, dataType);
    }
    if (element >= _primitiveParams.size()) {
        return false;
    }
    return _sampler.Sample(
        HdMeshUtil::DecodeFaceIndexFromCoarseFaceParam(
            _primitiveParams[element]),
        value, dataType);
}

// HdEmbreeTriangleVertexSampler

bool
HdEmbreeTriangleVertexSampler::Sample(unsigned int element, float u, float v,
    void* value, HdTupleType dataType) const
{
    if (element >= _indices.size()) {
        return false;
    }
    HdEmbreeTypeHelper::PrimvarTypeContainer corners[3];
    if (!_sampler.Sample(_indices[element][0], &corners[0], dataType) ||
        !_sampler.Sample(_indices[element][1], &corners[1], dataType) ||
        !_sampler.Sample(_indices[element][2], &corners[2], dataType)) {
        return false;
    }
    void* samples[3] = { static_cast<void*>(&corners[0]),
                         static_cast<void*>(&corners[1]),
                         static_cast<void*>(&corners[2]) };
    // Embree specification of triangle interpolation:
    // t_uv = (1-u-v)*t0 + u*t1 + v*t2
    float weights[3] = { 1.0f - u - v, u, v };
    return _Interpolate(value, samples, weights, 3, dataType);
}

// HdEmbreeTriangleFaceVaryingSampler

bool
HdEmbreeTriangleFaceVaryingSampler::Sample(unsigned int element, float u,
    float v, void* value, HdTupleType dataType) const
{
    HdEmbreeTypeHelper::PrimvarTypeContainer corners[3];
    if (!_sampler.Sample(element*3 + 0, &corners[0], dataType) ||
        !_sampler.Sample(element*3 + 1, &corners[1], dataType) ||
        !_sampler.Sample(element*3 + 2, &corners[2], dataType)) {
        return false;
    }
    void* samples[3] = { static_cast<void*>(&corners[0]),
                         static_cast<void*>(&corners[1]),
                         static_cast<void*>(&corners[2]) };
    // Embree specification of triangle interpolation:
    // t_uv = (1-u-v)*t0 + u*t1 + v*t2
    float weights[3] = { 1.0f - u - v, u, v };
    return _Interpolate(value, samples, weights, 3, dataType);
}

/* static */ VtValue
HdEmbreeTriangleFaceVaryingSampler::_Triangulate(TfToken const& name,
    VtValue const& value, HdMeshUtil &meshUtil)
{
    HdVtBufferSource buffer(name, value);
    VtValue triangulated;
    if (!meshUtil.ComputeTriangulatedFaceVaryingPrimvar(
            buffer.GetData(),
            buffer.GetNumElements(),
            buffer.GetTupleType().type,
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
    : _embreeBufferId(-1)
    , _buffer(name, value)
    , _meshScene(meshScene)
    , _meshId(meshId)
    , _allocator(allocator)
{
    // Arrays are not supported
    if (_buffer.GetTupleType().count != 1) {
        TF_WARN("Unsupported array size for vertex primvar");
        return;
    }

    // The embree API only supports float-component primvars.
    RTCFormat format = RTC_FORMAT_FLOAT;
    switch (HdGetComponentType(_buffer.GetTupleType().type)) {
        case HdTypeFloat:
            format = RTC_FORMAT_FLOAT;
            break;
        case HdTypeFloatVec2:
            format = RTC_FORMAT_FLOAT2;
            break;
        case HdTypeFloatVec3:
            format = RTC_FORMAT_FLOAT3;
            break;
        case HdTypeFloatVec4:
            format = RTC_FORMAT_FLOAT4;
            break;
        default:
            TF_WARN("Embree subdivision meshes only support float-based"
            " primvars for vertex interpolation mode");
            return;
    };


    _embreeBufferId = _allocator->Allocate();
    // The embree API has a constant number of primvar slots (16 at last
    // count), shared between vertex and face-varying modes.
    if (_embreeBufferId == -1) {
        TF_WARN("Embree subdivision meshes only support %d primvars"
            " in vertex interpolation mode, excceded for rprim ",
            HdEmbreeRTCBufferAllocator::PXR_MAX_USER_VERTEX_BUFFERS);
        return;
    }

    // Set number of vertex attributes correctly
    rtcSetGeometryVertexAttributeCount(
        rtcGetGeometry(_meshScene,_meshId),_allocator->NumBuffers());

    // The start address (`byteOffset` argument) and stride (`byteStride`
    // argument) must be both aligned to 4 bytes; otherwise the
    // `rtcSetGeometryBuffer` function will fail. Pretty sure we are interpolating
    // floats, so this will be ok, but this is possibly not robust. Not sure
    // that it will be easy to enforce this alignment on the data
    // that is gotten from the HdVtBufferSource
    rtcSetSharedGeometryBuffer(
        rtcGetGeometry(_meshScene,_meshId), /* RTCGeometry geometry */
        RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, /*enum RTCBufferType type */
        static_cast<size_t>(_embreeBufferId), /* unsigned int slot */
        format, /*enum RTCFormat format */
        _buffer.GetData(), /* const void* ptr */
        0, /*size_t byteOffset */
        HdDataSizeOfTupleType(_buffer.GetTupleType()), /* size_t byteStride */
        _buffer.GetNumElements() /* size_t itemCount */);
}

HdEmbreeSubdivVertexSampler::~HdEmbreeSubdivVertexSampler()
{
    if (_embreeBufferId != -1) {
        _allocator->Free(_embreeBufferId);
    }
}

bool
HdEmbreeSubdivVertexSampler::Sample(unsigned int element, float u, float v,
    void* value, HdTupleType dataType) const
{
    // Make sure the buffer type and sample type have the same arity.
    if (_embreeBufferId == -1 || dataType != _buffer.GetTupleType()) {
        return false;
    }

    // Combine number of components in the underlying type and tuple arity.
    size_t numFloats = HdGetComponentCount(dataType.type) * dataType.count;

    // To use `rtcInterpolateN` for a geometry, all changes to that geometry
    // must be properly committed using `rtcCommitGeometry`
    rtcInterpolate1(
        rtcGetGeometry(_meshScene,_meshId), /* RTCGeometry geometry */
        element, /* unsigned int primID */
        u, /* float u */
        v, /* float v */
        RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, /* enum RTCBufferType bufferType */
        static_cast<size_t>(_embreeBufferId), /* unsigned int slot */
        static_cast<float*>(value), /* float* P */
        nullptr, /* float* dPdu */
        nullptr, /* float* dPdv */
        numFloats /* unsigned int valueCount */);

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
