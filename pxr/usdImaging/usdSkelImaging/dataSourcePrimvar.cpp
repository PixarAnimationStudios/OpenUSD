//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdSkelImaging/dataSourcePrimvar.h"

PXR_NAMESPACE_OPEN_SCOPE

TfTokenVector 
UsdSkelImaging_DataSourcePrimvar::GetNames()
{
    static const TfTokenVector names = {
        HdPrimvarSchemaTokens->primvarValue,
        HdPrimvarSchemaTokens->interpolation,
        HdPrimvarSchemaTokens->role };
    return names;
}

HdDataSourceBaseHandle 
UsdSkelImaging_DataSourcePrimvar::Get(const TfToken &name)
{
    if (name == HdPrimvarSchemaTokens->primvarValue) {
        return _valueSource;
    }
    if (name == HdPrimvarSchemaTokens->interpolation) {
        return HdPrimvarSchema::BuildInterpolationDataSource(_interpolation);
    }
    if (name == HdPrimvarSchemaTokens->role) {
        return HdPrimvarSchema::BuildRoleDataSource(_role);
    }
    return nullptr;
}


UsdSkelImaging_DataSourcePrimvar::UsdSkelImaging_DataSourcePrimvar(
    HdDataSourceBaseHandle valueSource, 
    const TfToken& interpolation, const TfToken& role):
    _valueSource(std::move(valueSource)), 
    _interpolation(interpolation), _role(role)
{
}

PXR_NAMESPACE_CLOSE_SCOPE
