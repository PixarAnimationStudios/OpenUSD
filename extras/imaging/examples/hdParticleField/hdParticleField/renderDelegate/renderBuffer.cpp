#include "renderBuffer.h"
#include "renderParam.h"

PXR_NAMESPACE_OPEN_SCOPE

HdParticleFieldRenderBuffer::HdParticleFieldRenderBuffer(SdfPath const& id)
    : HdRenderBuffer(id), _width(0), _height(0), _format(HdFormatInvalid), _buffer(), _mappers(0), _converged(false) {}

HdParticleFieldRenderBuffer::~HdParticleFieldRenderBuffer() = default;

void HdParticleFieldRenderBuffer::Sync(HdSceneDelegate* sceneDelegate, HdRenderParam* renderParam,
                                       HdDirtyBits* dirtyBits) {
    if (*dirtyBits & DirtyDescription) {
        // We have a background thread write directly into render buffers,
        // so we need to stop the render thread before reallocating them.
        static_cast<HdParticleFieldRenderParam*>(renderParam)->AcquireRendererForEdit();
    }

    HdRenderBuffer::Sync(sceneDelegate, renderParam, dirtyBits);
}

void HdParticleFieldRenderBuffer::Finalize(HdRenderParam* renderParam) {
    // We have a background thread write directly into render buffers,
    // so we need to stop the render thread before removing them.
    static_cast<HdParticleFieldRenderParam*>(renderParam)->AcquireRendererForEdit();

    HdRenderBuffer::Finalize(renderParam);
}

void HdParticleFieldRenderBuffer::_Deallocate() {
    // If the buffer is mapped while we're doing this, there's not a great
    // recovery path...
    TF_VERIFY(!IsMapped());

    _width  = 0;
    _height = 0;
    _format = HdFormatInvalid;
    _buffer.resize(0);
    _mappers.store(0);
    _converged.store(false);
}

size_t HdParticleFieldRenderBuffer::_GetBufferSize(GfVec2i const& dims, HdFormat format) {
    return dims[0] * dims[1] * HdDataSizeOfFormat(format);
}

OIIO::TypeDesc::BASETYPE _GetOIIOTypeDescBaseType(HdFormat format) {
    HdFormat component = HdGetComponentFormat(format);

    if (component == HdFormatFloat32) {
        return OIIO::TypeDesc::FLOAT;
    } else if (component == HdFormatInt32) {
        return OIIO::TypeDesc::INT32;
    } else if (component == HdFormatUNorm8) {
        return OIIO::TypeDesc::UINT8;
    }

    std::cerr << "Unhandled type conversion" << std::endl;

    return OIIO::TypeDesc::UNKNOWN;
}

bool HdParticleFieldRenderBuffer::Allocate(GfVec3i const& dimensions, HdFormat format, bool multiSampled) {
    _Deallocate();

    if (dimensions[2] != 1) {
        TF_WARN("Render buffer allocated with dims <%d, %d, %d> and"
                " format %s; depth must be 1!",
                dimensions[0], dimensions[1], dimensions[2], TfEnum::GetName(format).c_str());
        return false;
    }

    _width  = dimensions[0];
    _height = dimensions[1];
    _format = format;
    _buffer.resize(_GetBufferSize(GfVec2i(_width, _height), format));

    size_t nchans = HdGetComponentCount(format);

    {
        // create the OIIO ImageBuf wrapper around the hydra buffer allocated
        // memory.
        OIIO::ImageSpec spec = OIIO::ImageSpec(_width, _height, nchans,
                                               _GetOIIOTypeDescBaseType(
                                                   format));
        size_t xStride = HdDataSizeOfFormat(format);
        unsigned long yStride = xStride * _width;
        unsigned long zStride = yStride * _height;

        _buffer_imgbuf = OIIO::ImageBuf(spec, (void*)_buffer.data(), xStride, yStride, zStride);
    }

    return true;
}

template <typename T> static void _WriteOutput(HdFormat format, uint8_t* dst, size_t valueComponents, T const* value) {
    HdFormat componentFormat = HdGetComponentFormat(format);
    size_t componentCount    = HdGetComponentCount(format);

    for (size_t c = 0; c < componentCount; ++c) {
        if (componentFormat == HdFormatInt32) {
            ((int32_t*)dst)[c] = (c < valueComponents) ? (int32_t)(value[c]) : 0;
        } else if (componentFormat == HdFormatFloat16) {
            ((uint16_t*)dst)[c] = (c < valueComponents) ? GfHalf(value[c]).bits() : 0;
        } else if (componentFormat == HdFormatFloat32) {
            ((float*)dst)[c] = (c < valueComponents) ? (float)(value[c]) : 0.0f;
        } else if (componentFormat == HdFormatUNorm8) {
            ((uint8_t*)dst)[c] = (c < valueComponents) ? (uint8_t)(value[c] * 255.0f) : 0.0f;
        } else if (componentFormat == HdFormatSNorm8) {
            ((int8_t*)dst)[c] = (c < valueComponents) ? (int8_t)(value[c] * 127.0f) : 0.0f;
        }
    }
}

void HdParticleFieldRenderBuffer::Write(GfVec3i const& pixel, size_t numComponents, float const* value) {
    size_t idx        = pixel[1] * _width + pixel[0];
    size_t formatSize = HdDataSizeOfFormat(_format);
    uint8_t* dst      = &_buffer[idx * formatSize];
    _WriteOutput(_format, dst, numComponents, value);
}

void HdParticleFieldRenderBuffer::Write(GfVec3i const& pixel, size_t numComponents, int const* value) {
    size_t idx        = pixel[1] * _width + pixel[0];
    size_t formatSize = HdDataSizeOfFormat(_format);
    uint8_t* dst      = &_buffer[idx * formatSize];
    _WriteOutput(_format, dst, numComponents, value);
}

void HdParticleFieldRenderBuffer::Clear(size_t numComponents, float const* value) {
    size_t formatSize = HdDataSizeOfFormat(_format);
    for (size_t i = 0; i < _width * _height; ++i) {
        uint8_t* dst = &_buffer[i * formatSize];
        _WriteOutput(_format, dst, numComponents, value);
    }
}

void HdParticleFieldRenderBuffer::Clear(size_t numComponents, int const* value) {
    size_t formatSize = HdDataSizeOfFormat(_format);
    for (size_t i = 0; i < _width * _height; ++i) {
        uint8_t* dst = &_buffer[i * formatSize];
        _WriteOutput(_format, dst, numComponents, value);
    }
}

void HdParticleFieldRenderBuffer::Resolve() {
    // we're not multi-sampled - so we just return - the render thread actively
    // writes to the buffer
    return;
}

PXR_NAMESPACE_CLOSE_SCOPE
