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
#ifndef PXRUSDMAYA_USDWRITEJOBCTX_H
#define PXRUSDMAYA_USDWRITEJOBCTX_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/path.h"
#include "usdMaya/api.h"
#include "usdMaya/skelBindingsWriter.h"
#include "usdMaya/JobArgs.h"

#include <maya/MDagPath.h>
#include <maya/MObjectHandle.h>

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class MayaPrimWriter;
typedef std::shared_ptr<MayaPrimWriter> MayaPrimWriterPtr;

class usdWriteJob;

/// \class usdWriteJobCtx
/// \brief Provides basic functionality and access to shared data for MayaPrimWriters.
///
/// The main purpose of this class is to handle source prim creation for instancing,
/// and to avoid storing the JobExportArgs and UsdStage on each prim writer.
///
class usdWriteJobCtx {
protected:
    friend class usdWriteJob;

    PXRUSDMAYA_API
    usdWriteJobCtx(const JobExportArgs& args);
public:
    const JobExportArgs& getArgs() const { return mArgs; };
    const UsdStageRefPtr& getUsdStage() const { return mStage; };
    // Querying the master path for instancing. This also creates the shape if it doesn't exists.
    PXRUSDMAYA_API
    SdfPath getMasterPath(const MDagPath& dg);
    /// Creates a prim writer that writes the given prim (and descendants) to
    /// the USD namespace hierarchy anchored at the given path.
    /// If the given path is empty, then the USD path will be inferred from the
    /// Maya DAG path.
    PXRUSDMAYA_API
    MayaPrimWriterPtr createPrimWriter(
            const MDagPath& curDag,
            const SdfPath& usdPath = SdfPath());
    PXRUSDMAYA_API
    bool needToTraverse(const MDagPath& curDag);
    PXRUSDMAYA_API
    PxrUsdMaya_SkelBindingsWriter& getSkelBindingsWriter()
    {
        return mSkelBindingsWriter;
    }
protected:
    PXRUSDMAYA_API
    bool openFile(const std::string& filename, bool append);
    PXRUSDMAYA_API
    void processInstances();

    JobExportArgs mArgs;
    // List of the primitive writers to iterate over
    std::vector<MayaPrimWriterPtr> mMayaPrimWriterList;
    // Stage used to write out USD file
    UsdStageRefPtr mStage;
private:
    PXRUSDMAYA_API
    SdfPath getUsdPathFromDagPath(const MDagPath& dagPath, bool instanceSource);

    struct MObjectHandleComp {
        bool operator()(const MObjectHandle& rhs, const MObjectHandle& lhs) const {
            return rhs.hashCode() < lhs.hashCode();
        }
    };
    std::map<MObjectHandle, SdfPath, MObjectHandleComp> mMasterToUsdPath;
    PXRUSDMAYA_API
    MayaPrimWriterPtr _createPrimWriter(
            const MDagPath& curDag,
            const SdfPath& usdPath,
            bool instanceSource);
    UsdPrim mInstancesPrim;
    SdfPath mParentScopePath;
    bool mNoInstances;
    PxrUsdMaya_SkelBindingsWriter mSkelBindingsWriter;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
