//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_GP_GENERATIVE_PROCEDURAL_PLUGIN_H
#define PXR_IMAGING_HD_GP_GENERATIVE_PROCEDURAL_PLUGIN_H

#include "pxr/imaging/hdGp/generativeProcedural.h"

#include "pxr/pxr.h"
#include "pxr/imaging/hdGp/api.h"
#include "pxr/imaging/hf/pluginBase.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdGpGenerativeProceduralPlugin
/// 
/// HdGpGenerativeProceduralPlugin represents an HdGpGenerativeProcedural for
/// plug-in discovery via HdGpGenerativeProceduralPluginRegistry.
///
class HdGpGenerativeProceduralPlugin : public HfPluginBase
{
public:

    /// Subclasses implement this to instantiate an HdGpGenerativeProcedural
    /// at a given prim path.
    HDGP_API
    virtual HdGpGenerativeProcedural *Construct(
        const SdfPath &proceduralPrimPath);

protected:

    HDGP_API
    HdGpGenerativeProceduralPlugin();

    HDGP_API
    ~HdGpGenerativeProceduralPlugin() override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
