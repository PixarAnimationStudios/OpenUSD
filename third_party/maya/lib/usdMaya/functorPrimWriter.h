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
#ifndef PXRUSDMAYA_FUNCTOR_PRIM_WRITER_H
#define PXRUSDMAYA_FUNCTOR_PRIM_WRITER_H

/// \file usdMaya/functorPrimWriter.h

#include "pxr/pxr.h"
#include "usdMaya/transformWriter.h"

#include "usdMaya/primWriter.h"
#include "usdMaya/primWriterRegistry.h"
#include "usdMaya/writeJobContext.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/timeCode.h"

#include <maya/MFnDependencyNode.h>

#include <functional>


PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdMaya_FunctorPrimWriter
/// \brief This class is scaffolding to hold bare prim writer functions and
/// adapt them to the UsdMayaPrimWriter or UsdMayaTransformWriter interface
/// (depending on whether the writer plugin is handling a shape or a transform).
///
/// It is used by the PXRUSDMAYA_DEFINE_WRITER macro.
class UsdMaya_FunctorPrimWriter final : public UsdMayaTransformWriter
{
public:
    UsdMaya_FunctorPrimWriter(
            const MFnDependencyNode& depNodeFn,
            const SdfPath& usdPath,
            UsdMayaWriteJobContext& jobCtx,
            UsdMayaPrimWriterRegistry::WriterFn plugFn);

    ~UsdMaya_FunctorPrimWriter() override;

    void Write(const UsdTimeCode& usdTime) override;
    bool ExportsGprims() const override;
    bool ShouldPruneChildren() const override;
    const SdfPathVector& GetModelPaths() const override;

    static UsdMayaPrimWriterSharedPtr Create(
            const MFnDependencyNode& depNodeFn,
            const SdfPath& usdPath,
            UsdMayaWriteJobContext& jobCtx,
            UsdMayaPrimWriterRegistry::WriterFn plugFn);

    static UsdMayaPrimWriterRegistry::WriterFactoryFn
            CreateFactory(UsdMayaPrimWriterRegistry::WriterFn plugFn);

private:
    UsdMayaPrimWriterRegistry::WriterFn _plugFn;
    bool _exportsGprims;
    bool _pruneChildren;
    SdfPathVector _modelPaths;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
