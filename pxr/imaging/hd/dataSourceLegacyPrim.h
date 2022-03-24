//
// Copyright 2021 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef PXR_IMAGING_HD_DATA_SOURCE_LEGACY_PRIM_H
#define PXR_IMAGING_HD_DATA_SOURCE_LEGACY_PRIM_H

#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;
class HdExtComputationContext;

#define HD_LEGACY_PRIMTYPE_TOKENS  \
    /* Bprims */                   \
    (openvdbAsset)                 \
    (field3dAsset)

TF_DECLARE_PUBLIC_TOKENS(HdLegacyPrimTypeTokens, HD_API, 
                         HD_LEGACY_PRIMTYPE_TOKENS);

/// \class HdExtComputationCallbackDataSource
///
/// This is a data source which holds a legacy ext computation. It is used
/// only during emulation of legacy scene delegates but is exposed here as it
/// is used by HdSceneIndexAdapterSceneDelegate for emulation of legacy
/// render delegates.
///
class HdExtComputationCallbackDataSource : public HdDataSourceBase
{
public:
    HD_DECLARE_DATASOURCE(HdExtComputationCallbackDataSource);

    HdExtComputationCallbackDataSource(
        const SdfPath &id, HdSceneDelegate *sceneDelegate)
    : _id(id), _sceneDelegate(sceneDelegate) {}

    void Invoke(HdExtComputationContext *context);

private:
    SdfPath _id;
    HdSceneDelegate *_sceneDelegate;
};

HD_DECLARE_DATASOURCE_HANDLES(HdExtComputationCallbackDataSource);

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

    bool Has(const TfToken &name) override;
    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    /// This clears internal cached values and is currently called only by
    /// HdLegacyPrimSceneIndex in response to its own DirtyPrims method
    void PrimDirtied(const HdDataSourceLocatorSet &locators);

    /// Return which locators PrimDirtied will respond to...
    static const HdDataSourceLocatorSet &GetCachedLocators();

private:
    HdDataSourceLegacyPrim(
        SdfPath id, 
        TfToken type, 
        HdSceneDelegate *sceneDelegate);

    HdDataSourceBaseHandle _GetPrimvarsDataSource();
    HdDataSourceBaseHandle _GetExtComputationPrimvarsDataSource();
    HdDataSourceBaseHandle _GetMaterialBindingDataSource();
    HdDataSourceBaseHandle _GetXformDataSource();
    HdDataSourceBaseHandle _GetMaterialDataSource();
    HdDataSourceBaseHandle _GetDisplayStyleDataSource();
    HdDataSourceBaseHandle _GetInstancedByDataSource();
    HdDataSourceBaseHandle _GetDataSharingDataSource();
    HdDataSourceBaseHandle _GetInstancerTopologyDataSource();
    HdDataSourceBaseHandle _GetVolumeFieldBindingDataSource();
    HdDataSourceBaseHandle _GetCoordSysBindingDataSource();
    HdDataSourceBaseHandle _GetVisibilityDataSource();
    HdDataSourceBaseHandle _GetPurposeDataSource();
    HdDataSourceBaseHandle _GetExtentDataSource();
    HdDataSourceBaseHandle _GetCategoriesDataSource();
    HdDataSourceBaseHandle _GetInstanceCategoriesDataSource();
    HdDataSourceBaseHandle _GetPrmanParamsDataSource();

private:
    SdfPath _id;
    TfToken _type;
    HdSceneDelegate *_sceneDelegate;

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
