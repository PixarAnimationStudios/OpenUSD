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

#include "framebuffer.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/tf.h"
#include "RixDspy.h"
#include "hdPrman/rixStrings.h"
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

static PtDspyError HydraDspyImageOpen(
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
    std::lock_guard<std::mutex> lock(buf->mutex);
    int count = 2;
    int origin[2];
    int originalSize[2];
    originalSize[0] = 0;
    originalSize[1] = 0;

    s_dspy->FindIntsInParamList("origin", &count,
                                origin, paramCount, parameters);
    s_dspy->FindIntsInParamList("OriginalSize", &count,
                                originalSize, paramCount, parameters);
    buf->Resize(originalSize[0], originalSize[1],
                origin[0], origin[1],
                width, height);

    *handle_p = buf;
    return PkDspyErrorNone;
}

static PtDspyError HydraDspyImageActiveRegion(
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

static PtDspyError HydraDspyImageData(
    PtDspyImageHandle handle,
    int xmin,
    int xmax_plusone,
    int ymin,
    int ymax_plusone,
    int entrysize,
    const unsigned char *data)
{
    // XXX: This assumes the AOV list matches what was declared to riley
    // in hdPrman/interactiveContext.cpp CreateDisplays
    int nComponents = entrysize / sizeof(float);

    HdPrmanFramebuffer* buf = reinterpret_cast<HdPrmanFramebuffer*>(handle);

    std::lock_guard<std::mutex> lock(buf->mutex);

    if(buf->w == 0 || buf->h == 0)
    {
        return PkDspyErrorBadParams;
    }

    if (buf->pendingClear)
    {
        buf->pendingClear = false;
        buf->Clear();
    }

    int xmin_plusorigin = xmin + buf->cropOrigin[0];
    int xmax_plusorigin = xmax_plusone + buf->cropOrigin[0];
    int ymin_plusorigin = ymin + buf->cropOrigin[1];
    int ymax_plusorigin = ymax_plusone + buf->cropOrigin[1];
    
    float *data_f32 = (float*) data;
    int32_t *data_i32 = (int32_t*) data;
    for (int y=ymin_plusorigin; y < ymax_plusorigin; y++) {
        // Flip y-axis
        int offset = (buf->h-1-y)*buf->w+xmin_plusorigin;
        int pixelOffset = 0;
        for (int x=xmin_plusorigin; x < xmax_plusorigin; x++) {
            int dataIdx = 0;
            int32_t primIdVal = 0;
            for(HdPrmanFramebuffer::HdPrmanAovIt it = buf->aovs.begin();
                it != buf->aovs.end(); ++it)
            {
                int cc = HdGetComponentCount(it->format);
                if(it->format == HdFormatInt32)
                {
                    int32_t* aovData = reinterpret_cast<int32_t*>(
                        &it->pixels[offset * cc + pixelOffset * cc]);

                    if(it->name == HdAovTokens->primId)
                    {
                        aovData[0] = (data_i32[dataIdx++]-1);
                        primIdVal = aovData[0];
                    }
                    else if((it->name == HdAovTokens->instanceId ||
                             it->name == HdAovTokens->elementId) &&
                            // Note, this will always fail if primId
                            // isn't in the AOV list"
                            primIdVal == -1)
                    {
                        aovData[0] = -1;
                        dataIdx++;
                    }
                    else
                    {
                        aovData[0] = data_i32[dataIdx++];
                    }
                }
                else
                {
                    float* aovData = reinterpret_cast<float*>(
                        &it->pixels[offset*cc + pixelOffset * cc]);
                    if(it->name == HdAovTokens->depth)
                    {
                        if(std::isfinite(data_f32[dataIdx]))
                        {
                            aovData[0] =
                                buf->proj.Transform(
                                    GfVec3f(0,0,-data_f32[dataIdx++]))[2];
                        }
                        else
                        {
                            aovData[0] = -1.0f;
                        }
                    }
                    else if(cc == 4)
                    {
                        // Premultiply color with alpha
                        // to blend pixels with background.
                        float alphaInv = 1-data_f32[3];
                        GfVec4f const& clear = it->clearValue.Get<GfVec4f>();
                        aovData[0] = data_f32[dataIdx++] +
                            (alphaInv) * clear[0]; // R
                        aovData[1] = data_f32[dataIdx++] +
                            (alphaInv) * clear[1]; // G
                        aovData[2] = data_f32[dataIdx++] +
                            (alphaInv) * clear[2]; // B
                        aovData[3] = data_f32[dataIdx++]; // A
                    }
                    else
                    {
                        aovData[0] = data_f32[dataIdx++];
                        if(cc >=3)
                        {
                            aovData[1] = data_f32[dataIdx++];
                            aovData[2] = data_f32[dataIdx++];
                        }
                    }
                }
            }
            pixelOffset++;
            data_f32 += nComponents;
            data_i32 += nComponents;
        }
    }
    return PkDspyErrorNone;
}

static PtDspyError HydraDspyImageClose (
     PtDspyImageHandle handle)
{
    TF_UNUSED(handle);
    return PkDspyErrorNone;
}

static PtDspyError HydraDspyImageQuery (
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
    typedef std::map<int32_t, HdPrmanFramebuffer*> IDMap;

    std::mutex mutex;
    IDMap buffers;
    int32_t nextID = 0;
};
static TfStaticData<_BufferRegistry> _bufferRegistry;

HdPrmanFramebuffer::HdPrmanFramebuffer() : w(0), h(0)
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
HdPrmanFramebuffer::AddAov(TfToken aovName, HdFormat format,
                            VtValue clearValue)
{
    HdPrmanAov aov;
    aov.name = aovName;
    aov.format = format;
    aov.clearValue = clearValue;
    aovs.push_back(aov);
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

        for(HdPrmanFramebuffer::HdPrmanAovIt it = aovs.begin();
            it != aovs.end(); ++it)
        {
            int cc = HdGetComponentCount(it->format);
            it->pixels.resize(w*h*cc);
        }
    }
}

