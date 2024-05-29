//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_CLEANUP_TRACKER_H
#define PXR_USD_SDF_CLEANUP_TRACKER_H

/// \file sdf/cleanupTracker.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/spec.h"

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfSpec);

#include <vector>

/// \class Sdf_CleanupTracker
///
/// A singleton that tracks specs edited within an Sdf_CleanupEnabler scope.
///
/// When the last Sdf_CleanupEnabler goes out of scope, the specs are removed
/// from the layer if they are inert.
///
class Sdf_CleanupTracker : public TfWeakBase
{
public:
    
    /// Retrieves singleton instance.
    static Sdf_CleanupTracker &GetInstance();

    /// Adds the spec to the vector of tracked specs if there is at least one
    /// Sdf_CleanupEnabler on the stack.
    void AddSpecIfTracking(SdfSpecHandle const &spec);

    /// Return the authoring monitor identified by the index
    void CleanupSpecs();

private:
    
    Sdf_CleanupTracker();
    ~Sdf_CleanupTracker();

    std::vector<SdfSpecHandle> _specs;
    
    friend class TfSingleton<Sdf_CleanupTracker>;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_CLEANUP_TRACKER_H
