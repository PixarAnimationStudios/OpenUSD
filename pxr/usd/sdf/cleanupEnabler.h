//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_CLEANUP_ENABLER_H
#define PXR_USD_SDF_CLEANUP_ENABLER_H

/// \file sdf/cleanupEnabler.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/base/tf/stacked.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class SdfCleanupEnabler
///
/// An RAII class which, when an instance is alive, enables scheduling of
/// automatic cleanup of SdfLayers.
/// 
/// Any affected specs which no longer contribute to the scene will be removed 
/// when the last SdfCleanupEnabler instance goes out of scope. Note that for 
/// this purpose, SdfPropertySpecs are removed if they have only required fields 
/// (see SdfPropertySpecs::HasOnlyRequiredFields), but only if the property spec 
/// itself was affected by an edit that left it with only required fields. This 
/// will have the effect of uninstantiating on-demand attributes. For example, 
/// if its parent prim was affected by an edit that left it otherwise inert, it 
/// will not be removed if it contains an SdfPropertySpec with only required 
/// fields, but if the property spec itself is edited leaving it with only 
/// required fields, it will be removed, potentially uninstantiating it if it's 
/// an on-demand property.
/// 
/// SdfCleanupEnablers are accessible in both C++ and Python.
/// 
/// /// SdfCleanupEnabler can be used in the following manner:
/// \code
/// {
///     SdfCleanupEnabler enabler;
///     
///     // Perform any action that might otherwise leave inert specs around, 
///     // such as removing info from properties or prims, or removing name 
///     // children. i.e:
///     primSpec->ClearInfo(SdfFieldKeys->Default);
/// 
///     // When enabler goes out of scope on the next line, primSpec will 
///     // be removed if it has been left as an empty over.
/// }
/// \endcode
///
TF_DEFINE_STACKED(SdfCleanupEnabler, false, SDF_API)
{
public:

    SDF_API SdfCleanupEnabler();

    SDF_API ~SdfCleanupEnabler();

    /// Returns whether cleanup is currently being scheduled.
    SDF_API static bool IsCleanupEnabled();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // #ifndef PXR_USD_SDF_CLEANUP_ENABLER_H
