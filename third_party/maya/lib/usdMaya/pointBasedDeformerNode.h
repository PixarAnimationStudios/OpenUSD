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
#ifndef PXRUSDMAYA_POINT_BASED_DEFORMER_NODE_H
#define PXRUSDMAYA_POINT_BASED_DEFORMER_NODE_H

/// \file usdMaya/pointBasedDeformerNode.h

#include "pxr/pxr.h"
#include "usdMaya/api.h"

#include "pxr/base/tf/staticTokens.h"

#include <maya/MDataBlock.h>
#include <maya/MItGeometry.h>
#include <maya/MMatrix.h>
#include <maya/MObject.h>
#include <maya/MPxDeformerNode.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>


PXR_NAMESPACE_OPEN_SCOPE


#define PXRUSDMAYA_POINT_BASED_DEFORMER_NODE_TOKENS \
    ((MayaTypeName, "pxrUsdPointBasedDeformerNode"))

TF_DECLARE_PUBLIC_TOKENS(PxrUsdMayaPointBasedDeformerNodeTokens,
                         PXRUSDMAYA_API,
                         PXRUSDMAYA_POINT_BASED_DEFORMER_NODE_TOKENS);


/// Maya deformer that uses the points of a UsdGeomPointBased prim to deform
/// the geometry.
///
/// This deformer node can be used to deform Maya geometry to match the points
/// of a UsdGeomPointBased prim. It takes as input a stage data object (which
/// can be received from a connection to a USD stage node), the prim path to a
/// UsdGeomPointBased prim in the stage data's stage, and a time sample. When
/// the deformer runs, it will read the points attribute of the prim at that
/// time sample and use the positions to modify the positions of the geometry
/// being deformed.
class UsdMayaPointBasedDeformerNode : public MPxDeformerNode
{
    public:
        PXRUSDMAYA_API
        static const MTypeId typeId;
        PXRUSDMAYA_API
        static const MString typeName;

        // Attributes
        PXRUSDMAYA_API
        static MObject inUsdStageAttr;
        PXRUSDMAYA_API
        static MObject primPathAttr;
        PXRUSDMAYA_API
        static MObject timeAttr;

        PXRUSDMAYA_API
        static void* creator();

        PXRUSDMAYA_API
        static MStatus initialize();

        // MPxGeometryFilter overrides
        PXRUSDMAYA_API
        MStatus deform(
                MDataBlock& block,
                MItGeometry& iter,
                const MMatrix& mat,
                unsigned int multiIndex) override;

    private:
        UsdMayaPointBasedDeformerNode();
        ~UsdMayaPointBasedDeformerNode() override;

        UsdMayaPointBasedDeformerNode(const UsdMayaPointBasedDeformerNode&);
        UsdMayaPointBasedDeformerNode& operator=(
                const UsdMayaPointBasedDeformerNode&);
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
