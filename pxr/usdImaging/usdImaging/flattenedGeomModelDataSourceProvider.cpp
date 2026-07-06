//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/usdImaging/usdImaging/flattenedGeomModelDataSourceProvider.h"

#include "pxr/usdImaging/usdImaging/geomModelSchema.h"

#include "pxr/usd/usdGeom/tokens.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

bool
_ContainsDrawMode(const TfTokenVector &vec)
{
    return std::find(vec.begin(), vec.end(),
        UsdImagingGeomModelSchemaTokens->drawMode) != vec.end();
}

bool
_ContainsCardVisibility(const TfTokenVector &vec)
{
    return std::find(vec.begin(), vec.end(),
        UsdImagingGeomModelSchemaTokens->cardVisibility) != vec.end();
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
        if (!_ContainsCardVisibility(result) &&
             _ContainsCardVisibility(_parentModel->GetNames())) {
            result.push_back(
                UsdImagingGeomModelSchemaTokens->cardVisibility);
        }

        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == UsdImagingGeomModelSchemaTokens->drawMode) {
            return _GetDrawMode();
        }
        if (name == UsdImagingGeomModelSchemaTokens->cardVisibility) {
            return _GetCardVisibility();
        }
        return _primModel->Get(name);
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

    HdDataSourceBaseHandle _GetDrawMode() {
        if (HdTokenDataSourceHandle ds =
               UsdImagingGeomModelSchema(_primModel).GetDrawMode()) {
            const TfToken drawMode = ds->GetTypedValue(0.0f);
            if (!drawMode.IsEmpty() && drawMode != UsdGeomTokens->inherited) {
                return ds;
            }
        }
        return UsdImagingGeomModelSchema(_parentModel).GetDrawMode();
    }

    HdDataSourceBaseHandle _GetCardVisibility() {
        if (HdTokenDataSourceHandle ds =
               UsdImagingGeomModelSchema(_primModel).GetCardVisibility()) {
            const TfToken drawMode = ds->GetTypedValue(0.0f);
            if (!drawMode.IsEmpty() && drawMode != UsdGeomTokens->inherited) {
                return ds;
            }
        }
        return UsdImagingGeomModelSchema(_parentModel).GetCardVisibility();
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
    static const HdDataSourceLocator cardModeComplexityLocator(
        UsdImagingGeomModelSchemaTokens->cardVisibility);
    static const HdDataSourceLocatorSet drawModeLocatorSet{
        drawModeLocator};
    static const HdDataSourceLocatorSet cardModeComplexityLocatorSet{
        cardModeComplexityLocator};
    static const HdDataSourceLocatorSet drawModeAndCardModeComplexityLocatorSet{
        drawModeLocator, cardModeComplexityLocator};

    bool drawMode = locators->Intersects(drawModeLocator);
    bool cardInvis = locators->Intersects(cardModeComplexityLocator);
    if (drawMode && cardInvis) {
        *locators = drawModeAndCardModeComplexityLocatorSet;
    } else if (drawMode) {
        *locators = drawModeLocatorSet;
    } else if (cardInvis) {
        *locators = cardModeComplexityLocatorSet;
    } else {
        *locators = HdDataSourceLocatorSet();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

