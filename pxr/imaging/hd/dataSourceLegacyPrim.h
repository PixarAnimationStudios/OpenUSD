//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_DATA_SOURCE_LEGACY_PRIM_H
#define PXR_IMAGING_HD_DATA_SOURCE_LEGACY_PRIM_H

#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceLocator.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"

#include "pxr/pxr.h"

#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;

#define HD_LEGACY_PRIMTYPE_TOKENS  \
    /* Bprims */                   \
    (openvdbAsset)                 \
    (field3dAsset)

TF_DECLARE_PUBLIC_TOKENS(HdLegacyPrimTypeTokens, HD_API, 
                         HD_LEGACY_PRIMTYPE_TOKENS);

/// Instancers from scene delegates ignore visibility.
/// This fixes that usdImaging does not update the visibility of an instancer
/// properly.
#define HD_LEGACY_FLAG_TOKENS \
    (isLegacyInstancer)

TF_DECLARE_PUBLIC_TOKENS(HdLegacyFlagTokens, HD_API, 
                         HD_LEGACY_FLAG_TOKENS);

/// \class HdDataSourceLegacyPrim
///
/// This is an HdContainerDataSource which represents a prim-level data source
/// for adapting HdSceneDelegate calls into the forms defined by HdSchemas
/// during emulation of legacy scene delegates.
///
class HdDataSourceLegacyPrim : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(HdDataSourceLegacyPrim);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    /// This clears internal cached values and is currently called only by
    /// HdLegacyPrimSceneIndex in response to its own DirtyPrims method
    void PrimDirtied(const HdDataSourceLocatorSet &locators);

    /// Return which locators PrimDirtied will respond to...
    static const HdDataSourceLocatorSet &GetCachedLocators();

protected:
    HdDataSourceLegacyPrim(
        const SdfPath& id, 
        const TfToken& type, 
        HdSceneDelegate *sceneDelegate);

private:
    HdDataSourceBaseHandle _GetPrimvarsDataSource();
    HdDataSourceBaseHandle _GetExtComputationPrimvarsDataSource();
    HdDataSourceBaseHandle _GetMaterialBindingsDataSource();
    HdDataSourceBaseHandle _GetXformDataSource();
    HdDataSourceBaseHandle _GetMaterialDataSource();
    HdDataSourceBaseHandle _GetIntegratorDataSource();
    HdDataSourceBaseHandle _GetSampleFilterDataSource();
    HdDataSourceBaseHandle _GetDisplayFilterDataSource();
    HdDataSourceBaseHandle _GetDisplayStyleDataSource();
    HdDataSourceBaseHandle _GetInstancedByDataSource();
    HdDataSourceBaseHandle _GetInstancerTopologyDataSource();
    HdDataSourceBaseHandle _GetVolumeFieldBindingDataSource();
    HdDataSourceBaseHandle _GetCoordSysBindingDataSource();
    HdDataSourceBaseHandle _GetVisibilityDataSource();
    HdDataSourceBaseHandle _GetPurposeDataSource();
    HdDataSourceBaseHandle _GetExtentDataSource();
    HdDataSourceBaseHandle _GetCategoriesDataSource();
    HdDataSourceBaseHandle _GetInstanceCategoriesDataSource();

    bool _IsLight();
    bool _IsInstanceable();

protected:
    SdfPath _id;
    TfToken _type;
    HdSceneDelegate *_sceneDelegate;

private:
    std::atomic_bool _primvarsBuilt;
    bool _extComputationPrimvarsBuilt : 1;

    HdContainerDataSourceAtomicHandle _primvars;
    HdContainerDataSourceHandle _extComputationPrimvars;

    // Note: _instancerTopology needs to be an atomic handle, since
    // some downstream customers of it (render index sync, hdSt instancer sync)
    // are not threadsafe.
    HdContainerDataSourceAtomicHandle _instancerTopology;
};

HD_DECLARE_DATASOURCE_HANDLES(HdDataSourceLegacyPrim);

bool HdLegacyPrimTypeIsVolumeField(TfToken const &primType);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
