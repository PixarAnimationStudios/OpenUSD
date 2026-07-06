//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_EXEC_IMAGING_ADAPTER_REGISTRY_H
#define PXR_USD_IMAGING_USD_EXEC_IMAGING_ADAPTER_REGISTRY_H

/// \file

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdExecImagingPrimAdapterInterface;
class UsdPrim;

/// A singleton class that organizes all registered prim adapters.
///
/// TODO: This class remains largely un-implemented with the exception of a few
/// hard-coded adapter types. This will change in the future when we add the
/// ability to register prim adapters from separate plugins.
///
class UsdExecImaging_AdapterRegistry
{
public:
    /// Returns the adapter registered to handle the typed schema for \p prim.
    ///
    /// If there is no registered adapter for the type of \p prim, this returns
    /// a null pointer.
    ///
    static UsdExecImagingPrimAdapterInterface *GetPrimAdapter(
        const UsdPrim &prim);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif