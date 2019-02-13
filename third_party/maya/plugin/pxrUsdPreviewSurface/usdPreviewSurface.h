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
#ifndef PXRUSDPREVIEWSURFACE_USD_PREVIEW_SURFACE_H
#define PXRUSDPREVIEWSURFACE_USD_PREVIEW_SURFACE_H

/// \file pxrUsdPreviewSurface/usdPreviewSurface.h

#include "pxr/pxr.h"
#include "pxrUsdPreviewSurface/api.h"

#include "pxr/base/tf/staticTokens.h"

#include <maya/MDataBlock.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPxNode.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>


PXR_NAMESPACE_OPEN_SCOPE


#define PXRUSDPREVIEWSURFACE_USD_PREVIEW_SURFACE_TOKENS \
    ((MayaTypeName, "pxrUsdPreviewSurface")) \
    ((ClearcoatAttrName, "clearcoat")) \
    ((ClearcoatRoughnessAttrName, "clearcoatRoughness")) \
    ((DiffuseColorAttrName, "diffuseColor")) \
    ((DisplacementAttrName, "displacement")) \
    ((EmissiveColorAttrName, "emissiveColor")) \
    ((IorAttrName, "ior")) \
    ((MetallicAttrName, "metallic")) \
    ((NormalAttrName, "normal")) \
    ((OcclusionAttrName, "occlusion")) \
    ((OpacityAttrName, "opacity")) \
    ((RoughnessAttrName, "roughness")) \
    ((SpecularColorAttrName, "specularColor")) \
    ((UseSpecularWorkflowAttrName, "useSpecularWorkflow")) \
    ((OutColorAttrName, "outColor")) \
    ((OutTransparencyAttrName, "outTransparency"))

TF_DECLARE_PUBLIC_TOKENS(
    PxrMayaUsdPreviewSurfaceTokens,
    PXRUSDPREVIEWSURFACE_API,
    PXRUSDPREVIEWSURFACE_USD_PREVIEW_SURFACE_TOKENS);


class PxrMayaUsdPreviewSurface : public MPxNode
{
    public:
        PXRUSDPREVIEWSURFACE_API
        static const MTypeId typeId;
        PXRUSDPREVIEWSURFACE_API
        static const MString typeName;

        PXRUSDPREVIEWSURFACE_API
        static const MString drawDbClassification;
        PXRUSDPREVIEWSURFACE_API
        static const MString fullClassification;

        // Attributes
        PXRUSDPREVIEWSURFACE_API
        static MObject clearcoatAttr;
        PXRUSDPREVIEWSURFACE_API
        static MObject clearcoatRoughnessAttr;
        PXRUSDPREVIEWSURFACE_API
        static MObject diffuseColorAttr;
        PXRUSDPREVIEWSURFACE_API
        static MObject displacementAttr;
        PXRUSDPREVIEWSURFACE_API
        static MObject emissiveColorAttr;
        PXRUSDPREVIEWSURFACE_API
        static MObject iorAttr;
        PXRUSDPREVIEWSURFACE_API
        static MObject metallicAttr;
        PXRUSDPREVIEWSURFACE_API
        static MObject normalAttr;
        PXRUSDPREVIEWSURFACE_API
        static MObject occlusionAttr;
        PXRUSDPREVIEWSURFACE_API
        static MObject opacityAttr;
        PXRUSDPREVIEWSURFACE_API
        static MObject roughnessAttr;
        PXRUSDPREVIEWSURFACE_API
        static MObject specularColorAttr;
        PXRUSDPREVIEWSURFACE_API
        static MObject useSpecularWorkflowAttr;

        // Output Attributes.
        PXRUSDPREVIEWSURFACE_API
        static MObject outColorAttr;
        PXRUSDPREVIEWSURFACE_API
        static MObject outTransparencyAttr;

        PXRUSDPREVIEWSURFACE_API
        static void* creator();

        PXRUSDPREVIEWSURFACE_API
        static MStatus initialize();

        PXRUSDPREVIEWSURFACE_API
        void postConstructor() override;

        PXRUSDPREVIEWSURFACE_API
        MStatus compute(const MPlug& plug, MDataBlock& dataBlock) override;

    private:
        PxrMayaUsdPreviewSurface();
        ~PxrMayaUsdPreviewSurface() override;

        PxrMayaUsdPreviewSurface(const PxrMayaUsdPreviewSurface&) = delete;
        PxrMayaUsdPreviewSurface& operator=(
                const PxrMayaUsdPreviewSurface&) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
