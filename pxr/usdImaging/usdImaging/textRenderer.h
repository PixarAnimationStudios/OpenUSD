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
#ifndef PXR_USD_IMAGING_USD_IMAGING_TEXT_RENDERER_H
#define PXR_USD_IMAGING_USD_IMAGING_TEXT_RENDERER_H

/// \file usdImaging/text.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/textRawGlyph.h"
#include "pxr/usdImaging/usdImaging/tokens.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/usd/usd/prim.h"
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

using UsdImagingTextRendererSharedPtr = std::shared_ptr<class UsdImagingTextRenderer>;

enum class TextRendererInputType {
    TextRendererInputTypeRasterization = 0x02,
    TextRendererInputTypeControlPoints = 0x03
};

class TextRendererInput
{
public:
    virtual ~TextRendererInput()
    {}
    virtual TextRendererInputType Type() const = 0;
};

class RasterizationInput : public TextRendererInput
{
public:
    virtual ~RasterizationInput()
    {}

    virtual TextRendererInputType Type() const
    {
        return TextRendererInputType::TextRendererInputTypeRasterization;
    }
private:
    void* _data;
};

class ControlPointsInput : public TextRendererInput
{
public:
    ControlPointsInput(std::shared_ptr<UsdImagingTextRawGlyph> rawGlyph)
    {
        _rawGlyph = rawGlyph;
    }

    virtual ~ControlPointsInput()
    {}

    virtual TextRendererInputType Type() const
    {
        return TextRendererInputType::TextRendererInputTypeControlPoints;
    }

    std::shared_ptr<UsdImagingTextRawGlyph> GetRawGlyph()
    {
        return _rawGlyph;
    }
    
private:
    std::shared_ptr<UsdImagingTextRawGlyph> _rawGlyph;
};

/// \class UsdImagingTextRenderer
///
/// Base class for the text plugin.
///
class UsdImagingTextRenderer {
public:
    /// Initialize the text plugin using a text setting.
    USDIMAGING_API
    static UsdImagingTextRendererSharedPtr GetTextRenderer(const std::string&);

    USDIMAGING_API
    virtual std::string Name() = 0;

    USDIMAGING_API
    virtual TextRendererInputType RequireInput() const = 0;

    /// Generate the geometry for markupText.
    USDIMAGING_API
    virtual bool GenerateGeometryAndCoords(std::shared_ptr<TextRendererInput> input,
            VtVec3fArray& geometry,
            VtVec4fArray& textCoords) = 0;

protected:
    USDIMAGING_API
    UsdImagingTextRenderer() = default;
    USDIMAGING_API
    virtual ~UsdImagingTextRenderer();

private:
    friend class UsdImagingTextRendererRegistry;

    // This class doesn't require copy support.
    UsdImagingTextRenderer(const UsdImagingTextRenderer&)             = delete;
    UsdImagingTextRenderer&operator =(const UsdImagingTextRenderer&) = delete;
};

/// \class UsdImagingTextRendererFactoryBase
///
/// Base class for the factory of UsdImagingTextRenderer.
///
class UsdImagingTextRendererFactoryBase : public TfType::FactoryBase {
public:
    virtual UsdImagingTextRendererSharedPtr New() const = 0;
};

/// \class UsdImagingTextRendererFactory
///
/// The factory to create UsdImagingTextRenderer.
///
template <class T>
class UsdImagingTextRendererFactory : public UsdImagingTextRendererFactoryBase {
public:
    virtual UsdImagingTextRendererSharedPtr New() const
    {
        return UsdImagingTextRendererSharedPtr(new T);
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_USD_IMAGING_USD_IMAGING_TEXT_RENDERER_H
