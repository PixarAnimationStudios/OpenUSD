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
#ifndef PXRUSDMAYA_STAGE_NODE_H
#define PXRUSDMAYA_STAGE_NODE_H

/// \file usdMaya/stageNode.h

#include "usdMaya/api.h"

#include "pxr/pxr.h"

#include "pxr/base/tf/staticTokens.h"

#include <maya/MDataBlock.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPxNode.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>


PXR_NAMESPACE_OPEN_SCOPE


#define PXRUSDMAYA_STAGE_NODE_TOKENS \
    ((MayaTypeName, "pxrUsdStageNode"))

TF_DECLARE_PUBLIC_TOKENS(UsdMayaStageNodeTokens,
                         PXRUSDMAYA_API,
                         PXRUSDMAYA_STAGE_NODE_TOKENS);


/// Maya dependency node that reads and outputs a USD stage.
///
/// This is a simple MPxNode that reads in the USD stage identified by its
/// file path attribute and makes that stage available as a stage data object
/// on its output attribute. Downstream Maya nodes can connect this output to
/// their own stage data input attributes to gain access to the stage. This
/// allows sharing of a single USD stage by multiple downstream consumer nodes,
/// and it keeps all of the specifics of reading/caching USD stages and layers
/// in this stage node so that consumers can simply focus on working with the
/// stage and its contents.
class UsdMayaStageNode : public MPxNode
{
    public:
        PXRUSDMAYA_API
        static const MTypeId typeId;
        PXRUSDMAYA_API
        static const MString typeName;

        // Attributes
        PXRUSDMAYA_API
        static MObject filePathAttr;
        PXRUSDMAYA_API
        static MObject outUsdStageAttr;

        PXRUSDMAYA_API
        static void* creator();

        PXRUSDMAYA_API
        static MStatus initialize();

        // MPxNode overrides
        PXRUSDMAYA_API
        MStatus compute(const MPlug& plug, MDataBlock& dataBlock) override;

    private:
        UsdMayaStageNode();
        ~UsdMayaStageNode() override;

        UsdMayaStageNode(const UsdMayaStageNode&);
        UsdMayaStageNode& operator=(const UsdMayaStageNode&);
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
