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
#ifndef PXRUSDMAYA_INSTANCED_NODE_WRITER_H
#define PXRUSDMAYA_INSTANCED_NODE_WRITER_H

#include "usdMaya/MayaPrimWriter.h"
#include "usdMaya/usdWriteJobCtx.h"

#include "pxr/pxr.h"
#include "pxr/usd/usd/references.h"

PXR_NAMESPACE_OPEN_SCOPE

/// This is a "helper" prim writer used internally by usdWriteJobCtx to
/// author nodes that are directly instanced in Maya.
class PxrUsdMaya_InstancedNodeWriter : public MayaPrimWriter {
public:
    PXRUSDMAYA_API
    PxrUsdMaya_InstancedNodeWriter(
        const MDagPath& srcPath,
        const SdfPath& instancePath,
        usdWriteJobCtx& ctx);

    PXRUSDMAYA_API
    bool ExportsGprims() const override;

    PXRUSDMAYA_API
    bool ShouldPruneChildren() const override;

    PXRUSDMAYA_API
    const SdfPathVector& GetModelPaths() const override;

    PXRUSDMAYA_API
    const PxrUsdMayaUtil::MDagPathMap<SdfPath>::Type&
            GetDagToUsdPathMapping() const override;

    PXRUSDMAYA_API
    void Write(const UsdTimeCode& usdTime) override;

private:
    usdWriteJobCtx::_ExportAndRefPaths _masterPaths;

    // All of the data below is cached when we construct/obtain prim writers.
    bool _exportsGprims;
    std::vector<SdfPath> _modelPaths;
    PxrUsdMayaUtil::MDagPathMap<SdfPath>::Type _dagToUsdPaths;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
