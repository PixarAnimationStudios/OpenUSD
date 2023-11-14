//
// Copyright 2023 Pixar
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

