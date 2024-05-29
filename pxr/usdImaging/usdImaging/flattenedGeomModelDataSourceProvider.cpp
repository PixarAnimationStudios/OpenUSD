//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/usdImaging/usdImaging/flattenedGeomModelDataSourceProvider.h"

#include "pxr/usdImaging/usdImaging/geomModelSchema.h"

#include "pxr/usd/usdGeom/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

bool
_ContainsDrawMode(const TfTokenVector &vec)
{
    for (const TfToken &token : vec) {
        if (token == UsdImagingGeomModelSchemaTokens->drawMode) {
            return true;
        }
    }
    return false;
}

class _ModelDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_ModelDataSource);

    TfTokenVector GetNames() override {
        TfTokenVector result = _primModel->GetNames();
        if (!_ContainsDrawMode(result) &&
             _ContainsDrawMode(_parentModel->GetNames())) {
            result.push_back(UsdImagingGeomModelSchemaTokens->drawMode);
        }

        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name != UsdImagingGeomModelSchemaTokens->drawMode) {
            return _primModel->Get(name);
        }
        if (HdTokenDataSourceHandle ds =
               UsdImagingGeomModelSchema(_primModel).GetDrawMode()) {
            const TfToken drawMode = ds->GetTypedValue(0.0f);
            if (!drawMode.IsEmpty() && drawMode != UsdGeomTokens->inherited) {
                return ds;
            }
        }
        return UsdImagingGeomModelSchema(_parentModel).GetDrawMode();
    }

    static
    HdContainerDataSourceHandle
    UseOrCreateNew(
        HdContainerDataSourceHandle const &primModel,
        HdContainerDataSourceHandle const &parentModel)
    {
        if (!primModel) {
            return parentModel;
        }
        if (!parentModel) {
            return primModel;
        }
        return New(primModel, parentModel);
    }
            
private:
    _ModelDataSource(
        HdContainerDataSourceHandle const &primModel,
        HdContainerDataSourceHandle const &parentModel)
      : _primModel(primModel)
      , _parentModel(parentModel)
    {
    }

    HdContainerDataSourceHandle const _primModel;
    HdContainerDataSourceHandle const _parentModel;
};

}

UsdImagingFlattenedGeomModelDataSourceProvider::~UsdImagingFlattenedGeomModelDataSourceProvider() = default;

HdContainerDataSourceHandle
UsdImagingFlattenedGeomModelDataSourceProvider::GetFlattenedDataSource(
    const Context &ctx) const
{
    return
        _ModelDataSource::UseOrCreateNew(
            ctx.GetInputDataSource(),
            ctx.GetFlattenedDataSourceFromParentPrim());
}

void
UsdImagingFlattenedGeomModelDataSourceProvider::ComputeDirtyLocatorsForDescendants(
    HdDataSourceLocatorSet * const locators) const
{
    static const HdDataSourceLocator drawModeLocator(
        UsdImagingGeomModelSchemaTokens->drawMode);
    static const HdDataSourceLocatorSet drawModeLocatorSet{
        drawModeLocator};

    // Only the draw mode is inherited by ancestors.
    if (locators->Intersects(drawModeLocator)) {
        *locators = drawModeLocatorSet;
    } else {
        *locators = HdDataSourceLocatorSet();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

