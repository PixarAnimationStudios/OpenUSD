//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USDMTLX_READER_H
#define PXR_USD_USDMTLX_READER_H
 
#include "pxr/pxr.h"
#include "pxr/usd/usdMtlx/api.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/declarePtrs.h"
#include <MaterialXCore/Document.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_PTRS(UsdStage);

/// Translate the MaterialX document in \p mtlx into the stage \p stage.
/// \p internalPath is a namespace path where converted MaterialX objects
/// will live. \p externalPath is a namespace path of a prim that will
/// have all of the look variants.  It will have references into
/// \p internalPath.  Clients are expected to reference the prim at
/// \p externalPath to apply looks.
///
/// Note that this method will traverse the materialx network following material
/// nodes, and does an early exit if no looks are defined in the mtlx document.
USDMTLX_API
void UsdMtlxRead(const MaterialX::ConstDocumentPtr& mtlx,
                 const UsdStagePtr& stage,
                 const SdfPath& internalPath = SdfPath("/MaterialX"),
                 const SdfPath& externalPath = SdfPath("/ModelRoot"));

/// Translate node graphs in the MaterialX document in \p mtlx into the
/// stage \p stage.  \p internalPath is a namespace path where converted
/// MaterialX objects will live.
///
/// Note that this method will traverse all the nodegraphs in the mtlx document.
USDMTLX_API
void UsdMtlxReadNodeGraphs(const MaterialX::ConstDocumentPtr& mtlx,
                           const UsdStagePtr& stage,
                           const SdfPath& internalPath = SdfPath("/MaterialX"));

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USDMTLX_READER_H
