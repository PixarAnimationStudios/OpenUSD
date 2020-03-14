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
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/tf.h"
#include "RixDspy.h"
#include <map>

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
    // XXX: This assumes the AOV list is the following:
    // Ci (offset = 0)
    // a (offset = 12)
    // z (offset = 16)
    // id (offset = 20)
    // id2 (offset = 24)
    // __faceindex (offset = 28)
    // This needs to be kept in sync with hdxPrman/context.cpp.
    int nComponents = entrysize / 4;
    if (nComponents != 8) {
        return PkDspyErrorBadParams;
    }

    HdxPrmanFramebuffer* buf = reinterpret_cast<HdxPrmanFramebuffer*>(handle);
    std::lock_guard<std::mutex> lock(buf->mutex);

    if (buf->pendingClear) {
        buf->pendingClear = false;
        int size = buf->w * buf->h;
        float *depth = &buf->depth[0];
        float *color = buf->color.data();
        int32_t *id = &buf->primId[0];
        int32_t *id2 = &buf->instanceId[0];
        int32_t *id3 = &buf->elementId[0];
        for (int i = 0; i < size; i++) {
            color[i*4+0] = buf->clearColor[0];
            color[i*4+1] = buf->clearColor[1];
            color[i*4+2] = buf->clearColor[2];
            color[i*4+3] = buf->clearColor[3];
            depth[i] = buf->clearDepth;
            id[i] = buf->clearId;
            id2[i] = buf->clearId;
            id3[i] = buf->clearId;
        }
    }

    float *data_f32 = (float*) data;
    int32_t *data_i32 = (int32_t*) data;
    for (int y=ymin; y < ymax_plusone; y++) {
        // Flip y-axis
        float* color = &buf->color[((buf->h-1-y)*buf->w+xmin)*4];
        float* depth = &buf->depth[((buf->h-1-y)*buf->w+xmin)];
        int32_t* primId = &buf->primId[((buf->h-1-y)*buf->w+xmin)];
        int32_t* instanceId = &buf->instanceId[((buf->h-1-y)*buf->w+xmin)];
        int32_t* elementId = &buf->elementId[((buf->h-1-y)*buf->w+xmin)];
        for (int x=xmin; x < xmax_plusone; x++) {
            color[0] = data_f32[0]; // red
            color[1] = data_f32[1]; // green
            color[2] = data_f32[2]; // blue
            color[3] = data_f32[3]; // alpha
            // XXX: We shouldn't be getting true inf from prman?
            if (std::isfinite(data_f32[4])) {
                // Project the depth to NDC and then transform it to clip space
                // assuming a depth range of [0,1].
                depth[0] = (buf->proj.Transform(GfVec3f(0,0,-data_f32[4]))[2]
                            + 1.0f) / 2.0f;
            }
            primId[0] = (data_i32[5]-1);
            if (primId[0] == -1) {
                instanceId[0] = -1;
                elementId[0] = -1;
            } else {
                instanceId[0] = data_i32[6];
                elementId[0] = data_i32[7];
            }
            color += 4;
            depth += 1;
            primId += 1;
            instanceId += 1;
            elementId += 1;
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
//    HdxPrmanFramebuffer* buf = reinterpret_cast<HdxPrmanFramebuffer*>(handle);
//    delete buf;
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

HdxPrmanFramebuffer::HdxPrmanFramebuffer()
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
HdxPrmanFramebuffer::Resize(int width, int height)
{
    w = width;
    h = height;

    color.resize(w*h*4);
    depth.resize(w*h);
    primId.resize(w*h);
    instanceId.resize(w*h);
    elementId.resize(w*h);

    pendingClear = true;
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
