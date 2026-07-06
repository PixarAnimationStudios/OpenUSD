//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "hdPrman/framebuffer.h"
#include "hdPrman/idMap.h"
#include "hdPrman/rixStrings.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/tf.h"
#include "pxr/base/work/loops.h"
#include "RixDspy.h"
#include "display/display.h"
#include "display/renderoutput.h"
#include <map>
#include <unordered_map>


PXR_NAMESPACE_OPEN_SCOPE

// Note: RixDspy is a singleton API so it's safe to use a static variable
//       We need to use the virtual API because we don't link to libprman
static RixDspy* s_dspy = nullptr;

////////////////////////////////////////////////////////////////////////
// PRMan Display Driver API entrypoints
////////////////////////////////////////////////////////////////////////

extern "C" DISPLAYEXPORT
PtDspyError DspyImageOpen(
    PtDspyImageHandle* handle_p,
    const char* drivername,
    const char* filename,
    int width, int height,
    int paramCount,
    const UserParameter* parameters,
    int formatCount,
    PtDspyDevFormat* format,
    PtFlagStuff* flagstuff)
{
    TF_UNUSED(drivername);
    TF_UNUSED(filename);
    if ((0 == width)||(0 == height)||(0 == formatCount)) {
        return PkDspyErrorBadParams;
    }

    // Request all pixels as F32. Requesting ID as integer seems to break
    // things? But if it's "integer" in the display channel setup, interpreting
    // it as integer seems to work.
    for (int formatIndex = 0; formatIndex < formatCount; ++formatIndex) {
        format[formatIndex].type = PkDspyFloat32;
    }
    flagstuff->flags |= PkDspyFlagsWantsEmptyBuckets;

    // Find the buffer.
    int bufferID = 0;
    s_dspy->FindIntInParamList("bufferID", &bufferID, paramCount, parameters);
    HdPrmanFramebuffer *buf = HdPrmanFramebuffer::GetByID(bufferID);
    if (!buf) {
        return PkDspyErrorBadParams;
    }

    int count = 2;
    int origin[2];
    int originalSize[2];
    originalSize[0] = 0;
    originalSize[1] = 0;

    s_dspy->FindIntsInParamList("origin", &count,
                                origin, paramCount, parameters);
    s_dspy->FindIntsInParamList("OriginalSize", &count,
                                originalSize, paramCount, parameters);

    {
        std::lock_guard<std::mutex> lock(buf->mutex);
        buf->Resize(originalSize[0], originalSize[1],
            origin[0], origin[1], width, height);
    }
    *handle_p = buf;
    return PkDspyErrorNone;
}

extern "C" DISPLAYEXPORT
PtDspyError DspyImageActiveRegion(
    PtDspyImageHandle handle,
    int xmin,
    int xmax_plus_one,
    int ymin,
    int ymax_plus_one
)
{
    HdPrmanFramebuffer* buf = reinterpret_cast<HdPrmanFramebuffer*>(handle);
    // weirdly, no longer need offset once an edit (and this callback) happens
    buf->cropOrigin[0] = 0;
    buf->cropOrigin[1] = 0;
    return PkDspyErrorNone;
}

// Transform NDC space (-1, 1) depth to window space (0, 1).
static float _ConvertAovDepth(const GfMatrix4d &m, const float depth) {
    if (std::isfinite(depth)) {
        return GfClamp(
            (m.Transform(GfVec3f(0, 0, -depth))[2] * 0.5) + 0.5, 0, 1); 
    } else {
        return 0.f;
    }
}

