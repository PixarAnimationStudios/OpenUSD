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
#ifndef PXRUSDMAYA_HD_IMAGING_SHAPE_H
#define PXRUSDMAYA_HD_IMAGING_SHAPE_H

/// \file usdMaya/hdImagingShape.h

#include "pxr/pxr.h"
#include "usdMaya/api.h"

#include "pxr/base/tf/staticTokens.h"

#include <maya/MBoundingBox.h>
#include <maya/MDagPath.h>
#include <maya/MObject.h>
#include <maya/MPxSurfaceShape.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>


PXR_NAMESPACE_OPEN_SCOPE


#define PXRUSDMAYA_HD_IMAGING_SHAPE_TOKENS \
    ((MayaTypeName, "pxrHdImagingShape"))

TF_DECLARE_PUBLIC_TOKENS(PxrMayaHdImagingShapeTokens,
                         PXRUSDMAYA_API,
                         PXRUSDMAYA_HD_IMAGING_SHAPE_TOKENS);


/// Simple Maya shape providing batched drawing of other shapes imaged by Hydra.
///
/// This shape does nothing other than to act as a single invocation point for
/// Hydra task execution for all other shapes in the scene that are imaged by
/// Hydra. Those other shapes will respond to Maya's requests for draw
/// preparation, but the actual drawing of those shapes by Hydra will only be
/// invoked when this shape is drawn.
class PxrMayaHdImagingShape : public MPxSurfaceShape
{
    public:

        PXRUSDMAYA_API
        static const MTypeId typeId;
        PXRUSDMAYA_API
        static const MString typeName;

        PXRUSDMAYA_API
        static void* creator();

        PXRUSDMAYA_API
        static MStatus initialize();

        PXRUSDMAYA_API
        static PxrMayaHdImagingShape* GetShapeAtDagPath(
                const MDagPath& dagPath);

        /// Gets the "singleton" instance of the shape if it exists, or creates
        /// it if it doesn't.
        ///
        /// There is typically only one instance of this node in a Maya scene
        /// that takes care of all Hydra imaging for the scene. This method can
        /// be used to ensure that that instance exists, and to get the MObject
        /// for it.
        /// Note that since this node is a shape, it is required to have a
        /// transform node as a parent. This method will create that node as
        /// well and set it up such that it will *not* save into the Maya scene
        /// file. The nodes are also locked to prevent accidental deletion,
        /// re-naming, or re-parenting.
        PXRUSDMAYA_API
        static MObject GetOrCreateInstance();


        // MPxSurfaceShape Overrides

        PXRUSDMAYA_API
        bool isBounded() const override;

        PXRUSDMAYA_API
        MBoundingBox boundingBox() const override;


        // MPxNode Overrides

        PXRUSDMAYA_API
        void postConstructor() override;

    private:

        PxrMayaHdImagingShape();
        ~PxrMayaHdImagingShape() override;

        PxrMayaHdImagingShape(const PxrMayaHdImagingShape&) = delete;
        PxrMayaHdImagingShape& operator=(const PxrMayaHdImagingShape&) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
