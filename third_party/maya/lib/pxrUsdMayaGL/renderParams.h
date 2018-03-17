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

/// \file pxrUsdMayaGL/renderParams.h

#include "pxr/pxr.h"

#include "pxrUsdMayaGL/api.h"

#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/token.h"

#include <boost/functional/hash.hpp>


PXR_NAMESPACE_OPEN_SCOPE


struct PxrMayaHdRenderParams
{
    // Raster Params
    //
    bool enableLighting = true;

    // Color Params
    //
    GfVec4f overrideColor = GfVec4f(0.0f);
    GfVec4f wireframeColor = GfVec4f(0.0f);

    /// Custom bucketing on top of the regular bucketing based on render params.
    /// Leave this as the empty token if you want to use the default bucket for
    /// these params, along with its associated Hydra tasks.
    /// Set this to a non-empty token if you want to render with separate
    /// Hydra tasks, since these are allocated on a per-bucket basis.
    TfToken customBucketName;

    /// Helper function to find a batch key for the render params
    size_t Hash() const
    {
        size_t hash = size_t(enableLighting);
        boost::hash_combine(hash, overrideColor);
        boost::hash_combine(hash, wireframeColor);
        boost::hash_combine(hash, customBucketName);

        return hash;
    }
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif // PXRUSDMAYAGL_RENDER_PARAMS_H
