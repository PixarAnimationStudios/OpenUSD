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
#ifndef HDXPRMAN_RENDER_PARAM_H
#define HDXPRMAN_RENDER_PARAM_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "hdPrman/renderParam.h"
#include "hdxPrman/context.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdxPrman_RenderParam : public HdPrman_RenderParam {
public:
    HdxPrman_RenderParam(std::shared_ptr<HdxPrman_InteractiveContext> context)
        : HdPrman_RenderParam(context)
        {}
    virtual ~HdxPrman_RenderParam() = default;

    // Request edit access to the Riley scene and then return the context.
    HdPrman_Context* AcquireContext() override {
        HdxPrman_InteractiveContext* ctx =
            static_cast<HdxPrman_InteractiveContext*>(_context.get());
        ctx->StopRender();
        ctx->sceneVersion++;
        return _context.get();
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDXPRMAN_RENDER_PARAM_H
