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
    HdxPrmanFramebuffer *buf = HdxPrmanFramebuffer::GetByID(bufferID);
    if (!buf) {
        return PkDspyErrorBadParams;
    }
    std::lock_guard<std::mutex> lock(buf->mutex);
    buf->Resize(width, height);
    *handle_p = buf;
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
    // in hdxPrman/context.cpp CreateDisplays
    int nComponents = entrysize / sizeof(float);

    HdxPrmanFramebuffer* buf = reinterpret_cast<HdxPrmanFramebuffer*>(handle);

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

    float *data_f32 = (float*) data;
    int32_t *data_i32 = (int32_t*) data;
    for (int y=ymin; y < ymax_plusone; y++) {
        // Flip y-axis
        int offset = (buf->h-1-y)*buf->w+xmin;
        int pixelOffset = 0;
        for (int x=xmin; x < xmax_plusone; x++) {
            int dataIdx = 0;
            int32_t primIdVal = 0;
            for(HdxPrmanFramebuffer::HdPrmanAovIt it = buf->aovs.begin();
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
    typedef std::map<int32_t, HdxPrmanFramebuffer*> IDMap;

    std::mutex mutex;
    IDMap buffers;
    int32_t nextID = 0;
};
static TfStaticData<_BufferRegistry> _bufferRegistry;

HdxPrmanFramebuffer::HdxPrmanFramebuffer() : w(0), h(0)
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

HdxPrmanFramebuffer::~HdxPrmanFramebuffer()
{
    _BufferRegistry& registry = *_bufferRegistry;
    {
        std::lock_guard<std::mutex> lock(registry.mutex);
        registry.buffers.erase(id);
        registry.nextID = 0;
    }
}

HdxPrmanFramebuffer*
HdxPrmanFramebuffer::GetByID(int32_t id)
{
    _BufferRegistry& registry = *_bufferRegistry;
    {
        std::lock_guard<std::mutex> lock(registry.mutex);
        _BufferRegistry::IDMap::const_iterator i = registry.buffers.find(id);
        if (i == registry.buffers.end()) {
            TF_CODING_ERROR("HdxPrmanFramebuffer: Unknown buffer ID %i\n", id);
            return nullptr;
        }
        return i->second;
    }
}

void
HdxPrmanFramebuffer::AddAov(TfToken aovName, HdFormat format,
                            VtValue clearValue)
{
    HdPrmanAov aov;
    aov.name = aovName;
    aov.format = format;
    aov.clearValue = clearValue;
    aovs.push_back(aov);
}

void
HdxPrmanFramebuffer::Resize(int width, int height)
{
    if (w != width || h != height) {
        w = width;
        h = height;

        pendingClear = true;

        for(HdxPrmanFramebuffer::HdPrmanAovIt it = aovs.begin();
            it != aovs.end(); ++it)
        {
            int cc = HdGetComponentCount(it->format);
            it->pixels.resize(w*h*cc);
        }
    }
}

void
HdxPrmanFramebuffer::Clear()
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
HdxPrmanFramebuffer::Register(RixContext* ctx)
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
    dt.pActiveRegion = nullptr;
    dt.pMetadata = nullptr;
    if (s_dspy->RegisterDriverTable("hydra", &dt))
    {
        TF_CODING_ERROR("HdxPrmanFramebuffer: Failed to register\n");
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
