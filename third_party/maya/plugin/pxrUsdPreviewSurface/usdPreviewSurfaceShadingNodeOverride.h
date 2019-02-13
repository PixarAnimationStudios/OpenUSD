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
#ifndef PXRUSDPREVIEWSURFACE_USD_PREVIEW_SURFACE_SHADING_NODE_OVERRIDE_H
#define PXRUSDPREVIEWSURFACE_USD_PREVIEW_SURFACE_SHADING_NODE_OVERRIDE_H

/// \file pxrUsdPreviewSurface/usdPreviewSurfaceShadingNodeOverride.h

#include "pxr/pxr.h"
#include "pxrUsdPreviewSurface/api.h"

#include "pxr/base/tf/staticTokens.h"

#include <maya/MObject.h>
#include <maya/MPxSurfaceShadingNodeOverride.h>
#include <maya/MString.h>
#include <maya/MViewport2Renderer.h>


PXR_NAMESPACE_OPEN_SCOPE


#define PXRUSDPREVIEWSURFACE_USD_PREVIEW_SURFACE_SHADING_NODE_OVERRIDE_TOKENS \
    ((Float4ToFloatXFragmentName, "float4ToFloatX")) \
    ((Float4ToFloatYFragmentName, "float4ToFloatY")) \
    ((Float4ToFloatZFragmentName, "float4ToFloatZ")) \
    ((Float4ToFloatWFragmentName, "float4ToFloatW")) \
    ((LightingStructFragmentName, "lightingContributions")) \
    ((LightingFragmentName, "usdPreviewSurfaceLighting")) \
    ((CombinerFragmentName, "usdPreviewSurfaceCombiner")) \
    ((SurfaceFragmentGraphName, "usdPreviewSurface"))

TF_DECLARE_PUBLIC_TOKENS(
    PxrMayaUsdPreviewSurfaceShadingNodeTokens,
    PXRUSDPREVIEWSURFACE_API,
    PXRUSDPREVIEWSURFACE_USD_PREVIEW_SURFACE_SHADING_NODE_OVERRIDE_TOKENS);


class PxrMayaUsdPreviewSurfaceShadingNodeOverride :
        public MHWRender::MPxSurfaceShadingNodeOverride
{
    public:

        PXRUSDPREVIEWSURFACE_API
        static MHWRender::MPxSurfaceShadingNodeOverride* creator(
                const MObject& obj);

        PXRUSDPREVIEWSURFACE_API
        PxrMayaUsdPreviewSurfaceShadingNodeOverride(const MObject& obj);
        PXRUSDPREVIEWSURFACE_API
        ~PxrMayaUsdPreviewSurfaceShadingNodeOverride() override;

        // MPxSurfaceShadingNodeOverride Overrides.

        PXRUSDPREVIEWSURFACE_API
        MString primaryColorParameter() const override;

        PXRUSDPREVIEWSURFACE_API
        MString transparencyParameter() const override;

        PXRUSDPREVIEWSURFACE_API
        MString bumpAttribute() const override;


        // MPxShadingNodeOverride Overrides.

        PXRUSDPREVIEWSURFACE_API
        MHWRender::DrawAPI supportedDrawAPIs() const override;

        PXRUSDPREVIEWSURFACE_API
        MString fragmentName() const override;

        PXRUSDPREVIEWSURFACE_API
        void getCustomMappings(
                MHWRender::MAttributeParameterMappingList& mappings) override;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
