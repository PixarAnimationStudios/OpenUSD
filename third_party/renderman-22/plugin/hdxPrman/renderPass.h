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
#ifndef HDXPRMAN_RENDER_PASS_H
#define HDXPRMAN_RENDER_PASS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hdx/compositor.h"
#include "pxr/base/gf/matrix4d.h"
#include "Riley.h"

PXR_NAMESPACE_OPEN_SCOPE

struct HdPrman_Context;
struct HdxPrman_InteractiveContext;

class HdxPrman_RenderPass final : public HdRenderPass {
public:
    HdxPrman_RenderPass(HdRenderIndex *index,
                      HdRprimCollection const &collection,
                      std::shared_ptr<HdPrman_Context> context);
    virtual ~HdxPrman_RenderPass();

    virtual bool IsConverged() const override;

protected:
    virtual void _Execute(HdRenderPassStateSharedPtr const& renderPassState,
                          TfTokenVector const &renderTags) override;

private:
    unsigned int _width;
    unsigned int _height;
    bool _converged;
    std::shared_ptr<HdPrman_Context> _context;
    // Down-casted version of the context.
    std::shared_ptr<HdxPrman_InteractiveContext> _interactiveContext;
    int _lastRenderedVersion;
    int _lastSettingsVersion;

    GfMatrix4d _lastProj, _lastViewToWorldMatrix;

    HdxCompositor _compositor;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDXPRMAN_RENDER_PASS_H
