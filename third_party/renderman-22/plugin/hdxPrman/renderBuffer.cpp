//
// Copyright 2019 Pixar
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
#include "hdxPrman/renderBuffer.h"
#include "pxr/base/gf/half.h"

PXR_NAMESPACE_OPEN_SCOPE

HdxPrmanRenderBuffer::HdxPrmanRenderBuffer(SdfPath const& id)
    : HdRenderBuffer(id)
    , _width(0)
    , _height(0)
    , _format(HdFormatInvalid)
    , _buffer()
    , _mappers(0)
    , _converged(false)
{
}

HdxPrmanRenderBuffer::~HdxPrmanRenderBuffer()
{
}

void
HdxPrmanRenderBuffer::_Deallocate()
{
    // If the buffer is mapped while we're doing this, there's not a great
    // recovery path...
    TF_VERIFY(!IsMapped());

    _width = 0;
    _height = 0;
    _format = HdFormatInvalid;
    _buffer.resize(0);
    _mappers.store(0);
    _converged.store(false);
}

/*static*/
size_t
HdxPrmanRenderBuffer::_GetBufferSize(GfVec2i const &dims, HdFormat format)
{
    return dims[0] * dims[1] * HdDataSizeOfFormat(format);
}

/*virtual*/
bool
HdxPrmanRenderBuffer::Allocate(GfVec3i const& dimensions,
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
    
    return true;
}

template<typename T>
static void _ConvertPixel(HdFormat dstFormat, uint8_t *dst,
                          HdFormat srcFormat, uint8_t const *src)
{
    HdFormat srcComponentFormat = HdGetComponentFormat(srcFormat);
    HdFormat dstComponentFormat = HdGetComponentFormat(dstFormat);
    size_t srcComponentCount = HdGetComponentCount(srcFormat);
    size_t dstComponentCount = HdGetComponentCount(dstFormat);

    for (size_t c = 0; c < dstComponentCount; ++c) {
        T readValue = 0;
        if (c < srcComponentCount) {
            if (srcComponentFormat == HdFormatInt32) {
                readValue = ((int32_t*)src)[c];
            } else if (srcComponentFormat == HdFormatFloat16) {
                GfHalf half;
                half.setBits(((uint16_t*)src)[c]);
                readValue = static_cast<float>(half);
            } else if (srcComponentFormat == HdFormatFloat32) {
                readValue = ((float*)src)[c];
            } else if (srcComponentFormat == HdFormatUNorm8) {
                readValue = ((uint8_t*)src)[c] / 255.0f;
            } else if (srcComponentFormat == HdFormatSNorm8) {
                readValue = ((int8_t*)src)[c] / 127.0f;
            }
        }

        if (dstComponentFormat == HdFormatInt32) {
            ((int32_t*)dst)[c] = readValue;
        } else if (dstComponentFormat == HdFormatFloat16) {
            ((uint16_t*)dst)[c] = GfHalf(float(readValue)).bits();
        } else if (dstComponentFormat == HdFormatFloat32) {
            ((float*)dst)[c] = readValue;
        } else if (dstComponentFormat == HdFormatUNorm8) {
            ((uint8_t*)dst)[c] = (readValue * 255.0f);
        } else if (dstComponentFormat == HdFormatSNorm8) {
            ((int8_t*)dst)[c] = (readValue * 127.0f);
        }
    }
}

void
HdxPrmanRenderBuffer::Blit(HdFormat format, int width, int height,
                           int offset, int stride, uint8_t const* data)
{
    size_t pixelSize = HdDataSizeOfFormat(_format);
    if (_format == format) {
        if (static_cast<unsigned int>(width) == _width 
            && static_cast<unsigned int>(height) == _height) {
            // Awesome! Blit line by line.
            for (unsigned int j = 0; j < _height; ++j) {
                memcpy(&_buffer[(j * _width) * pixelSize],
                       &data[(j * stride + offset) * pixelSize],
                       _width * pixelSize);
            }
        } else {
            // Ok...  Blit pixel by pixel, with nearest point sampling.
            float scalei = width/float(_width);
            float scalej = height/float(_height);
            for (unsigned int j = 0; j < _height; ++j) {
                for (unsigned int i = 0; i < _width; ++i) {
                    unsigned int ii = scalei * i;
                    unsigned int jj = scalej * j;
                    memcpy(&_buffer[(j * _width + i) * pixelSize],
                           &data[(jj * stride + offset + ii) * pixelSize],
                           pixelSize);
                }
            }
        }
    } else {
        // D'oh.  Convert pixel by pixel, with nearest point sampling.
        // If src and dst are both int-based, don't round trip to float.
        bool convertAsInt =
            (HdGetComponentFormat(format) == HdFormatInt32) &&
            (HdGetComponentFormat(_format) == HdFormatInt32);

        float scalei = width/float(_width);
        float scalej = height/float(_height);
        for (unsigned int j = 0; j < _height; ++j) {
            for (unsigned int i = 0; i < _width; ++i) {
                unsigned int ii = scalei * i;
                unsigned int jj = scalej * j;
                if (convertAsInt) {
                    _ConvertPixel<int32_t>(
                        _format, static_cast<uint8_t*>(
                            &_buffer[(j * _width + i) * pixelSize]),
                        format,
                            &data[(jj * stride + offset + ii) * pixelSize]);
                } else {
                    _ConvertPixel<float>(
                        _format, static_cast<uint8_t*>(
                            &_buffer[(j * _width + i) * pixelSize]),
                        format,
                            &data[(jj * stride + offset + ii) * pixelSize]);
                }
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