extern "C" DISPLAYEXPORT
PtDspyError DspyImageData(
    PtDspyImageHandle handle,
    int xmin,
    int xmax_plusone,
    int ymin,
    int ymax_plusone,
    int entrysize,
    const unsigned char* data)
{
    // XXX: This assumes the AOV list matches what was declared to riley
    // in hdPrman/renderParam.cpp CreateDisplays
    const int nComponents = entrysize / sizeof(float);

    HdPrmanFramebuffer* buf = reinterpret_cast<HdPrmanFramebuffer*>(handle);

    if (buf->w == 0 || buf->h == 0) {
        return PkDspyErrorBadParams;
    }

    if (buf->pendingClear) {
        // Lock threads while we clear the buffer.
        std::lock_guard<std::mutex> lock(buf->mutex);
        if (buf->pendingClear) {
            buf->Clear();
            buf->pendingClear = false;
        }
    }

    const int xmin_plusorigin = xmin + buf->cropOrigin[0];
    const int xmax_plusorigin = xmax_plusone + buf->cropOrigin[0];
    const int ymin_plusorigin = ymin + buf->cropOrigin[1];
    const int ymax_plusorigin = ymax_plusone + buf->cropOrigin[1];

    // Looping over aov buffers first (rather than data) reduces
    // branching and gives more consistant memory access.
    int dataOffset = 0;
    for (HdPrmanFramebuffer::AovBuffer& aovBuffer : buf->aovBuffers) {
        const HdPrmanFramebuffer::AovDesc& aovDesc = aovBuffer.desc;
        const int cc = HdGetComponentCount(aovDesc.format);

        if (aovDesc.format == HdFormatInt32) {
            int32_t* data_i32 = (int32_t*)data + dataOffset;
            for (int y = ymin_plusorigin; y < ymax_plusorigin; y++) {
                // Flip y-axis
                int offset = (buf->h - 1 - y) * buf->w + xmin_plusorigin;
                int32_t* aovData = reinterpret_cast<int32_t*>(
                    &aovBuffer.pixels[offset * cc]);
                for (int x = xmin_plusorigin; x < xmax_plusorigin; x++) {
                    *aovData = *data_i32;
                    aovData++;
                    data_i32 += nComponents;
                }
            }
        } else {
            float* data_f32 = (float*)data + dataOffset;
            if (aovDesc.name == HdAovTokens->depth) {
                for (int y = ymin_plusorigin; y < ymax_plusorigin; y++) {
                    // Flip y-axis
                    int offset = (buf->h - 1 - y) * buf->w + xmin_plusorigin;
                    float* aovData = reinterpret_cast<float*>(
                        &aovBuffer.pixels[offset * cc]);
                    for (int x = xmin_plusorigin; x < xmax_plusorigin; x++) {
                        *aovData = _ConvertAovDepth(buf->proj, *data_f32);
                        aovData += cc;
                        data_f32 += nComponents;
                    }
                }
            } else if (cc == 4) {
                for (int y = ymin_plusorigin; y < ymax_plusorigin; y++) {
                    // Flip y-axis
                    int offset = (buf->h - 1 - y) * buf->w + xmin_plusorigin;
                    float* aovData = reinterpret_cast<float*>(
                        &aovBuffer.pixels[offset * cc]);
                    for (int x = xmin_plusorigin; x < xmax_plusorigin; x++) {
                        // Premultiply color with alpha to blend pixels with
                        // background.
                        const float alphaInv = 1.f - data_f32[3];
                        GfVec4f const& clear
                            = aovDesc.clearValue.Get<GfVec4f>();
                        for (int i = 0; i < 3; i++) // RGB
                            aovData[i] = data_f32[i] + (alphaInv)*clear[i];
                        aovData[3] = data_f32[3]; // A
                        aovData += 4;
                        data_f32 += nComponents;
                    }
                }
            } else if (cc == 3) {
                for (int y = ymin_plusorigin; y < ymax_plusorigin; y++) {
                    // Flip y-axis
                    int offset = (buf->h - 1 - y) * buf->w + xmin_plusorigin;
                    float* aovData = reinterpret_cast<float*>(
                        &aovBuffer.pixels[offset * cc]);
                    for (int x = xmin_plusorigin; x < xmax_plusorigin; x++) {
                        for (int i = 0; i < 3; i++) {
                            aovData[i] = data_f32[i];
                        }
                        aovData += 3;
                        data_f32 += nComponents;
                    }
                }
            } else if (cc == 1) {
                for (int y = ymin_plusorigin; y < ymax_plusorigin; y++) {
                    // Flip y-axis
                    int offset = (buf->h - 1 - y) * buf->w + xmin_plusorigin;
                    float* aovData = reinterpret_cast<float*>(
                        &aovBuffer.pixels[offset * cc]);
                    for (int x = xmin_plusorigin; x < xmax_plusorigin; x++) {
                        *aovData = *data_f32;
                        aovData++;
                        data_f32 += nComponents;
                    }
                }
            }
        }

        dataOffset += cc;
    }

    buf->ConvertRmanIdAOVsToHydra(
        xmin_plusorigin, xmax_plusorigin,
        ymin_plusorigin, ymax_plusorigin);

    buf->newData = true;

    return PkDspyErrorNone;
}

