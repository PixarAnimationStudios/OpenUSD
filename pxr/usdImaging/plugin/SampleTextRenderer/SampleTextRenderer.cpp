//
// Copyright 2024 Pixar
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

#include "pxr/pxr.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/usdImaging/usdImaging/textRenderer.h"
#include "pxr/usd/usdText/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingSampleTextRenderer
///
/// The sample text render plugin.
/// It just creates a rectangle for each character.
///
class UsdImagingSampleTextRenderer : public UsdImagingTextRenderer
{
public:
    using Base = UsdImagingTextRenderer;

    UsdImagingSampleTextRenderer();

    ~UsdImagingSampleTextRenderer() override;

private:

    std::string Name() override;

    TextRendererInputType RequireInput() const override;

    /// Generate the geometries and coords for markupText and simpleText.
    bool GenerateGeometryAndCoords(std::shared_ptr<TextRendererInput> input,
            VtVec3fArray& geometries,
            VtVec4fArray& textCoords) override;
};

TF_REGISTRY_FUNCTION(TfType)
{
    TfType t = TfType::Define<UsdImagingSampleTextRenderer, TfType::Bases<UsdImagingSampleTextRenderer::Base> >();
    t.SetFactory< UsdImagingTextRendererFactory<UsdImagingSampleTextRenderer> >();
}

UsdImagingSampleTextRenderer::UsdImagingSampleTextRenderer()
{
}

/* virtual */
UsdImagingSampleTextRenderer::~UsdImagingSampleTextRenderer() = default;

std::string UsdImagingSampleTextRenderer::Name()
{
    return "SampleTextRenderer";
}

TextRendererInputType UsdImagingSampleTextRenderer::RequireInput() const
{
    return TextRendererInputType::TextRendererInputTypeControlPoints;
}

bool UsdImagingSampleTextRenderer::GenerateGeometryAndCoords(std::shared_ptr<TextRendererInput> input,
    VtVec3fArray& geometry,
    VtVec4fArray& textCoords)
{
    std::shared_ptr<ControlPointsInput> controlPointsInput = std::dynamic_pointer_cast<ControlPointsInput> (input);
    if (controlPointsInput == nullptr)
        return false;
    const std::shared_ptr<UsdImagingTextRawGlyph> rawGlyph = controlPointsInput->GetRawGlyph();
    const GfVec2i& boundBoxMin = rawGlyph->GetBoundBoxMin();
    const GfVec2i& boundBoxMax = rawGlyph->GetBoundBoxMax();

    // Just create a rectangle for each character.
    geometry.emplace_back(GfVec3f(boundBoxMin[0], boundBoxMin[1], 0.0f));
    textCoords.emplace_back(GfVec4f(0.0, 1.0, 0.0, 0.0));
    geometry.emplace_back(GfVec3f(boundBoxMin[0], boundBoxMax[1], 0.0f));
    textCoords.emplace_back(GfVec4f(0.0, 0.0, 0.0, 0.0));
    geometry.emplace_back(GfVec3f(boundBoxMax[0], boundBoxMax[1], 0.0f));
    textCoords.emplace_back(GfVec4f(1.0, 0.0, 0.0, 0.0));
    geometry.emplace_back(GfVec3f(boundBoxMin[0], boundBoxMin[1], 0.0f));
    textCoords.emplace_back(GfVec4f(0.0, 1.0, 0.0, 0.0));
    geometry.emplace_back(GfVec3f(boundBoxMax[0], boundBoxMax[1], 0.0f));
    textCoords.emplace_back(GfVec4f(1.0, 0.0, 0.0, 0.0));
    geometry.emplace_back(GfVec3f(boundBoxMax[0], boundBoxMin[1], 0.0f));
    textCoords.emplace_back(GfVec4f(1.0, 1.0, 0.0, 0.0));
    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE

