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
#include "usdMaya/functorPrimWriter.h"
#include "pxr/usd/usdGeom/xformable.h"

PXR_NAMESPACE_OPEN_SCOPE


PxrUsdMaya_FunctorPrimWriter::PxrUsdMaya_FunctorPrimWriter(
        const MDagPath& iDag,
        const SdfPath& uPath,
        usdWriteJobCtx& jobCtx,
        PxrUsdMayaPrimWriterRegistry::WriterFn plugFn) :
    MayaTransformWriter(iDag, uPath, jobCtx),
    _plugFn(plugFn),
    _exportsGprims(false),
    _pruneChildren(false)
{
}

PxrUsdMaya_FunctorPrimWriter::~PxrUsdMaya_FunctorPrimWriter()
{
}

void
PxrUsdMaya_FunctorPrimWriter::Write(const UsdTimeCode& usdTime)
{
    MayaTransformWriter::Write(usdTime);

    SdfPath authorPath = GetUsdPath();
    UsdStageRefPtr stage = GetUsdStage();

    PxrUsdMayaPrimWriterArgs args(GetDagPath(),
        _GetExportArgs().exportRefsAsInstanceable);
    PxrUsdMayaPrimWriterContext ctx(usdTime, authorPath, stage);
    _plugFn(args, &ctx);
    _exportsGprims = ctx.GetExportsGprims();
    _pruneChildren = ctx.GetPruneChildren();
    _modelPaths = ctx.GetModelPaths();
}

bool
PxrUsdMaya_FunctorPrimWriter::ExportsGprims() const
{
    return _exportsGprims;
}

bool
PxrUsdMaya_FunctorPrimWriter::ShouldPruneChildren() const
{
    return _pruneChildren;
}

const SdfPathVector&
PxrUsdMaya_FunctorPrimWriter::GetModelPaths() const
{
    return _modelPaths;
}

/* static */
MayaPrimWriterSharedPtr
PxrUsdMaya_FunctorPrimWriter::Create(
    const MDagPath& dag,
    const SdfPath& path,
    usdWriteJobCtx& jobCtx,
    PxrUsdMayaPrimWriterRegistry::WriterFn plugFn)
{
    return MayaPrimWriterSharedPtr(
            new PxrUsdMaya_FunctorPrimWriter(dag, path, jobCtx, plugFn));
}

/* static */
PxrUsdMayaPrimWriterRegistry::WriterFactoryFn
PxrUsdMaya_FunctorPrimWriter::CreateFactory(
    PxrUsdMayaPrimWriterRegistry::WriterFn fn)
{
    return std::bind(
            Create,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3,
            fn);
}

PXR_NAMESPACE_CLOSE_SCOPE

