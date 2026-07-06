//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDLOD_HEURISTIC_QUERY_H
#define USDLOD_HEURISTIC_QUERY_H

/// \file usdLod/heuristicQuery.h

#include "pxr/pxr.h"
#include "pxr/usd/usdLod/api.h"

#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdLodHeuristicQuery
///
/// This class is the base class of all heuristic query objects. These objects
/// contain the state of a heuristic and implement the calculation of LOD
/// children without any dependencies on USD scene data or any USD data
/// types. It is intended to be usable inside a renderer to make fast, run-time
/// LOD decisions without requiring access back to the original USD layers or
/// prims.
///
/// This base class contains a domain token, but all actual calculations are
/// required to be implemented in derived classes.
class UsdLodHeuristicQuery
{
public:
    /// The LOD domain of this heuristic. All heuristics have a domain over
    /// which they operate. This serves to further identify the purpose of this
    /// heuristic to the renderer. If the renderer is not interested in this
    /// domain, it should ignore the heuristic. For example, an audio renderer
    /// may ignore heuristics in the imaging domain.
    TfToken lodDomain;

public:
    UsdLodHeuristicQuery() = default;

    /// Construct with an explicit domain.
    UsdLodHeuristicQuery(const TfToken& lodDomainIn)
    : lodDomain(lodDomainIn)
    { }

    /// Destructor. The destructor is virtual to support RTTI and allow the
    /// object to be safely held as a pointer to UsdLodHeuristicQuery.
    USDLOD_API
    virtual ~UsdLodHeuristicQuery();


};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
