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
#ifndef EXT_RMANPKG_23_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_PARAM_H
#define EXT_RMANPKG_23_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_PARAM_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "hdPrman/context.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdPrman_RenderParam : public HdRenderParam {
public:
    HdPrman_RenderParam(std::shared_ptr<HdPrman_Context> const& context)
        : _context(context)
        {}
    virtual ~HdPrman_RenderParam() = default;

    // Request edit access to the Riley scene and then return the context.
    virtual HdPrman_Context* AcquireContext() {
        return _context.get();
    }

protected:
    std::shared_ptr<HdPrman_Context> _context;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_23_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_PARAM_H
