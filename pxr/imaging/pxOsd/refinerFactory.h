//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_PX_OSD_REFINER_FACTORY_H
#define PXR_IMAGING_PX_OSD_REFINER_FACTORY_H

/// \file pxOsd/refinerFactory.h

#include "pxr/pxr.h"
#include "pxr/imaging/pxOsd/api.h"
#include "pxr/imaging/pxOsd/meshTopology.h"
#include "pxr/base/vt/array.h"

#include <opensubdiv/far/topologyRefiner.h>

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


typedef std::shared_ptr<class OpenSubdiv::Far::TopologyRefiner> 
    PxOsdTopologyRefinerSharedPtr;

class PxOsdRefinerFactory {

public:

    /// Returns an OpenSubdiv TopologyRefiner
    PXOSD_API
    static PxOsdTopologyRefinerSharedPtr Create(
        PxOsdMeshTopology const & topology, TfToken name=TfToken());

    /// Returns an OpenSubdiv TopologyRefiner with faceVarying topology
    PXOSD_API
    static PxOsdTopologyRefinerSharedPtr Create(
        PxOsdMeshTopology const & topology,
        std::vector<VtIntArray> const &fvarTopologies,
        TfToken name=TfToken());

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PX_OSD_REFINER_FACTORY_H
