//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HIO_OPENVDB_ASSET_INTERFACE_H
#define PXR_IMAGING_HIO_OPENVDB_ASSET_INTERFACE_H

/// \file hioOpenVDB/vdbAssetInterface.h

#include "pxr/pxr.h"
#include "pxr/usd/ar/asset.h"

#include "openvdb/openvdb.h"

#include "pxr/imaging/hioOpenVDB/api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HioOpenVDBArAssetInterface
///
/// Interface for an ArAsset subclass that enables direct access to
/// OpenVDB grids.
class HioOpenVDBArAssetInterface : public ArAsset
{
public:
    /// Return a shared pointer to an OpenVDB grid with \p name,
    /// or nullptr if no grid matching \p name exists.
    virtual openvdb::GridBase::Ptr GetGrid(const std::string& name) const = 0;

    /// Return a shared pointer to a vector of OpenVDB grids.
    virtual openvdb::GridPtrVecPtr GetGrids() const = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HIO_OPENVDB_ASSET_INTERFACE_H