extern "C" DISPLAYEXPORT
PtDspyError DspyImageClose (
     PtDspyImageHandle handle)
{
    TF_UNUSED(handle);
    return PkDspyErrorNone;
}

extern "C" DISPLAYEXPORT
PtDspyError DspyImageQuery (
    PtDspyImageHandle handle,
    PtDspyQueryType querytype,
    int datalen,
    void* data)
{
    TF_UNUSED(handle);
    if (!datalen || !data) {
        return PkDspyErrorBadParams;
    }
    switch (querytype) {
    case PkSizeQuery:
        {
            PtDspySizeInfo sizeInfo;
            if (size_t(datalen) > sizeof(sizeInfo))
                datalen = sizeof(sizeInfo);
            sizeInfo.width = 0;
            sizeInfo.height = 0;
            sizeInfo.aspectRatio = 1.0f;
            memcpy(data, &sizeInfo, datalen);
            return PkDspyErrorNone;;
        }
    case PkOverwriteQuery:
        {
            PtDspyOverwriteInfo overwriteInfo;
            if (size_t(datalen) > sizeof(overwriteInfo))
                datalen = sizeof(overwriteInfo);
            overwriteInfo.overwrite = 1;
            // https://renderman.pixar.com/resources/RenderMan_20/dspyNote.html
            // say this is not used
            overwriteInfo.interactive = 1;
            memcpy(data, &overwriteInfo, datalen);
            return PkDspyErrorNone;
        }
    case PkRedrawQuery:
        {
            PtDspyRedrawInfo redrawInfo;
            if (size_t(datalen) > sizeof(redrawInfo))
                datalen = sizeof(redrawInfo);
            redrawInfo.redraw = 1;
            memcpy(data, &redrawInfo, datalen);
            return PkDspyErrorNone;
        }
    default:
        return PkDspyErrorUnsupported;
    }
}

////////////////////////////////////////////////////////////////////////
// hdPrman framebuffer utility
////////////////////////////////////////////////////////////////////////

struct _BufferRegistry
{
    // Map of ID's to buffers
    using IDMap = std::map<int32_t, HdPrmanFramebuffer*>;

    std::mutex mutex;
    IDMap buffers;
    int32_t nextID = 0;
};
static TfStaticData<_BufferRegistry> _bufferRegistry;

HdPrmanFramebuffer::HdPrmanFramebuffer(HdPrman_IdMap *idMap)
  : _idMap(idMap)
  , w(0)
  , h(0)
  , cropOrigin{0,0}
  , cropRes{0,0}
  , pendingClear(true)
  , newData(false)
{
    // Add this buffer to the registry, assigning an id.
    _BufferRegistry& registry = *_bufferRegistry;
    {
        std::lock_guard<std::mutex> lock(registry.mutex);
        std::pair<_BufferRegistry::IDMap::iterator, bool> entry;
        do {
            id = registry.nextID++;
            entry = registry.buffers.insert( std::make_pair(id, this) );
        } while(!entry.second);
    }
}

HdPrmanFramebuffer::~HdPrmanFramebuffer()
{
    _BufferRegistry& registry = *_bufferRegistry;
    {
        std::lock_guard<std::mutex> lock(registry.mutex);
        registry.buffers.erase(id);
        registry.nextID = 0;
    }
}

HdPrmanFramebuffer*
HdPrmanFramebuffer::GetByID(int32_t id)
{
    _BufferRegistry& registry = *_bufferRegistry;
    {
        std::lock_guard<std::mutex> lock(registry.mutex);
        _BufferRegistry::IDMap::const_iterator i = registry.buffers.find(id);
        if (i == registry.buffers.end()) {
            TF_CODING_ERROR("HdPrmanFramebuffer: Unknown buffer ID %i\n", id);
            return nullptr;
        }
        return i->second;
    }
}

void
HdPrmanFramebuffer::CreateAovBuffers(const AovDescVector &aovDescs)
{
    aovBuffers.clear();
    aovBuffers.reserve(aovDescs.size());

    for (const AovDesc &aovDesc : aovDescs) {
        aovBuffers.push_back({aovDesc, {}});
    }

    // Reset w and h so that pixels will be allocated on the next resize
    w = 0;
    h = 0;
}

