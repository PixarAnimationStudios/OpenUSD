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
#include "usdMaya/FunctorPrimWriter.h"
#include "pxr/usd/usdGeom/xformable.h"

PXR_NAMESPACE_OPEN_SCOPE


FunctorPrimWriter::FunctorPrimWriter(
        const MDagPath& iDag,
        const SdfPath& uPath,
        bool instanceSource,
        usdWriteJobCtx& jobCtx,
        WriterFn plugFn) :
    MayaTransformWriter(iDag, uPath, instanceSource, jobCtx),
    _plugFn(plugFn),
    _exportsGprims(false),
    _exportsReferences(false),
    _pruneChildren(false)
{
}

FunctorPrimWriter::~FunctorPrimWriter()
{
}

void
FunctorPrimWriter::write(
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

    mUsdPrim = stage->GetPrimAtPath(authorPath);
    if (!mUsdPrim) {
        return;
    }

    // Write "parent" class attrs
    UsdGeomXformable primSchema(mUsdPrim);
    if (primSchema) {
        writeTransformAttrs(usdTime, primSchema);
    }
}

bool
FunctorPrimWriter::exportsGprims() const
{
    return _exportsGprims;
}
    
bool
FunctorPrimWriter::exportsReferences() const
{
    return _exportsReferences;
}

bool
FunctorPrimWriter::shouldPruneChildren() const
{
    return _pruneChildren;
}

/* static */
MayaPrimWriterPtr
FunctorPrimWriter::Create(
    const MDagPath& dag,
    const SdfPath& path,
    bool instanceSource,
    usdWriteJobCtx& jobCtx,
    WriterFn plugFn)
{
    return MayaPrimWriterPtr(new FunctorPrimWriter(dag, path, instanceSource, jobCtx, plugFn));
}

/* static */
std::function< MayaPrimWriterPtr(const MDagPath&,
                                 const SdfPath&, bool,
                                 usdWriteJobCtx&) >
FunctorPrimWriter::CreateFactory(WriterFn fn)
{
    return std::bind(
            Create,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3,
            std::placeholders::_4,
            fn);
}

PXR_NAMESPACE_CLOSE_SCOPE

