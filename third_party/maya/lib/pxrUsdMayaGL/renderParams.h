//
// Copyright 2018 Pixar
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
#ifndef PXRUSDMAYAGL_RENDER_PARAMS_H
#define PXRUSDMAYAGL_RENDER_PARAMS_H

/// \file renderParams.h

#include "pxr/pxr.h"

#include "pxrUsdMayaGL/api.h"

#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/token.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/usd/usd/timeCode.h"

#include <boost/functional/hash.hpp>


PXR_NAMESPACE_OPEN_SCOPE


struct PxrMayaHdRenderParams
{
    // USD Params
    //
    UsdTimeCode timeCode = UsdTimeCode::Default();
    uint8_t refineLevel = 0u;
    TfToken geometryCol = HdTokens->geometry;

    // Raster Params
    //
    bool enableLighting = true;

    // Geometry Params
    //
    HdCullStyle cullStyle = HdCullStyleNothing;
    TfToken drawRepr = HdTokens->refined;
    TfTokenVector renderTags;

    // Color Params
    //
    GfVec4f overrideColor = GfVec4f(0.0f);
    GfVec4f wireframeColor = GfVec4f(0.0f);

    /// \brief Helper function to find a batch key for the render params
    size_t Hash() const
    {
        size_t hash = (refineLevel << 1) + enableLighting;
        boost::hash_combine(hash, timeCode);
        boost::hash_combine(hash, geometryCol);
        boost::hash_combine(hash, cullStyle);
        boost::hash_combine(hash, drawRepr);
        boost::hash_combine(hash, overrideColor);
        boost::hash_combine(hash, wireframeColor);

        return hash;
    }
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif // PXRUSDMAYAGL_RENDER_PARAMS_H
