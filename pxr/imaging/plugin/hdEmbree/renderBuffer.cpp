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
#include "pxr/imaging/hdEmbree/renderBuffer.h"

PXR_NAMESPACE_OPEN_SCOPE

HdEmbreeRenderBuffer::HdEmbreeRenderBuffer(SdfPath const& id)
    : HdRenderBuffer(id)
    , _width(0)
    , _height(0)
    , _format(HdFormatInvalid)
    , _multiSampled(false)
    , _buffer()
    , _sampleBuffer()
    , _sampleCount()
    , _mappers(0)
    , _converged(false)
{
}

HdEmbreeRenderBuffer::~HdEmbreeRenderBuffer()
{
}

void
HdEmbreeRenderBuffer::_Deallocate()
{
    // If the buffer is mapped while we're doing this, there's not a great
    // recovery path...
    TF_VERIFY(!IsMapped());

    _width = 0;
    _height = 0;
    _format = HdFormatInvalid;
    _multiSampled = false;
    _buffer.resize(0);
    _sampleBuffer.resize(0);
    _sampleCount.resize(0);

    _mappers.store(0);
    _converged.store(false);
}

/*static*/
size_t
HdEmbreeRenderBuffer::_GetBufferSize(GfVec2i const &dims, HdFormat format)
{
    return dims[0] * dims[1] * HdDataSizeOfFormat(format);
}

/*static*/
HdFormat
HdEmbreeRenderBuffer::_GetSampleFormat(HdFormat format)
{
    HdFormat component = HdGetComponentFormat(format);
    size_t arity = HdGetComponentCount(format);

    if (component == HdFormatUNorm8 || component == HdFormatSNorm8 ||
        component == HdFormatFloat32) {
        if (arity == 1) { return HdFormatFloat32; }
        else if (arity == 2) { return HdFormatFloat32Vec2; }
        else if (arity == 3) { return HdFormatFloat32Vec3; }
        else if (arity == 4) { return HdFormatFloat32Vec4; }
    } else if (component == HdFormatInt32) {
        if (arity == 1) { return HdFormatInt32; }
        else if (arity == 2) { return HdFormatInt32Vec2; }
        else if (arity == 3) { return HdFormatInt32Vec3; }
        else if (arity == 4) { return HdFormatInt32Vec4; }
    }
    return HdFormatInvalid;
}

/*virtual*/
bool
HdEmbreeRenderBuffer::Allocate(GfVec3i const& dimensions,
                               HdFormat format,
                               bool multiSampled)
{
    _Deallocate();

    if (dimensions[2] != 1) {
        TF_WARN("Render buffer allocated with dims <%d, %d, %d> and"
                " format %s; depth must be 1!",
                dimensions[0], dimensions[1], dimensions[2],
                TfEnum::GetName(format).c_str());
        return false;
    }

    _width = dimensions[0];
    _height = dimensions[1];
    _format = format;
    _buffer.resize(_GetBufferSize(GfVec2i(_width, _height), format));
    
    _multiSampled = multiSampled;
    if (_multiSampled) {
        _sampleBuffer.resize(_GetBufferSize(GfVec2i(_width, _height),
            _GetSampleFormat(format)));
        _sampleCount.resize(_width*_height);
    }

    return true;
}

template<typename T>
static void _WriteSample(HdFormat format, uint8_t *dst,
                         int valueComponents, T const* value)
{
    HdFormat componentFormat = HdGetComponentFormat(format);
    size_t componentCount = HdGetComponentCount(format);

    for (size_t c = 0; c < componentCount; ++c) {
        if (componentFormat == HdFormatInt32) {
            ((int32_t*)dst)[c] +=
                (c < valueComponents) ? (int32_t)(value[c]) : 0;
        } else {
            ((float*)dst)[c] +=
                (c < valueComponents) ? (float)(value[c]) : 0.0f;
        }
    }
}

template<typename T>
static void _WriteOutput(HdFormat format, uint8_t *dst,
                         int valueComponents, T const* value)
{
    HdFormat componentFormat = HdGetComponentFormat(format);
    size_t componentCount = HdGetComponentCount(format);

    for (size_t c = 0; c < componentCount; ++c) {
        if (componentFormat == HdFormatInt32) {
            ((int32_t*)dst)[c] =
                (c < valueComponents) ? (int32_t)(value[c]) : 0;
        } else if (componentFormat == HdFormatFloat32) {
            ((float*)dst)[c] =
                (c < valueComponents) ? (float)(value[c]) : 0.0f;
        } else if (componentFormat == HdFormatUNorm8) {
            ((uint8_t*)dst)[c] =
                (c < valueComponents) ? (uint8_t)(value[c] * 255.0f) : 0.0f;
        } else if (componentFormat == HdFormatSNorm8) {
            ((int8_t*)dst)[c] =
                (c < valueComponents) ? (int8_t)(value[c] * 127.0f) : 0.0f;
        }
    }
}