void
HdPrmanFramebuffer::Resize(int width, int height,
                           int cropXMin, int cropYMin,
                           int cropWidth, int cropHeight)
{
    if (w != width || h != height ||
        cropOrigin[0] != cropXMin || cropOrigin[1] != cropYMin ||
        cropRes[0] != cropWidth || cropRes[1] != cropHeight) {
        w = width;
        h = height;
        cropRes[0] = cropWidth;
        cropRes[1] = cropHeight;
        cropOrigin[0] = cropXMin;
        cropOrigin[1] = cropYMin;
        pendingClear = true;
        newData = true;

        for(HdPrmanFramebuffer::AovBuffer &aovBuffer : aovBuffers) {
            const int cc = HdGetComponentCount(aovBuffer.desc.format);
            aovBuffer.pixels.resize(w*h*cc);
        }
    }
}

void
HdPrmanFramebuffer::Clear()
{
    const int size = w * h;

    for(AovBuffer &aovBuffer : aovBuffers) {
        const AovDesc &aovDesc = aovBuffer.desc;
        if(aovDesc.format == HdFormatInt32) {
            const int32_t clear = aovDesc.clearValue.Get<int32_t>();
            int32_t * const data = reinterpret_cast<int32_t*>(
                aovBuffer.pixels.data());
            for(int i = 0; i < size; i++ ) {
                data[i] = clear;
            }
        } else {
            float * const data = reinterpret_cast<float*>(aovBuffer.pixels.data());
            const int cc = HdGetComponentCount(aovDesc.format);
            if (cc == 1) {
                const float clear = aovDesc.clearValue.Get<float>();
                for(int i=0; i < size; i++) {
                    data[i] = clear;
                }
            } else if (cc == 3) {
                GfVec3f const& clear = aovDesc.clearValue.Get<GfVec3f>();
                for(int i=0; i < size; i++) {
                    data[i*cc+0] = clear[0];
                    data[i*cc+1] = clear[1];
                    data[i*cc+2] = clear[2];
                }
            } else if (cc == 4) {
                GfVec4f const& clear = aovDesc.clearValue.Get<GfVec4f>();
                for(int i=0; i < size; i++) {
                    data[i*cc+0] = clear[0];
                    data[i*cc+1] = clear[1];
                    data[i*cc+2] = clear[2];
                    data[i*cc+3] = clear[3];
                }
            }
        }
    }
}

void
HdPrmanFramebuffer::ConvertRmanIdAOVsToHydra(
    int xmin, int xmax, int ymin, int ymax)
{
    // First find the primId and instanceId AOV buffers.
    // XXX: Could store these in CreateAovBuffers() instead
    HdPrmanFramebuffer::AovBuffer* primIdAovBuffer = nullptr;
    HdPrmanFramebuffer::AovBuffer* instanceIdAovBuffer = nullptr;
    for (HdPrmanFramebuffer::AovBuffer& aovBuffer : aovBuffers) {
        if (aovBuffer.desc.format == HdFormatInt32) {
            if (aovBuffer.desc.name == HdAovTokens->primId) {
                primIdAovBuffer = &aovBuffer;
            } else if (aovBuffer.desc.name == HdAovTokens->instanceId) {
                instanceIdAovBuffer = &aovBuffer;
            }
        }
    }
    if (!primIdAovBuffer || !instanceIdAovBuffer) {
        return;
    }
    for (int y = ymin; y < ymax; y++) {
        // Flip Y
        const int offset = (h-1-y) * w + xmin;
        int32_t* primIdAovData = reinterpret_cast<int32_t*>(
            &primIdAovBuffer->pixels[offset]);
        int32_t* instanceIdAovData = reinterpret_cast<int32_t*>(
            &instanceIdAovBuffer->pixels[offset]);
        for (int x = xmin; x < xmax; x++) {
            int id  = *primIdAovData;
            int id2 = *instanceIdAovData;
            auto key = HdPrman_IdMap::KeyFromAttrs(id, id2);
            HdPrman_IdMap::IdDetails details;
            if (_idMap->GetDetails(key, &details)) {
                *primIdAovData     = details.primId;
                *instanceIdAovData = details.instanceId;
            } else {
                *primIdAovData     = -1;
                *instanceIdAovData = -1;
            }
            primIdAovData++;
            instanceIdAovData++;
        }
    }
}

