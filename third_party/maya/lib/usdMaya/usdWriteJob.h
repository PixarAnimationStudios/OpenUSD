//
// Copyright 2016 Pixar
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
#ifndef PXRUSDMAYA_USDWRITEJOB_H
#define PXRUSDMAYA_USDWRITEJOB_H

/// \file usdWriteJob.h

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "usdMaya/Chaser.h"

#include "usdMaya/util.h"
#include "usdMaya/MayaPrimWriter.h"
#include "usdMaya/ModelKindWriter.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE


class usdWriteJob
{
  public:

    PXRUSDMAYA_API
    usdWriteJob(const JobExportArgs & iArgs);

    PXRUSDMAYA_API
    ~usdWriteJob();

    // returns true if the stage can be created successfully
    PXRUSDMAYA_API
    bool beginJob(bool append);
    PXRUSDMAYA_API
    void evalJob(double iFrame);
    PXRUSDMAYA_API
    void endJob();
    PXRUSDMAYA_API
    TfToken writeVariants(const UsdPrim &usdRootPrim);

  private:
    void perFrameCallback(double iFrame);
    void postCallback();
    bool createPrimWriter(MDagPath &curDag, MayaPrimWriterPtr* primWriterOut);
    
  private:
    JobExportArgs mArgs;

    // List of the primitive writers to iterate over
    std::vector<MayaPrimWriterPtr> mMayaPrimWriterList;

    // Stage used to write out USD file
    UsdStageRefPtr mStage;

    // Name of current layer since it should be restored after looping over them
    MString mCurrentRenderLayerName;
    
    // List of renderLayerObjects. Currently used for variants
    MObjectArray mRenderLayerObjs;
    
    // USD Maya Prim mapping used for variants
    PxrUsdMayaUtil::MDagPathMap<SdfPath>::Type mDagPathToUsdPathMap;

    PxrUsdMayaChaserRefPtrVector mChasers;

    PxrUsdMaya_ModelKindWriter mModelKindWriter;
};

typedef shared_ptr < usdWriteJob > usdWriteJobPtr;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_USDWRITEJOB_H
