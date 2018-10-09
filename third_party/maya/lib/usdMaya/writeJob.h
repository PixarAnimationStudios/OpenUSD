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
#ifndef PXRUSDMAYA_WRITE_JOB_H
#define PXRUSDMAYA_WRITE_JOB_H

/// \file usdMaya/writeJob.h

#include "usdMaya/chaser.h"
#include "usdMaya/util.h"
#include "usdMaya/writeJobContext.h"

#include "pxr/pxr.h"

#include "pxr/base/tf/hashmap.h"

#include <maya/MObjectHandle.h>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class UsdMaya_ModelKindProcessor;

class UsdMaya_WriteJob
{
public:
    UsdMaya_WriteJob(const UsdMayaJobExportArgs & iArgs);

    ~UsdMaya_WriteJob();

    /// Writes the Maya stage to the given USD file name, If \p append is
    /// \c true, adds to an existing stage. Otherwise, replaces any existing
    /// file.
    /// This will write the entire frame range specified by the export args.
    /// Returns \c true if successful, or \c false if an error was encountered.
    bool Write(const std::string& fileName, bool append);

private:
    /// Begins constructing the USD stage, writing out the values at the default
    /// time. Returns \c true if the stage can be created successfully.
    bool _BeginWriting(const std::string& fileName, bool append);
  
    /// Writes the stage values at the given frame.
    /// Warning: this function must be called with non-decreasing frame numbers.
    /// If you call WriteFrame() with a frame number lower than a previous
    /// WriteFrame() call, internal code may generate errors.
    bool _WriteFrame(double iFrame);

    /// Runs any post-export processes, closes the USD stage, and writes it out
    /// to disk.
    bool _FinishWriting();

    /// Writes the root prim variants based on the Maya render layers.
    TfToken _WriteVariants(const UsdPrim &usdRootPrim);

    /// Creates a usdz package from the write job's current USD stage.
    void _CreatePackage() const;

    void _PerFrameCallback(double iFrame);
    void _PostCallback();

    // Name of the created/appended USD file
    std::string _fileName;

    // Name of destination packaged archive.
    std::string _packageName;

    // Name of current layer since it should be restored after looping over them
    MString mCurrentRenderLayerName;
    
    // List of renderLayerObjects. Currently used for variants
    MObjectArray mRenderLayerObjs;

    UsdMayaUtil::MDagPathMap<SdfPath> mDagPathToUsdPathMap;

    // Currently only used if stripNamespaces is on, to ensure we don't have clashes
    TfHashMap<SdfPath, MDagPath, SdfPath::Hash> mUsdPathToDagPathMap;

    UsdMayaChaserRefPtrVector mChasers;

    UsdMayaWriteJobContext mJobCtx;

    std::unique_ptr<UsdMaya_ModelKindProcessor> _modelKindProcessor;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
