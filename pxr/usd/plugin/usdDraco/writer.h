//
// Copyright 2019 Google LLC
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_PLUGIN_USD_DRACO_WRITER_H
#define PXR_USD_PLUGIN_USD_DRACO_WRITER_H

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/mesh.h"

#include <string>


PXR_NAMESPACE_OPEN_SCOPE


/// Encodes mesh and writes it in Draco format to a file at \p fileName.
bool UsdDraco_WriteDraco(const UsdGeomMesh &mesh,
                         const std::string &fileName,
                         int qp,
                         int qt,
                         int qn,
                         int cl,
                         int preservePolygons,
                         int preservePositionOrder,
                         int preserveHoles);

/// Checks whether a USD primvar can be encoded to Draco. It is called from
/// usdcompress.py script to determine whether a primvar should be deleted from
/// USD mesh or remain in USD mesh.
bool UsdDraco_PrimvarSupported(const UsdGeomPrimvar &primvar);


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_USD_PLUGIN_USD_DRACO_WRITER_H
