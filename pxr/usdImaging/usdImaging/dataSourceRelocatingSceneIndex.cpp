//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/dataSourceRelocatingSceneIndex.h"

#include "pxr/usdImaging/usdImaging/usdPrimInfoSchema.h"

#include "pxr/imaging/hd/primvarsSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

static bool
_IsNativeInstance(const HdSceneIndexPrim& prim) 
{
    UsdImagingUsdPrimInfoSchema schema = 
        UsdImagingUsdPrimInfoSchema::GetFromParent(prim.dataSource);
    const HdPathDataSourceHandle pathDs = schema.GetNiPrototypePath();
    if (pathDs) {
        return !pathDs->GetTypedValue(0.0f).IsEmpty();
    }
    return false;
}

static void
_RelocateDataSource(HdSceneIndexPrim& prim, 
    const HdDataSourceLocator& srcLocator,
    const HdDataSourceLocator& dstLocator) 
{
    HdDataSourceBaseHandle src = 
        HdContainerDataSource::Get(prim.dataSource, srcLocator);

    if (!src) {
        return;
    }

    if (dstLocator.HasPrefix(HdPrimvarsSchema::GetDefaultLocator()) &&
        !srcLocator.HasPrefix(HdPrimvarsSchema::GetDefaultLocator())) {
        // InstanceProxyPathTranslation will later translate the 
        // instance proxy path in the primvar.
        src = HdPrimvarSchema::BuildRetained(
            /* primvarValue */ HdSampledDataSource::Cast(src),
            /* indexedPrimvarValue */ nullptr,
            /* indices */ nullptr,
            /* interpolation */ 
            HdPrimvarSchema::BuildInterpolationDataSource(
                HdPrimvarSchemaTokens->constant), 
            /* role */ HdPrimvarSchema::BuildRoleDataSource(TfToken()),
            /* colorSpace */ nullptr,
            /* elementSize */ nullptr);
    }
    HdContainerDataSourceEditor editor(prim.dataSource);
    editor.Set(dstLocator, src);
    editor.Set(srcLocator, HdBlockDataSource::New());
    prim.dataSource = editor.Finish();
}

HdSceneIndexPrim 
UsdImaging_DataSourceRelocatingSceneIndex::GetPrim(
    const SdfPath& primPath) const 
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
    if (!prim.dataSource) {
        return prim;
    }

    if (_forNativeInstance == _IsNativeInstance(prim)) {
        _RelocateDataSource(prim, _srcLocator, _dstLocator);
    }
    return prim;
}

void 
UsdImaging_DataSourceRelocatingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase& sender,
    const HdSceneIndexObserver::DirtiedPrimEntries& entries) 
{
    HdSceneIndexObserver::DirtiedPrimEntries newDirtiedEntries;
    for (const HdSceneIndexObserver::DirtiedPrimEntry& entry : entries) {
        if (entry.dirtyLocators.Contains(_srcLocator)) {
            newDirtiedEntries.push_back({entry.primPath, _dirtyDstLocators});
        }
        else {
            newDirtiedEntries.push_back(entry);
        }
    }
    _SendPrimsDirtied(newDirtiedEntries);
}

PXR_NAMESPACE_CLOSE_SCOPE
