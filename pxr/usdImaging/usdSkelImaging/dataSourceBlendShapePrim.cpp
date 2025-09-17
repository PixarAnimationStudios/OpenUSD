//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdSkelImaging/dataSourceBlendShapePrim.h"

#include "pxr/usdImaging/usdSkelImaging/blendShapeSchema.h"
#include "pxr/usdImaging/usdSkelImaging/inbetweenShapeSchema.h"

#include "pxr/usdImaging/usdImaging/dataSourceMapped.h"

#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"

#include "pxr/usd/usdSkel/blendShape.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

TF_DEFINE_PRIVATE_TOKENS(
    _usdSkelPrefixTokens,

    (inbetweens)
);

std::vector<UsdImagingDataSourceMapped::PropertyMapping>
_GetPropertyMappings()
{
    std::vector<UsdImagingDataSourceMapped::PropertyMapping> result;

    for (const TfToken &usdName :
             UsdSkelBlendShape::GetSchemaAttributeNames(
                 /* includeInherited = */ false)) {
        result.push_back(
            UsdImagingDataSourceMapped::AttributeMapping{
                usdName, HdDataSourceLocator(usdName) });
    }

    return result;
}

const UsdImagingDataSourceMapped::PropertyMappings &
_GetMappings() {
    static const UsdImagingDataSourceMapped::PropertyMappings result(
        _GetPropertyMappings(),
        UsdSkelImagingBlendShapeSchema::GetDefaultLocator());
    return result;
}

// Data source for UsdSkelImagingInbetweenShapeSchema at
// data source locator skelBlendShape:inbetweenShapes:NAME.
//
// Takes data from USD attributes of BlendShape prefixed by
// inbetweens:NAME.
//
class _InbetweenShapeSchemaDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_InbetweenShapeSchemaDataSource);

    TfTokenVector GetNames() override {
        return UsdSkelImagingInbetweenShapeSchemaTokens->allTokens;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == UsdSkelImagingInbetweenShapeSchemaTokens->weight) {
            float result;
            if (!_inbetweenShape.GetWeight(&result)) {
                return nullptr;
            }
            return
                HdRetainedTypedSampledDataSource<float>::New(result);
        }

        if (name == UsdSkelImagingInbetweenShapeSchemaTokens->offsets) {
            return
                UsdImagingDataSourceAttribute<VtArray<GfVec3f>>::New(
                    _inbetweenShape.GetAttr(),
                    _stageGlobals);
        }

        if (name == UsdSkelImagingInbetweenShapeSchemaTokens->normalOffsets) {
            return
                UsdImagingDataSourceAttribute<VtArray<GfVec3f>>::New(
                    _inbetweenShape.GetNormalOffsetsAttr(),
                    _stageGlobals);
        }

        return nullptr;
    }

private:
    _InbetweenShapeSchemaDataSource(
            const UsdSkelInbetweenShape &inbetweenShape,
            const UsdImagingDataSourceStageGlobals &stageGlobals)
     : _inbetweenShape(inbetweenShape)
     , _stageGlobals(stageGlobals)
    {}

    const UsdSkelInbetweenShape _inbetweenShape;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
};

// Data source for UsdSkelImagingInbetweenShapeSchema
// at skelBlendShape.
//
class _InbetweenShapeContainerSchemaDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_InbetweenShapeContainerSchemaDataSource);

    TfTokenVector GetNames() override {
        TfTokenVector result;
        for (const UsdSkelInbetweenShape &shape
                 : _blendShape.GetAuthoredInbetweens()) {
            if (std::optional<TfToken> name =
                    _GetInbetweenName(shape.GetAttr().GetName())) {
                result.push_back(std::move(*name));
            }
        }
        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        UsdSkelInbetweenShape inbetween(_blendShape.GetInbetween(name));
        if (!inbetween) {
            return nullptr;
        }

        return _InbetweenShapeSchemaDataSource::New(
            std::move(inbetween), _stageGlobals);
    }

    static HdDataSourceLocatorSet Invalidate(const TfTokenVector &properties) {
        HdDataSourceLocatorSet result;

        for (const TfToken &property : properties) {
            if (std::optional<TfToken> name = _GetInbetweenName(property)) {
                result.insert(
                    UsdSkelImagingBlendShapeSchema::GetInbetweenShapesLocator()
                        .Append(std::move(*name)));
            }
        }

        return result;
    }

private:
    _InbetweenShapeContainerSchemaDataSource(
            const UsdSkelBlendShape &blendShape,
            const UsdImagingDataSourceStageGlobals &stageGlobals)
     : _blendShape(blendShape)
     , _stageGlobals(stageGlobals)
    {}

    static
    std::optional<TfToken> _GetInbetweenName(const TfToken &usdAttrName)
    {
        const std::pair<std::string, bool> strippedName =
            SdfPath::StripPrefixNamespace(
                usdAttrName.GetString(),
                _usdSkelPrefixTokens->inbetweens.GetString());
        if (!strippedName.second) {
            return {};
        }
        return TfToken(strippedName.first);
    }

    const UsdSkelBlendShape _blendShape;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
};

}

// ----------------------------------------------------------------------------

UsdSkelImagingDataSourceBlendShapePrim::
UsdSkelImagingDataSourceBlendShapePrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
 : UsdImagingDataSourcePrim(sceneIndexPath, std::move(usdPrim), stageGlobals)
{
}

TfTokenVector
UsdSkelImagingDataSourceBlendShapePrim::GetNames()
{
    TfTokenVector result = UsdImagingDataSourcePrim::GetNames();
    result.push_back(UsdSkelImagingBlendShapeSchema::GetSchemaToken());
    return result;
}

HdDataSourceBaseHandle
UsdSkelImagingDataSourceBlendShapePrim::Get(const TfToken & name)
{
    if (name == UsdSkelImagingBlendShapeSchema::GetSchemaToken()) {
        return
            HdOverlayContainerDataSource::New(
                HdRetainedContainerDataSource::New(
                    UsdSkelImagingBlendShapeSchemaTokens->inbetweenShapes,
                    _InbetweenShapeContainerSchemaDataSource::New(
                        UsdSkelBlendShape(_GetUsdPrim()),
                        _GetStageGlobals())),
                UsdImagingDataSourceMapped::New(
                    _GetUsdPrim(),
                    _GetSceneIndexPath(),
                    _GetMappings(),
                    _GetStageGlobals()));
    }

    return UsdImagingDataSourcePrim::Get(name);
}

HdDataSourceLocatorSet
UsdSkelImagingDataSourceBlendShapePrim::Invalidate(
    UsdPrim const& prim,
    const TfToken &subprim,
    const TfTokenVector &properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    TRACE_FUNCTION();

    HdDataSourceLocatorSet locators =
        UsdImagingDataSourceGprim::Invalidate(
            prim, subprim, properties, invalidationType);
    
    locators.insert(
        UsdImagingDataSourceMapped::Invalidate(
            properties, _GetMappings()));

    locators.insert(
        _InbetweenShapeContainerSchemaDataSource::Invalidate(
            properties));

    return locators;
}

PXR_NAMESPACE_CLOSE_SCOPE
