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
#ifndef PXRUSDMAYA_MAYAPRIMWRITER_H
#define PXRUSDMAYA_MAYAPRIMWRITER_H

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "usdMaya/usdWriteJobCtx.h"

#include "pxr/usd/usd/stage.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdGeomImageable;
class UsdTimeCode;

// Writes an MFnMesh as a poly mesh OR a subd mesh
class MayaPrimWriter
{
  public:
    PXRUSDMAYA_API
    MayaPrimWriter(
            const MDagPath& iDag,
            const SdfPath& uPath,
            usdWriteJobCtx& jobCtx);
    virtual ~MayaPrimWriter() {};

    virtual void write(const UsdTimeCode &usdTime) = 0;
    virtual bool isShapeAnimated() const = 0;

    /// Does this PrimWriter directly create one or more gprims on the UsdStage?
    ///
    /// Base implementation returns \c false, so gprim/shape-derived classes
    /// should override.
    PXRUSDMAYA_API
    virtual bool exportsGprims() const;
    
    /// Does this PrimWriter add references on the UsdStage?
    ///
    /// Base implementation returns \c false.
    PXRUSDMAYA_API
    virtual bool exportsReferences() const;

    /// Does this PrimWriter request that the traversal code skip its child
    /// nodes because this PrimWriter will handle its child nodes by itself?
    ///
    /// Base implementation returns \c false.
    PXRUSDMAYA_API
    virtual bool shouldPruneChildren() const;

    /// Post export function that runs before saving the stage.
    ///
    /// Base implementation does nothing.
    PXRUSDMAYA_API
    virtual void postExport();

    /// Gets all of the prim paths that this prim writer has created.
    /// The base implementation just gets the single generated prim's path.
    /// Prim writers that generate more than one USD prim from a single Maya
    /// node should override this function to indicate all the prims they
    /// create.
    /// Implementations should add to outPaths instead of replacing. The return
    /// value should indicate whether any items were added to outPaths.
    PXRUSDMAYA_API
    virtual bool getAllAuthoredUsdPaths(SdfPathVector* outPaths) const;
    
public:
    const MDagPath&        getDagPath()    const { return mDagPath; }
    const SdfPath&         getUsdPath()    const { return mUsdPath; }
    const UsdStageRefPtr&  getUsdStage()   const { return mWriteJobCtx.getUsdStage(); }
    bool isValid()                         const { return mIsValid; }
    const JobExportArgs&   getArgs()       const { return mWriteJobCtx.getArgs(); }
    const UsdPrim&         getPrim()       const { return mUsdPrim; }

    bool getExportsVisibility() const { return mExportsVisibility; }
    void setExportsVisibility(bool exports);

protected:
    void setValid(bool isValid) { mIsValid = isValid;};
    void setUsdPath(const SdfPath &newPath) { mUsdPath = newPath;};
    PXRUSDMAYA_API
    bool writePrimAttrs(const MDagPath & iDag2, const UsdTimeCode &usdTime, UsdGeomImageable &primSchema);

    UsdPrim mUsdPrim;
    usdWriteJobCtx& mWriteJobCtx;

private:
    MDagPath mDagPath;
    SdfPath mUsdPath;

    bool mIsValid;
    bool mExportsVisibility;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_MAYAPRIMWRITER_H
