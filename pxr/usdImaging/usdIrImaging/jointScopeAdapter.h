//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IR_IMAGING_JOINT_SCOPE_ADAPTER_H
#define PXR_USD_IMAGING_USD_IR_IMAGING_JOINT_SCOPE_ADAPTER_H

/// \file

#include "pxr/pxr.h"

#include "pxr/usdImaging/usdImaging/sceneIndexPrimAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

/// A prim adapter that generates guide geometry for IrJointScope prims.
///
/// Draws a sphere at the location of the joint scope and a cone that points
/// along the +Z axis. The length of the cone is determined by the value of the
/// 'guide:length' attribute. The appearance of the guides are determined by the
/// values of the 'guide:displayColor' and 'guide:displayOpacity' attributes.
///
class UsdIrImagingJointScopeAdapter : public UsdImagingSceneIndexPrimAdapter
{
public:
    using BaseAdapter = UsdImagingSceneIndexPrimAdapter;
 
    TfTokenVector GetImagingSubprims(
        UsdPrim const& prim) override;
 
    TfToken GetImagingSubprimType(
        UsdPrim const& prim,
        TfToken const& subprim) override;
 
    HdContainerDataSourceHandle GetImagingSubprimData(
        UsdPrim const& prim,
        TfToken const& subprim,
        const UsdImagingDataSourceStageGlobals &stageGlobals) override;
 
    HdDataSourceLocatorSet InvalidateImagingSubprim(
        UsdPrim const& prim,
        TfToken const& subprim,
        TfTokenVector const& properties,
        UsdImagingPropertyInvalidationType invalidationType) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
