//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_PROVIDER_RESOLUTION_H
#define PXR_EXEC_PROVIDER_RESOLUTION_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/exec/types.h"

#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Data used to find computation providers during exec compilation.
///
/// The provider resolution process starts from the **origin object**, the
/// scene object that owns the consuming computation, and searches to find
/// **computation providers**, the scene objects that own the computations
/// that are evaluated to yield input values.
/// 
struct ExecProviderResolution {

    /// A path that describes the first part of provider resolution.
    ///
    /// This path is relative to the origin object.
    /// 
    SdfPath localTraversal;

    /// An enum that indicates the part of provider resolution that is
    /// implemented by exec compilation logic.
    ///
    /// This part of the traversal can search through the scene, apply
    /// predicates, and can branch out, potentially finding multiple providers.
    /// 
    enum class DynamicTraversal {
        /// The localTraversal path directly indicates the computation provider.
        Local,             

        /// Find the providers by traversing relationship targets, applying any
        /// relationship forwarding, to the targeted objects.
        RelationshipTargetedObjects,

        /// Find the provider by traversing upward in namespace
        NamespaceAncestor,
    };

    /// An enum value that indicates the type of dynamic traversal used during
    /// provider resolution.
    /// 
    DynamicTraversal dynamicTraversal;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
