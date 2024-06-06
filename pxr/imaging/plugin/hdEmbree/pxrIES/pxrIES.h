//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_PLUGIN_HD_EMBREE_PXRIES_PXRIES_H
#define PXR_IMAGING_PLUGIN_HD_EMBREE_PXRIES_PXRIES_H

#include "pxr/pxr.h"
#include "pxr/imaging/plugin/hdEmbree/pxrIES/ies.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \class PxrIESFile
///
/// Extends / overrides some functionality of standard IESFile.
///
class PxrIESFile : public pxr_ccl::IESFile {
private:
    using Base = pxr_ccl::IESFile;

public:

    bool load(std::string const& ies);  // non-virtual "override"
    void clear();  // non-virtual "override"

    /// \brief The light's power, as calculated when parsing
    inline float power() const
    {
        return _power;
    }

protected:
    // Extra processing we do on-top of the "standard" process() from IESFile
    void pxr_extra_process();

private:

    float _power = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_HD_EMBREE_PXRIES_PXRIES_H
