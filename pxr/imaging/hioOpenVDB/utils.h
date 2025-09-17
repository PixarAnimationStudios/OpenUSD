//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_IMAGING_HIO_OPENVDB_UTILS_H
#define PXR_IMAGING_HIO_OPENVDB_UTILS_H

/// \file hioOpenVDB/utils.h

#include "pxr/pxr.h"
#include "pxr/imaging/hioOpenVDB/api.h"

#include "openvdb/openvdb.h"
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// Return a shared pointer to an OpenVDB grid with \p name (or nullptr
/// if no grid matching \p name exists), given an \p assetPath.
HIOOPENVDB_API
openvdb::GridBase::Ptr
HioOpenVDBGridFromAsset(const std::string& name, const std::string& assetPath);

/// Return a shared pointer to a vector of OpenVDB grids,
/// given an \p assetPath.
HIOOPENVDB_API
openvdb::GridPtrVecPtr
HioOpenVDBGridsFromAsset(const std::string& assetPath);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HIO_OPENVDB_UTILS_H
