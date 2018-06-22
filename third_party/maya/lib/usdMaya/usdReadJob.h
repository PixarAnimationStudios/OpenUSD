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
#ifndef PXRUSDMAYA_USD_READ_JOB_H
#define PXRUSDMAYA_USD_READ_JOB_H

/// \file usdMaya/usdReadJob.h

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "usdMaya/jobArgs.h"
#include "usdMaya/primReaderContext.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/primRange.h"

#include <maya/MDagModifier.h>
#include <maya/MDagPath.h>

#include <map>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE



class usdReadJob
{
  public:

    PXRUSDMAYA_API
    usdReadJob(
            const std::string& iFileName,
            const std::string& iPrimPath,
            const std::map<std::string, std::string>& iVariants,
            const PxrUsdMayaJobImportArgs & iArgs);

    PXRUSDMAYA_API
    ~usdReadJob();

    PXRUSDMAYA_API
    bool doIt(std::vector<MDagPath>* addedDagPaths);
    PXRUSDMAYA_API
    bool redoIt();
    PXRUSDMAYA_API
    bool undoIt();

    // Getters/Setters
    void setMayaRootDagPath(const MDagPath &mayaRootDagPath) {
        mMayaRootDagPath = mayaRootDagPath;
    };

  private:
    // XXX: Activating the 'Expanded' representation of a USD reference
    // assembly node is very much like performing a regular usdReadJob but with
    // a few key differences (e.g. creating proxy shapes at collapse points).
    // These private helper methods cover the functionality of a regular
    // usdImport, and an 'Expanded' representation-style import, respectively.
    // It would be great if we could combine these into a single traversal at
    // some point.
    bool _DoImport(UsdPrimRange& range, const UsdPrim& usdRootPrim);
    bool _DoImportWithProxies(UsdPrimRange& range);

    // These are helper methods for the proxy import method.
    bool _ProcessProxyPrims(
            const std::vector<UsdPrim>& proxyPrims,
            const UsdPrim& pxrGeomRoot,
            const std::vector<std::string>& collapsePointPathStrings);
    bool _ProcessSubAssemblyPrims(const std::vector<UsdPrim>& subAssemblyPrims);
    bool _ProcessCameraPrims(const std::vector<UsdPrim>& cameraPrims);

    // Data
    PxrUsdMayaJobImportArgs mArgs;
    std::string mFileName;
    std::string mPrimPath;
    std::map<std::string,std::string> mVariants;
    MDagModifier mDagModifierUndo;
    bool mDagModifierSeeded;
    typedef PxrUsdMayaPrimReaderContext::ObjectRegistry PathNodeMap;
    PathNodeMap mNewNodeRegistry;
    MDagPath mMayaRootDagPath;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
