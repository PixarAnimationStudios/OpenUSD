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
#ifndef PXRUSDMAYA_FUNCTORPRIMWRITER_H
#define PXRUSDMAYA_FUNCTORPRIMWRITER_H

/// \file FunctorPrimWriter.h

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "usdMaya/MayaTransformWriter.h"

#include "usdMaya/primWriterArgs.h"
#include "usdMaya/primWriterContext.h"

#include "pxr/usd/usd/stage.h"

#include <functional>

PXR_NAMESPACE_OPEN_SCOPE


/// \class FunctorPrimWriter
/// \brief This class is scaffolding to hold the writer plugin and to adapt it
/// to the MayaTransformWriter class.  This allows our writer plugins to be
/// implemented without caring about the internal MayaTransformWriter interface.
///
/// This class can be used as a base for plugins that write user-defined Maya
/// shape nodes to a USD prim. For other types of nodes, you may want to
/// consider creating a custom prim writer.
class FunctorPrimWriter : public MayaTransformWriter
{
public:
    typedef std::function< bool (
            const PxrUsdMayaPrimWriterArgs&,
            PxrUsdMayaPrimWriterContext*) > WriterFn;

    PXRUSDMAYA_API
    FunctorPrimWriter(
            const MDagPath& iDag,
            const SdfPath& uPath,
            bool instanceSource,
            usdWriteJobCtx& jobCtx,
            WriterFn plugFn);

    PXRUSDMAYA_API
    virtual ~FunctorPrimWriter();

    // Overrides for MayaTransformWriter
    PXRUSDMAYA_API
    virtual void write(const UsdTimeCode &usdTime) override;
    
    PXRUSDMAYA_API
    virtual bool exportsGprims() const override;
    
    PXRUSDMAYA_API
    virtual bool exportsReferences() const override;

    PXRUSDMAYA_API
    virtual bool shouldPruneChildren() const override;    

    PXRUSDMAYA_API
    static MayaPrimWriterPtr Create(
            const MDagPath& dag,
            const SdfPath& path,
            bool instanceSource,
            usdWriteJobCtx& jobCtx,
            WriterFn plugFn);

    PXRUSDMAYA_API
    static std::function< MayaPrimWriterPtr(
            const MDagPath&,
            const SdfPath&,
            bool,
            usdWriteJobCtx&) >
            CreateFactory(WriterFn plugFn);

private:
    WriterFn _plugFn;
    bool _exportsGprims;
    bool _exportsReferences;
    bool _pruneChildren;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_FUNCTORPRIMWRITER_H
