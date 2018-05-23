//
// Copyright 2017 Pixar
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
#ifndef __GUSD_ROP_USDLUXOUTPUT_H__
#define __GUSD_ROP_USDLUXOUTPUT_H__

#include <pxr/pxr.h>
#include "pxr/base/tf/refPtr.h"

#include "pxr/usd/usd/stage.h"

#include <GA/GA_OffsetList.h>
#include <GA/GA_Types.h>
#include <GU/GU_Detail.h>
#include <ROP/ROP_Node.h>
#include <UT/UT_Map.h>

#include "gusd/GT_Utils.h"
#include "gusd/lightWrapper.h"

PXR_NAMESPACE_OPEN_SCOPE

class GusdROP_usdluxoutput : public ROP_Node
{
    using GprimMap = UT_Map<SdfPath, GT_PrimitiveHandle>;

    using UsdRefShader = std::pair<std::string, std::string>;
    using UsdRefShaderMap = UT_Map<UsdRefShader, std::vector<SdfPath> >;
    using HouMaterialMap = UT_Map<std::string, std::vector<SdfPath> >;

    enum Granularity { ONE_FILE, PER_FRAME };

public:

    static void Register(OP_OperatorTable* table);

    GusdROP_usdluxoutput(OP_Network* network,
                      const char* name,
                      OP_Operator* op);

    virtual ~GusdROP_usdluxoutput() = default;

    virtual bool updateParmsFlags();

    /// Called at the beginning of rendering to perform any intialization
    /// necessary.
    /// @param  nframes     Number of frames being rendered.
    /// @param  s           Start time, in seconds.
    /// @param  e           End time, in seconds.
    /// @return             True of success, false on failure (aborts the render).
    virtual int                  startRender(int nframes, fpreal s, fpreal e) override;
    /// Called once for every frame that is rendered.
    /// @param  time        The time to render at.
    /// @param  boss        Interrupt handler.
    /// @return             Return a status code indicating whether to abort the
    ///                     render, continue, or retry the current frame.
    virtual ROP_RENDER_CODE      renderFrame(fpreal time, UT_Interrupt *boss) override;
    /// Called after the rendering is done to perform any post-rendering steps
    /// required.
    /// @return             Return a status code indicating whether to abort the
    ///                     render, continue, or retry.
    virtual ROP_RENDER_CODE      endRender() override;

private:

    ROP_RENDER_CODE openStage(fpreal tstart, int startTimeCode, int endTimeCode);
    ROP_RENDER_CODE closeStage(fpreal tend);
    bool filterNode(OP_Node *node);

    exint collectLightNodes(OP_NodeList& lightNodeList, OP_NodeList& netNodeList, OP_Node* root);

    void resetState();

    ROP_RENDER_CODE abort(const std::string& errorMessage);

private:

    double          m_startFrame;
    double          m_endFrame;

    OP_NodeList     m_renderNodes;

    UsdStageRefPtr     m_usdStage;
    int                m_fdTmpFile;
    std::string        m_defaultPrimPath;

    Granularity m_granularity;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif //__GUSD_ROP_USDLUXOUTPUT_H__
