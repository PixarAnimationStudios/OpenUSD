//
// Copyright 2025 Apple
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

/// \file usdPmc/usdPmcEncodeSession.hpp

#ifndef USD_PMC_ENCODE_SESSION_H
#define USD_PMC_ENCODE_SESSION_H

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/mesh.h"
#include <pmc/pmEncoder.hpp>

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \struct PmcEncodeSession
///
/// Single mesh PMC encoder session.
///
/// This structure manages the encoding of a single UsdGeomMesh into PMC
/// compressed format. It handles the conversion of geometry primitives,
/// attributes, and metadata from USD to PMC representation.
struct PmcEncodeSession
{
    /// Construct an encode session for the given mesh.
    /// \param ugm The UsdGeomMesh to encode
    /// \param options Encoder parameters
    PmcEncodeSession(const UsdGeomMesh& ugm, const VtDictionary& options)
    : _ugm(ugm), _options(options)
    {}

    /// Encode the mesh into PMC compressed format.
    /// \return Encoded mesh data as a byte vector
    std::vector<uint8_t> encode();

    struct CoordSys;

protected:
    /// Prepare PMC GeometryMeshPart from UsdGeomMesh geometry primitives
    void _setupGeom();

    /// Prepare PMC AttributeMeshParts from UsdGeomMesh attributes
    void _setupAttrs();

    /// Common setup for a single attribute
    /// \return life-time limited reference to created pmc attribute structure.
    /// NB: returned reference is invalidated by subsequent _setupAttr() call.
    pmc::AttributeMeshpart& _setupAttr(VtValue vals, VtArray<int> idxs);

    /// Prepare PMC AttributeMeshPart from primvar attributes
    void _setupPrimvar(const UsdGeomPrimvar& pv);

    /// Prepare PMC AttributeMeshPart for geometry subsets
    void _setupGeomSubsets();

    /// Prepare PMC AttributeMeshPart for crease attributes
    void _setupCreases();

    /// Configure the encode session (geometry, attributes)
    void _configurePmc();

    /// Perform the actual encoding
    std::vector<uint8_t> _encode();

    public:
    const UsdGeomMesh& _ugm;
    const VtDictionary& _options;

    pmc::Encoder _enc;
    pmc::GeometryMeshpart _gmp;
    std::vector<pmc::AttributeMeshpart> _amps;

    std::set<std::string> processedAttributes;
    std::set<std::string> processedSubSets;

    // Storage for converted buffers. Access them via the meshpart buffer.
    // todo: let pmc adopt these.
    std::vector<std::vector<int>> _converted;

    // Smart pointers to keep mesh data alive
    std::vector<VtValue> _keepAlive;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_PMC_ENCODE_SESSION_H
