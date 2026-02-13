//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_SKEL_IMAGING_DATA_SOURCE_PRIMVAR_H
#define PXR_USD_IMAGING_USD_SKEL_IMAGING_DATA_SOURCE_PRIMVAR_H

#include "pxr/usdImaging/usdSkelImaging/api.h"

#include "pxr/usdImaging/usdImaging/types.h"

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/primvarsSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdSkelImaging_DataSourcePrimvar
///
/// A primvar data source for UsdSkel's skinning primvars.
///
class UsdSkelImaging_DataSourcePrimvar final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdSkelImaging_DataSourcePrimvar);

    USDSKELIMAGING_API
    TfTokenVector GetNames() override;

    USDSKELIMAGING_API
    HdDataSourceBaseHandle Get(const TfToken& name) override;

private:
    UsdSkelImaging_DataSourcePrimvar(HdDataSourceBaseHandle valueSource, 
        const TfToken& interpolation=HdPrimvarSchemaTokens->constant, 
        const TfToken& role=TfToken());

    const HdDataSourceBaseHandle _valueSource;
    const TfToken _interpolation;
    const TfToken _role;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdSkelImaging_DataSourcePrimvar);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