void
HdPrmanFramebuffer::Register(RixContext* ctx)
{
    assert(ctx);
    s_dspy = (RixDspy*)ctx->GetRixInterface(k_RixDspy);
    assert(s_dspy);
    PtDspyDriverFunctionTable dt;
    dt.Version = k_PtDriverCurrentVersion;
    dt.pOpen = DspyImageOpen;
    dt.pWrite = DspyImageData;
    dt.pClose = DspyImageClose;
    dt.pQuery = DspyImageQuery;
    dt.pActiveRegion = DspyImageActiveRegion;
    dt.pMetadata = nullptr;
    if (s_dspy->RegisterDriverTable("hydra", &dt)) {
        TF_CODING_ERROR("HdPrmanFramebuffer: Failed to register\n");
    }
}

HdPrmanFramebuffer::HdPrmanAccumulationRule
HdPrmanFramebuffer::ToAccumulationRule(RtUString name)
{
    if (name == RtUString("average")) {
        return k_accumulationRuleAverage;
    } else if (name == RtUString("min")) {
        return k_accumulationRuleMin;
    } else if (name == RtUString("max")) {
        return k_accumulationRuleMax;
    } else if (name == RtUString("zmin")) {
        return k_accumulationRuleZmin;
    } else if (name == RtUString("zmax")) {
        return k_accumulationRuleZmax;
    } else if (name == RtUString("sum")) {
        return k_accumulationRuleSum;
    } else {
        return k_accumulationRuleFilter;
    }
}

///////////////////////////////////////////////////////////////////////////////
// XPU Display Driver API entrypoints
///////////////////////////////////////////////////////////////////////////////

constexpr size_t k_invalidOffset = size_t(-1);

namespace {

class DisplayHydra final : public display::Display
{
public:
    DisplayHydra(const RtUString /*name*/, const pxrcore::ParamList& params) :
        m_width(0),
        m_height(0),
        m_surface(nullptr),
        m_alphaOffset(k_invalidOffset),
#if !defined(DISPLAY_INTERFACE_VERSION) || DISPLAY_INTERFACE_VERSION < 2
        m_weightsOffset(k_invalidOffset),
#endif
        m_buf(nullptr)
    {
        int bufferID = 0;
        params.GetInteger(RtUString("bufferID"), bufferID);
        m_buf = HdPrmanFramebuffer::GetByID(bufferID);
    }

    ~DisplayHydra() override = default;

    // Display interface
    bool Rebind(const uint32_t width, const uint32_t height, const char* /*srfaddrhandle*/,
#if !defined(DISPLAY_INTERFACE_VERSION) || DISPLAY_INTERFACE_VERSION < 2
                const void* srfaddr, const size_t /*srfsizebytes*/, const size_t weightsoffset,
                const size_t* srfoffsets, const display::RenderOutput* outputs,
#else
                const void* srfaddr, const size_t /*srfsizebytes*/, const size_t* srfoffsets,
                const size_t* /*sampleoffsets*/, const display::RenderOutput* outputs,
#endif
#if !defined(DISPLAY_INTERFACE_VERSION) || DISPLAY_INTERFACE_VERSION < 3
                const size_t noutputs) override
#else
                const size_t noutputs, const pxrcore::ParamList& metadata) override
#endif
    {
        m_surface = static_cast<const uint8_t*>(srfaddr);
        // The surface covers only the crop region (width x height).
        m_width = width;
        m_height = height;

#if !defined(DISPLAY_INTERFACE_VERSION) || DISPLAY_INTERFACE_VERSION < 2
        m_weightsOffset = weightsoffset;
#endif
        m_offsets.clear();
        m_offsets.reserve(noutputs);
        for (size_t i = 0; i < noutputs; i++)
        {
            m_offsets.push_back(srfoffsets[i]);
            if(outputs[i].name == RixStr.k_a)
            {
                m_alphaOffset = srfoffsets[i];
            }
        }

        // TODO: We should consider allocating HdPrmanFrameBuffer as "crop sized" and when we copy
        // into the full sized HdRenderBuffer copy into the correct region.

#if !defined(DISPLAY_INTERFACE_VERSION) || DISPLAY_INTERFACE_VERSION <= 3
        std::lock_guard<std::mutex> lock(m_buf->mutex);
        m_buf->Resize(width, height);
#else
        // Read the full-image dimensions and crop origin from metadata so the
        // buffer is allocated at full-frame size and pixels land at the right place.
        int cropOriginX = 0, cropOriginY = 0;
        int fullWidth = static_cast<int>(width);
        int fullHeight = static_cast<int>(height);
        {
            const RtUString kOrigin("origin");
            const RtUString kOriginalSize("OriginalSize");
            int32_t originArr[2] = {0, 0};
            int32_t originalSizeArr[2] = {0, 0};
            if (metadata.GetIntegerArray(kOrigin, 2))
            {
                const int32_t* o = metadata.GetIntegerArray(kOrigin, 2);
                originArr[0] = o[0];
                originArr[1] = o[1];
            }
            if (metadata.GetIntegerArray(kOriginalSize, 2))
            {
                const int32_t* s = metadata.GetIntegerArray(kOriginalSize, 2);
                if (s[0] > 0 && s[1] > 0)
                {
                    originalSizeArr[0] = s[0];
                    originalSizeArr[1] = s[1];
                }
            }
            cropOriginX = originArr[0];
            cropOriginY = originArr[1];
            if (originalSizeArr[0] > 0)
                fullWidth = originalSizeArr[0];
            if (originalSizeArr[1] > 0)
                fullHeight = originalSizeArr[1];
        }
        std::lock_guard<std::mutex> lock(m_buf->mutex);
        m_buf->Resize(fullWidth, fullHeight, cropOriginX, cropOriginY,
                      static_cast<int>(width), static_cast<int>(height));
#endif
        return true;
    }

