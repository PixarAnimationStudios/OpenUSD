//
// Copyright (c) 2022-2024, NVIDIA CORPORATION.
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

#ifndef HD_USD_WRITER_RENDER_PASS_H
#define HD_USD_WRITER_RENDER_PASS_H

#include "pxr/usdImaging/plugin/hdUsdWriter/api.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdUsdWriterRenderPass
///
/// HdRenderPass represents a single render iteration, rendering a view of the
/// scene (the HdRprimCollection) for a specific viewer (the camera/viewport
/// parameters in HdRenderPassState) to the current draw target.
///
class HdUsdWriterRenderPass final : public HdRenderPass
{
public:
    /// Renderpass constructor.
    ///   \param index The render index containing scene data to render.
    ///   \param collection The initial rprim collection for this renderpass.
    HDUSDWRITER_API
    HdUsdWriterRenderPass(HdRenderIndex* index, HdRprimCollection const& collection);

    /// Renderpass destructor.
    HDUSDWRITER_API
    virtual ~HdUsdWriterRenderPass();

protected:
    /// Draw the scene with the bound renderpass state.
    ///   \param renderPassState Input parameters (including viewer parameters)
    ///                          for this renderpass.
    ///   \param renderTags Which rendertags should be drawn this pass.
    HDUSDWRITER_API
    void _Execute(HdRenderPassStateSharedPtr const& renderPassState, TfTokenVector const& renderTags) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif