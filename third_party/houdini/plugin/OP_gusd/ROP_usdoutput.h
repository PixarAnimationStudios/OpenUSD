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
#ifndef __GUSD_ROP_USDOUTPUT_H__
#define __GUSD_ROP_USDOUTPUT_H__

#include <pxr/pxr.h>
#include "pxr/base/tf/refPtr.h"

#include "pxr/usd/usd/stage.h"

#include <GA/GA_OffsetList.h>
#include <GA/GA_Types.h>
#include <GU/GU_Detail.h>
#include <ROP/ROP_Node.h>
#include <UT/UT_Map.h>

#include "gusd/GT_Utils.h"

PXR_NAMESPACE_OPEN_SCOPE

class GusdROP_usdoutput : public ROP_Node
{
    typedef UT_Map<SdfPath, GT_PrimitiveHandle>   GprimMap;

    typedef std::pair<std::string, std::string> UsdRefShader;
    typedef UT_Map<UsdRefShader, std::vector<SdfPath> > UsdRefShaderMap;
    typedef UT_Map<std::string, std::vector<SdfPath> >  HouMaterialMap;

    enum Granularity { ONE_FILE, PER_FRAME };

public:

    static void Register(OP_OperatorTable* table);

    GusdROP_usdoutput(OP_Network* network,
               const char* name,
               OP_Operator* op);

    virtual ~GusdROP_usdoutput();

    virtual bool updateParmsFlags() override;
    
    virtual int startRender(int frameCount,
                            fpreal tstart,
                            fpreal tend) override;

    virtual ROP_RENDER_CODE renderFrame(fpreal time,
                                        UT_Interrupt* interrupt) override;

    virtual ROP_RENDER_CODE endRender() override;

private:

    ROP_RENDER_CODE openStage(fpreal tstart, int startTimeCode, int endTimeCode);
    ROP_RENDER_CODE closeStage(fpreal tend);

    ROP_RENDER_CODE bindAndWriteShaders(UsdRefShaderMap& usdRefShaderMap,
                                        HouMaterialMap& houMaterialMap);
    void resetState();

    ROP_RENDER_CODE abort(const std::string& errorMessage);

private:

    double          m_startFrame;
    double          m_endFrame;
    std::string     m_pathPrefix;
    bool            m_hasPartitionAttr;
    std::string     m_partitionAttrName;
    OP_Context      m_houdiniContext;

    SOP_Node*       m_renderNode;

    UsdStageRefPtr     m_usdStage;
    int                m_fdTmpFile;
    GusdGT_AttrFilter  m_primvarFilter;
    GprimMap           m_gprimMap;
    std::string        m_defaultPrimPath;
    UsdPrim            m_modelPrim;
    std::string        m_assetName;

    Granularity m_granularity;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif /* __GUSD_ROP_USDOUTPUT_H__ */
