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
#ifndef PXRUSDMAYA_PLUGINPRIMWRITER_H
#define PXRUSDMAYA_PLUGINPRIMWRITER_H

/// \file PluginPrimWriter.h

#include "usdMaya/MayaTransformWriter.h"
#include "usdMaya/JobArgs.h"

#include "usdMaya/primWriterRegistry.h"

#include "pxr/usd/usd/stage.h"
#include <boost/smart_ptr.hpp>

/// \class PxrUsdExport_PluginPrimWriter
/// \brief This class is scaffolding to hold the writer plugin and to adapt it
/// to the MayaTransformWriter class.  This allows our writer plugins to be
/// implemented without caring about the internal MayaTransformWriter interface.
///
/// This class is named "Plugin" because we are only supporting writer plugins
/// for user-defined maya dependency nodes.
class PxrUsdExport_PluginPrimWriter : public MayaTransformWriter
{
public:
    typedef boost::shared_ptr<PxrUsdExport_PluginPrimWriter> Ptr;
    PxrUsdExport_PluginPrimWriter(
            MDagPath& iDag,
            UsdStageRefPtr& stage,
            const JobExportArgs& iArgs,
            PxrUsdMayaPrimWriterRegistry::WriterFn plugFn);

    virtual ~PxrUsdExport_PluginPrimWriter();

    // Overrides for MayaTransformWriter
    virtual UsdPrim write(const UsdTimeCode &usdTime);
    
    virtual bool exportsGprims() const override;
    
    virtual bool exportsReferences() const override;

    virtual bool shouldPruneChildren() const override;    

private:
    PxrUsdMayaPrimWriterRegistry::WriterFn _plugFn;
    bool _exportsGprims;
    bool _exportsReferences;
    bool _pruneChildren;
};
#endif // PXRUSDMAYA_PLUGINPRIMWRITER_H
