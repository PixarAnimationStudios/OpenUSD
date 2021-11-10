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
#ifndef EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_FRAMEBUFFER_H
#define EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_FRAMEBUFFER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/base/gf/matrix4d.h"

#include "Riley.h"

#include <vector>
#include <mutex>

class RixContext;

PXR_NAMESPACE_OPEN_SCOPE

/// A simple framebuffer used to receive display-driver output from PRMan.
/// This lives in a separate small library so it can be accessible to
/// both the hdPrman hydra plgin at the d_hydra display driver plugin,
/// without requiring either to know about the other.
class HdPrmanFramebuffer 
{
public:
    struct HdPrmanAov 
    {
        TfToken name;
        HdFormat format;
        VtValue clearValue;
        std::vector<uint32_t> pixels;
    };
    typedef std::vector<HdPrmanAov> HdPrmanAovList ;
    typedef std::vector<HdPrmanAov>::iterator HdPrmanAovIt ;

    HdPrmanFramebuffer();
    ~HdPrmanFramebuffer();

    /// Find a buffer instance with the given ID.
    /// The expectation is that the buffer will exist, so
    /// this raises a runtime error if the ID is not found.
    static HdPrmanFramebuffer* GetByID(int32_t id);
    static void Register(RixContext*);

    /// Resize the buffer.
     void Resize(int width, int height,
                int cropXMin=0, int cropYMin=0,
                int cropWidth=0, int cropHeight=0);

    void Clear();

    void AddAov(TfToken aovName, HdFormat dataType, VtValue clearValue);

    std::mutex mutex;
    HdPrmanAovList aovs;

    int w, h;
    int cropOrigin[2] = {0,0};
    int cropRes[2] = {0,0};
    int32_t id;

    // Projection matrix (for the depth output).
    GfMatrix4d proj;

    // Clear functionality.
    bool pendingClear;

    std::atomic<bool> newData;

    riley::DisplayId dspyId;
    riley::RenderTargetId rtId;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