void
HdEmbreeRenderBuffer::Write(
    GfVec3i const& pixel, int numComponents, float const* value)
{
    size_t idx = pixel[1]*_width+pixel[0];
    if (_multiSampled) {
        size_t formatSize = HdDataSizeOfFormat(_GetSampleFormat(_format));
        uint8_t *dst = &_sampleBuffer[idx*formatSize];
        _WriteSample(_format, dst, numComponents, value);
        _sampleCount[idx]++;
    } else {
        size_t formatSize = HdDataSizeOfFormat(_format);
        uint8_t *dst = &_buffer[idx*formatSize];
        _WriteOutput(_format, dst, numComponents, value);
    }
}

void
HdEmbreeRenderBuffer::Write(
    GfVec3i const& pixel, int numComponents, int const* value)
{
    size_t idx = pixel[1]*_width+pixel[0];
    if (_multiSampled) {
        size_t formatSize = HdDataSizeOfFormat(_GetSampleFormat(_format));
        uint8_t *dst = &_sampleBuffer[idx*formatSize];
        _WriteSample(_format, dst, numComponents, value);
        _sampleCount[idx]++;
    } else {
        size_t formatSize = HdDataSizeOfFormat(_format);
        uint8_t *dst = &_buffer[idx*formatSize];
        _WriteOutput(_format, dst, numComponents, value);
    }
}

void
HdEmbreeRenderBuffer::Clear(int numComponents, float const* value)
{
    size_t formatSize = HdDataSizeOfFormat(_format);
    for (size_t i = 0; i < _width*_height; ++i) {
        uint8_t *dst = &_buffer[i*formatSize];
        _WriteOutput(_format, dst, numComponents, value);
    }

    if (_multiSampled) {
        std::fill(_sampleCount.begin(), _sampleCount.end(), 0);
        std::fill(_sampleBuffer.begin(), _sampleBuffer.end(), 0);
    }
}

void
HdEmbreeRenderBuffer::Clear(int numComponents, int const* value)
{
    size_t formatSize = HdDataSizeOfFormat(_format);
    for (size_t i = 0; i < _width*_height; ++i) {
        uint8_t *dst = &_buffer[i*formatSize];
        _WriteOutput(_format, dst, numComponents, value);
    }

    if (_multiSampled) {
        std::fill(_sampleCount.begin(), _sampleCount.end(), 0);
        std::fill(_sampleBuffer.begin(), _sampleBuffer.end(), 0);
    }
}

/*virtual*/
void
HdEmbreeRenderBuffer::Resolve()
{
    // Resolve the image buffer: find the average value per pixel by
    // dividing the summed value by the number of samples.

    if (!_multiSampled) {
        return;
    }

    HdFormat componentFormat = HdGetComponentFormat(_format);
    size_t componentCount = HdGetComponentCount(_format);
    size_t formatSize = HdDataSizeOfFormat(_format);
    size_t sampleSize = HdDataSizeOfFormat(_GetSampleFormat(_format));

    for (unsigned int i = 0; i < _width * _height; ++i) {

        int sampleCount = _sampleCount[i];
        // Skip pixels with no samples.
        if (sampleCount == 0) {
            continue;
        }

        uint8_t *dst = &_buffer[i*formatSize];
        uint8_t *src = &_sampleBuffer[i*sampleSize];
        for (size_t c = 0; c < componentCount; ++c) {
            if (componentFormat == HdFormatInt32) {
                ((int32_t*)dst)[c] = ((int32_t*)src)[c] / sampleCount;
            } else if (componentFormat == HdFormatFloat32) {
                ((float*)dst)[c] = ((float*)src)[c] / sampleCount;
            } else if (componentFormat == HdFormatUNorm8) {
                ((uint8_t*)dst)[c] = (uint8_t)
                    (((float*)src)[c] * 255.0f / sampleCount);
            } else if (componentFormat == HdFormatSNorm8) {
                ((int8_t*)dst)[c] = (int8_t)
                    (((float*)src)[c] * 127.0f / sampleCount);
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
