//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HF_PLUGIN_BASE_H
#define PXR_IMAGING_HF_PLUGIN_BASE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hf/api.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HfPluginBase
///
/// Base class for all hydra plugin classes. This class provides no
/// functionality other than to serve as a polymorphic type for the
/// plugin registry.
///
class HfPluginBase
{
public:
    HF_API
    virtual ~HfPluginBase();  // = default: See workaround in cpp file

protected:
    // Pure virtual class, must be derived
    HF_API
    HfPluginBase() = default;

private:
    ///
    /// This class is not intended to be copied.
    ///
    HfPluginBase(const HfPluginBase &)            = delete;
    HfPluginBase &operator=(const HfPluginBase &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HF_PLUGIN_BASE_H