    void Close() override
    {
    }

    void Notify(const uint32_t /*iteration*/,
                const uint32_t /*totaliterations*/,
                const Display::NotifyFlags flags,
                const pxrcore::ParamList& /*metadata*/) override
    {
        // We're only interested in displaying iterations.
        // renderComplete indicates the last valid set of
        // pixels and needs to be included 
        // Clear isn't interesting at all.
        if (flags != k_notifyIteration && flags != k_notifyRender)
        {
            return;
        }
        // Copy planar data into buffer
        std::lock_guard<std::mutex> lock(m_buf->mutex);

        m_buf->newData = true;

        if (m_buf->pendingClear) {
            m_buf->pendingClear = false;
            m_buf->Clear();
        }

        int offsetIdx = 0;
        for(size_t hydraAovIdx = 0;
            hydraAovIdx < m_buf->aovBuffers.size();
            hydraAovIdx++, offsetIdx++) {

            HdPrmanFramebuffer::AovBuffer &aovBuffer =
                m_buf->aovBuffers[hydraAovIdx];
            const HdPrmanFramebuffer::AovDesc &aovDesc =
                aovBuffer.desc;

#if !defined(DISPLAY_INTERFACE_VERSION) || DISPLAY_INTERFACE_VERSION < 2
            const float* const srcWeights = m_weightsOffset != k_invalidOffset ? 
                reinterpret_cast<const float*>(m_surface+m_weightsOffset) : nullptr;
#endif
            const bool shouldNormalize = aovDesc.ShouldNormalizeBySampleCount();
            const int cc = HdGetComponentCount(aovDesc.format);
            const size_t srcOffset = m_offsets[offsetIdx];

            if (aovDesc.format == HdFormatInt32) {
                const int32_t* srcInt =
                    reinterpret_cast<const int32_t*>(
                        m_surface + srcOffset);

                WorkParallelForN(m_height,
                                 [this, &srcInt, &aovBuffer]
                                 (size_t begin, size_t end) {
                    const int32_t* src = srcInt + begin * m_width;
                    for (size_t y=begin; y<end; ++y) {
                        int32_t* const aovData =
                            reinterpret_cast<int32_t*>(
                                aovBuffer.pixels.data()) +
                            (m_buf->h - 1 - (y + m_buf->cropOrigin[1])) * m_buf->w +
                            m_buf->cropOrigin[0];
                        memcpy(aovData, src, sizeof(int32_t) * m_width);
                        src += m_width;
                    }
                });
                srcInt += m_width * m_height;

            } else if (aovDesc.name == HdAovTokens->depth) {

                WorkParallelForN(m_height,
                                 [this, &aovBuffer, &srcOffset]
                                 (size_t begin, size_t end) {
                    const float* srcScalar =
                        reinterpret_cast<const float*>(
                            m_surface + srcOffset)
                        + begin * m_width;
                    for (size_t y=begin; y<end; ++y) {
                        // Flip Y
                        float* aovData =
                            reinterpret_cast<float*>(
                                aovBuffer.pixels.data()) +
                            (m_buf->h - 1 - (y + m_buf->cropOrigin[1])) * m_buf->w +
                            m_buf->cropOrigin[0];
                    for (uint32_t x = 0; x < m_width; x++) {
                        *aovData = _ConvertAovDepth(m_buf->proj,
                            *srcScalar);
                            aovData++;
                            srcScalar++;
                        }
                    }
                });

            } else if (cc == 4) {

                WorkParallelForN(m_height,
#if !defined(DISPLAY_INTERFACE_VERSION) || DISPLAY_INTERFACE_VERSION < 2
                                 [this, &aovBuffer, &srcWeights, &srcOffset, shouldNormalize]
#else
                                 [this, &aovBuffer, &srcOffset, shouldNormalize]
#endif
                                 (size_t begin, size_t end) {

#if !defined(DISPLAY_INTERFACE_VERSION) || DISPLAY_INTERFACE_VERSION < 2
                    const float* weights = srcWeights ?
                        srcWeights + begin * m_width : nullptr;
#endif
                    const float* srcColorR =
                        reinterpret_cast<const float*>(
                            m_surface + srcOffset)
                        + begin * m_width;
                    const float* srcColorA =
                        reinterpret_cast<const float*>(
                            m_surface + m_alphaOffset)
                        + begin * m_width;

                    const GfVec4f clear =
                        aovBuffer.desc.clearValue.Get<GfVec4f>();

                    for (size_t y=begin; y<end; ++y) {
                        // Flip Y and assume RGBA (i.e. 4) color width
                        float* aovData =
                            reinterpret_cast<float*>(
                                aovBuffer.pixels.data()) +
                            ((m_buf->h - 1 - (y + m_buf->cropOrigin[1])) * m_buf->w +
                             m_buf->cropOrigin[0]) * 4;

                        for (uint32_t x = 0; x < m_width; x++) {
                            float isc = 1.f;
#if !defined(DISPLAY_INTERFACE_VERSION) || DISPLAY_INTERFACE_VERSION < 2
                            if(weights && shouldNormalize && *weights > 0.f)
                                isc = 1.f / *weights;
#endif
                            const float* const srcColorG =
                                srcColorR + (m_height * m_width);
                            const float* const srcColorB =
                                srcColorG + (m_height * m_width);

                            // Premultiply color with alpha
                            // to blend pixels with background.
                            const float alphaInv = 1-(*srcColorA * isc);
                            aovData[0] = *srcColorR * isc +
                                (alphaInv) * clear[0]; // R
                            aovData[1] = *srcColorG * isc +
                                (alphaInv) * clear[1]; // G
                            aovData[2] = *srcColorB * isc +
                                (alphaInv) * clear[2]; // B
                            aovData[3] = *srcColorA * isc; // A

                            aovData += 4;
                            srcColorR++;
                            srcColorA++;
#if !defined(DISPLAY_INTERFACE_VERSION) || DISPLAY_INTERFACE_VERSION < 2
                            if (weights) weights++;
#endif
                        }
                    }
                });

                // When component count is 4 (rgba) in the hydra aov,
                // xpu's aovs will have a rgb aov followed by an alpha,
                // so need to do an extra increment to skip past the alpha
                offsetIdx++;

            } else if (cc == 1) {

                WorkParallelForN(m_height,
#if !defined(DISPLAY_INTERFACE_VERSION) || DISPLAY_INTERFACE_VERSION < 2
                                 [this, &aovBuffer, &srcWeights, &srcOffset, shouldNormalize]
#else
                                 [this, &aovBuffer, &srcOffset, shouldNormalize]
#endif
                                 (size_t begin, size_t end) {

#if !defined(DISPLAY_INTERFACE_VERSION) || DISPLAY_INTERFACE_VERSION < 2
                    const float* weights = srcWeights ?
                        srcWeights + begin * m_width : nullptr;
#endif
                    const float* srcColorR =
                        reinterpret_cast<const float*>(
                            m_surface + srcOffset)
                        + begin * m_width;
                    for (size_t y=begin; y<end; ++y) {
                        // Flip Y
                        float* aovData =
                            reinterpret_cast<float*>(
                                aovBuffer.pixels.data()) +
                            (m_buf->h - 1 - (y + m_buf->cropOrigin[1])) * m_buf->w +
                            m_buf->cropOrigin[0];

                    for (uint32_t x = 0; x < m_width; x++) {
                            float isc = 1.f;
#if !defined(DISPLAY_INTERFACE_VERSION) || DISPLAY_INTERFACE_VERSION < 2
                            if(weights && shouldNormalize && *weights > 0.f)
                                isc = 1.f / *weights;
#endif
                            *aovData = *srcColorR * isc;
                            aovData++;
                            srcColorR++;
#if !defined(DISPLAY_INTERFACE_VERSION) || DISPLAY_INTERFACE_VERSION < 2
                            if (weights) weights++;
#endif
                        }
                    }
                });

            } else if (TF_VERIFY(cc==3)) {

                WorkParallelForN(m_height,
#if !defined(DISPLAY_INTERFACE_VERSION) || DISPLAY_INTERFACE_VERSION < 2
                                 [this, &aovBuffer, &srcWeights, &srcOffset, shouldNormalize]
#else
                                 [this, &aovBuffer, &srcOffset, shouldNormalize]
#endif
                                 (size_t begin, size_t end) {

#if !defined(DISPLAY_INTERFACE_VERSION) || DISPLAY_INTERFACE_VERSION < 2
                    const float* weights = srcWeights ?
                        srcWeights + begin * m_width : nullptr;
#endif
                    const float* srcColorR =
                        reinterpret_cast<const float*>(
                            m_surface + srcOffset)
                        + begin * m_width;

                    for (size_t y=begin; y<end; ++y) {
                        // Flip Y and assume RGBA (i.e. 4) color width
                        float* aovData =
                            reinterpret_cast<float*>(aovBuffer.pixels.data()) +
                            ((m_buf->h - 1 - (y + m_buf->cropOrigin[1])) * m_buf->w +
                             m_buf->cropOrigin[0]) * 3;

                        for (uint32_t x = 0; x < m_width; x++) {
                            float isc = 1.f;
#if !defined(DISPLAY_INTERFACE_VERSION) || DISPLAY_INTERFACE_VERSION < 2
                            if(weights && shouldNormalize && *weights > 0.f)
                                isc = 1.f / *weights;
#endif

                            const float* const srcColorG =
                                srcColorR + (m_height * m_width);
                            const float* const srcColorB =
                                srcColorG + (m_height * m_width);

                            aovData[0] = *srcColorR * isc;
                            aovData[1] = *srcColorG * isc;
                            aovData[2] = *srcColorB * isc;

                            aovData += 3;
                            srcColorR++;
#if !defined(DISPLAY_INTERFACE_VERSION) || DISPLAY_INTERFACE_VERSION < 2
                            if (weights) weights++;
#endif
                        }
                    }
                });
            }
        }

        m_buf->ConvertRmanIdAOVsToHydra(0, m_width, 0, m_height);
    }

#if DISPLAY_INTERFACE_VERSION >= 2
    uint64_t GetRequirements() const override
    {
        return display::Display::k_reqFrameBuffer;
    }
#endif
private:
    uint32_t m_width;
    uint32_t m_height;
    const uint8_t* m_surface;
    size_t m_alphaOffset;
    std::vector<size_t> m_offsets;
#if !defined(DISPLAY_INTERFACE_VERSION) || DISPLAY_INTERFACE_VERSION < 2
    size_t m_weightsOffset;
#endif

    HdPrmanFramebuffer* m_buf;
};

}

// Export the current version of the Display API, necessary for binary compatibility with the
// renderer
#if DISPLAY_INTERFACE_VERSION >= 1
DISPLAYEXPORTVERSION
#endif

extern "C" DISPLAYEXPORT
display::Display* CreateDisplay(const pxrcore::UString& name,
                                const pxrcore::ParamList& params,
                                const pxrcore::ParamList& /* metadata */)
{
    return new DisplayHydra(name, params);
}

extern "C" DISPLAYEXPORT
void DestroyDisplay(const display::Display* p)
{
    delete p;
}

PXR_NAMESPACE_CLOSE_SCOPE
