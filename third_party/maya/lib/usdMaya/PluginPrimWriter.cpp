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
#include "pxr/pxr.h"
#include "usdMaya/PluginPrimWriter.h"
#include "pxr/usd/usdGeom/xformable.h"

PXR_NAMESPACE_OPEN_SCOPE


PxrUsdExport_PluginPrimWriter::PxrUsdExport_PluginPrimWriter(
        MDagPath& iDag,
        UsdStageRefPtr& stage,
        const JobExportArgs& iArgs,
        PxrUsdMayaPrimWriterRegistry::WriterFn plugFn) :
    MayaTransformWriter(iDag, stage, iArgs),
    _plugFn(plugFn),
    _exportsGprims(false),
    _exportsReferences(false),
    _pruneChildren(false)
{
}

PxrUsdExport_PluginPrimWriter::~PxrUsdExport_PluginPrimWriter()
{
}

UsdPrim
PxrUsdExport_PluginPrimWriter::write(
        const UsdTimeCode& usdTime)
{
    SdfPath authorPath = getUsdPath();
    UsdStageRefPtr stage = getUsdStage();

    PxrUsdMayaPrimWriterArgs args(getDagPath(),
        getArgs().exportRefsAsInstanceable);
    PxrUsdMayaPrimWriterContext ctx(usdTime, authorPath, stage);
    _plugFn(args, &ctx);
    _exportsGprims = ctx.GetExportsGprims();
    _exportsReferences = ctx.GetExportsReferences();
    _pruneChildren = ctx.GetPruneChildren();

    UsdPrim prim = stage->GetPrimAtPath(authorPath);
    if (!prim) {
        return prim;
    }

    // Write "parent" class attrs
    UsdGeomXformable primSchema(prim);
    if (primSchema) {
        writeTransformAttrs(usdTime, primSchema);
    }

    return prim;
}

bool
PxrUsdExport_PluginPrimWriter::exportsGprims() const
{
    return _exportsGprims;
}
    
bool
PxrUsdExport_PluginPrimWriter::exportsReferences() const
{
    return _exportsReferences;
}

bool
PxrUsdExport_PluginPrimWriter::shouldPruneChildren() const
{
    return _pruneChildren;
}


PXR_NAMESPACE_CLOSE_SCOPE