void
HdPrmanFramebuffer::Clear()
{
    int size = w * h;

    for (HdPrmanAovIt it = aovs.begin(); it != aovs.end(); ++it)
    {
        if(it->format == HdFormatInt32)
        {
            int32_t clear = it->clearValue.Get<int32_t>();
            int32_t *data = reinterpret_cast<int32_t*>(it->pixels.data());
            for(int i = 0; i < size; i++ )
            {
                data[i] = clear;
            }
        }
        else
        {
            float *data = reinterpret_cast<float*>(it->pixels.data());
            int cc = HdGetComponentCount(it->format);
            if(cc == 1)
            {
                float clear = it->clearValue.Get<float>();
                for(int i=0; i < size; i++)
                {
                    data[i] = clear;
                }
            }
            else if(cc == 3)
            {
                GfVec3f const& clear = it->clearValue.Get<GfVec3f>();
                for(int i=0; i < size; i++)
                {
                    data[i*cc+0] = clear[0];
                    data[i*cc+1] = clear[1];
                    data[i*cc+2] = clear[2];
                }
            }
            else if(cc == 4)
            {
                GfVec4f const& clear = it->clearValue.Get<GfVec4f>();
                for(int i=0; i < size; i++)
                {
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
HdPrmanFramebuffer::Register(RixContext* ctx)
{
    assert(ctx);
    s_dspy = (RixDspy*)ctx->GetRixInterface(k_RixDspy);
    assert(s_dspy);
    PtDspyDriverFunctionTable dt;
    dt.Version = k_PtDriverCurrentVersion;
    dt.pOpen = HydraDspyImageOpen;
    dt.pWrite = HydraDspyImageData;
    dt.pClose = HydraDspyImageClose;
    dt.pQuery = HydraDspyImageQuery;
    dt.pActiveRegion = HydraDspyImageActiveRegion;
    dt.pMetadata = nullptr;
    if (s_dspy->RegisterDriverTable("hydra", &dt))
    {
        TF_CODING_ERROR("HdPrmanFramebuffer: Failed to register\n");
    }
}

///////////////////////////////////////////////////////////////////////////////
// XPU Display Driver API entrypoints
///////////////////////////////////////////////////////////////////////////////

constexpr size_t k_invalidOffset = size_t(-1);

class DisplayHydra final : public display::Display
{
public:
    DisplayHydra(const RtUString /*name*/, const pxrcore::ParamList& params) :
        m_width(0),
        m_height(0),
        m_surface(nullptr),
        m_alphaOffset(k_invalidOffset),
        m_weightsOffset(k_invalidOffset),
        m_buf(nullptr)
    {
        int bufferID = 0;
        params.GetInteger(RtUString("bufferID"), bufferID);
        m_buf = HdPrmanFramebuffer::GetByID(bufferID);
    }

    ~DisplayHydra()
    {
    }

    // Display interface
    bool Rebind(const uint32_t width, const uint32_t height,
                const char* /*srfaddrhandle*/, const void* srfaddr,
                const size_t /*srfsizebytes*/, const size_t weightsoffset,
                const size_t* offsets, const display::RenderOutput* outputs,
                const size_t noutputs)
    {
        m_surface = static_cast<const uint8_t*>(srfaddr);
        m_width = width;
        m_height = height;

        m_weightsOffset = weightsoffset;
        m_offsets.clear();
        m_offsets.reserve(noutputs);
        for (size_t i = 0; i < noutputs; i++)
        {
            m_offsets.push_back(offsets[i]);
            if(outputs[i].name == RixStr.k_a)
            {
                m_alphaOffset = offsets[i];
            }
        }

        m_buf->Resize(width, height);
        return true;
    }

    void Close()
    {
    }

    void Notify(const uint32_t /*iteration*/,
                const uint32_t /*totaliterations*/,
                const Display::NotifyFlags flags,
                const pxrcore::ParamList& /*metadata*/)
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

        if (m_buf->pendingClear) {
            m_buf->pendingClear = false;
            m_buf->Clear();
        }

        int hydraAovIdx = 0;
        int offsetIdx = 0;
        int primIdIdx = -1;
        for(HdPrmanFramebuffer::HdPrmanAovIt it = m_buf->aovs.begin();
            it != m_buf->aovs.end(); ++it, ++hydraAovIdx, ++offsetIdx)
        {
            const float* srcWeights =
                reinterpret_cast<const float*>(m_surface+m_weightsOffset);


            int cc = HdGetComponentCount(it->format);
            size_t srcOffset = m_offsets[offsetIdx];
            if (it->format == HdFormatInt32)
            {
                const int32_t* srcInt =
                    reinterpret_cast<const int32_t*>(
                        m_surface + srcOffset);

                // This code manipulates primId, elementId, and
                // instanceId values to match similar code in the RIS
                // version of the dspy driver, further up in this file.
                // The intention is to subtract one from the primId aov
                // (which maps to 'id' in prman),
                // and then when the value is -1, also set the values of
                // elementId and instanceId to -1
                // (those are faceindex and id2 in prman).
                // The primId, elementId, and instanceId names are used
                // by flow and usdview, and those apps expect the prman
                // id values to have been manipulated in this way.
                // NB: There's an assumption here that the primId
                // aov is processed before elementId and instanceId aovs
                if (it->name == HdAovTokens->primId)
                {
                    primIdIdx = hydraAovIdx;

                    for (uint32_t y = 0; y < m_height; y++)
                    {
                        // Flip Y
                        int32_t* aovData =
                            reinterpret_cast<int32_t*>(it->pixels.data()) +
                            ((m_buf->h-1-y)*m_buf->w);

                        for (uint32_t x = 0; x < m_width; x++)
                        {
                            *aovData = (*srcInt) - 1;
                            srcInt++;
                            aovData++;
                        }
                    }
                }
                else if (it->name == HdAovTokens->instanceId ||
                                    it->name == HdAovTokens->elementId)
                {
                    for (uint32_t y = 0; y < m_height; y++)
                    {
                        // Flip Y
                        int32_t* aovData =
                            reinterpret_cast<int32_t*>(it->pixels.data()) +
                            ((m_buf->h-1-y)*m_buf->w);
                        int32_t* primIdData =
                            reinterpret_cast<int32_t*>(
                                m_buf->aovs[primIdIdx].pixels.data()) +
                            ((m_buf->h-1-y)*m_buf->w);

                        for (uint32_t x = 0; x < m_width; x++)
                        {
                            if(*primIdData == -1)
                            {
                                *aovData = -1;
                            }
                            else
                            {
                                *aovData = *srcInt;
                            }
                            srcInt++;
                            aovData++;
                            primIdData++;
                        }
                    }
                }
                else
                {
                    for (uint32_t y = 0; y < m_height; y++)
                    {
                        // Flip Y
                        int32_t* aovData =
                            reinterpret_cast<int32_t*>(it->pixels.data()) +
                            ((m_buf->h-1-y)*m_buf->w);
                        memcpy(aovData, srcInt, sizeof(int32_t) * m_width);
                        srcInt += m_width;
                    }
                }   
                    
            }
            else
            {
                if(it->name == HdAovTokens->depth)
                {
                    const float* srcScalar =
                        reinterpret_cast<const float*>(m_surface +
                                                       srcOffset);

                    for (uint32_t y = 0; y < m_height; y++)
                    {
                        // Flip Y
                        float* aovData =
                            reinterpret_cast<float*>(it->pixels.data()) +
                            ((m_buf->h-1-y)*m_buf->w);

                        for (uint32_t x = 0; x < m_width; x++)
                        {
                            float value = *srcScalar;
                            if(std::isfinite(value))
                            {
                                value =
                                    m_buf->proj.Transform(
                                        GfVec3f(0,0,-value))[2];
                            }
                            else
                            {
                                value = -1.0f;
                            }
                            *aovData = value;
                            
                            aovData++;
                            srcScalar++;
                        }
                    }
                }
                else if( cc == 4 )
                {
                    const float* srcColorR =
                        reinterpret_cast<const float*>(m_surface +
                                                       srcOffset);
                    const float* srcColorA =
                        reinterpret_cast<const float*>(
                            m_surface + m_alphaOffset);

                    GfVec4f clear =
                        it->clearValue.Get<GfVec4f>();

                    for (uint32_t y = 0; y < m_height; y++)
                    {
                        // Flip Y and assume RGBA (i.e. 4) color width
                        float* aovData =
                            reinterpret_cast<float*>(it->pixels.data()) +
                            ((m_buf->h-1-y)*m_buf->w)*4;

                        for (uint32_t x = 0; x < m_width; x++)
                        {
                            float isc = 1.f;
                            if(*srcWeights > 0.f)
                                isc = 1.f / *srcWeights;

                            const float* srcColorG =
                                srcColorR + (m_height * m_width);
                            const float* srcColorB =
                                srcColorG + (m_height * m_width);

                            // Premultiply color with alpha
                            // to blend pixels with background.
                            float alphaInv = 1-(*srcColorA * isc);
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
                            srcWeights++;
                        }
                    }

                    // When component count is 4 (rgba) in the hydra aov,
                    // xpu's aovs will have a rgb aov followed by an alpha,
                    // so need to do an extra increment to skip past the alpha
                    offsetIdx++;
                }
                else if (cc == 1)
                {
                    const float* srcColorR =
                        reinterpret_cast<const float*>(m_surface +
                                                       srcOffset);

                    for (uint32_t y = 0; y < m_height; y++)
                    {
                        // Flip Y
                        float* aovData =
                            reinterpret_cast<float*>(it->pixels.data()) +
                            ((m_buf->h-1-y)*m_buf->w);

                        for (uint32_t x = 0; x < m_width; x++)
                        {
                            float isc = 1.f;
                            if(*srcWeights > 0.f)
                                isc = 1.f / *srcWeights;

                            *aovData = *srcColorR * isc;

                            aovData++;
                            srcColorR++;
                            srcWeights++;
                        }
                    }
                }                
                else
                {
                    assert(cc == 3);
                    const float* srcColorR =
                        reinterpret_cast<const float*>(m_surface +
                                                       srcOffset);

                    for (uint32_t y = 0; y < m_height; y++)
                    {
                        // Flip Y and assume RGBA (i.e. 4) color width
                        float* aovData =
                            reinterpret_cast<float*>(it->pixels.data()) +
                            ((m_buf->h-1-y)*m_buf->w)*cc;

                        for (uint32_t x = 0; x < m_width; x++)
                        {
                            float isc = 1.f;
                            if(*srcWeights > 0.f)
                                isc = 1.f / *srcWeights;

                            const float* srcColorG =
                                srcColorR + (m_height * m_width);
                            const float* srcColorB =
                                srcColorG + (m_height * m_width);

                            aovData[0] = *srcColorR * isc;
                            aovData[1] = *srcColorG * isc;
                            aovData[2] = *srcColorB * isc;

                            aovData += cc;
                            srcColorR++;
                            srcWeights++;
                        }
                    }
                }
            }
        }
    }

private:
    uint32_t m_width;
    uint32_t m_height;
    const uint8_t* m_surface;
    size_t m_alphaOffset;
    std::vector<size_t> m_offsets;
    size_t m_weightsOffset;

    HdPrmanFramebuffer* m_buf;
};

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

