//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdMedia/assetPreviewsAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdMediaAssetPreviewsAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdMediaAssetPreviewsAPI::~UsdMediaAssetPreviewsAPI()
{
}

/* static */
UsdMediaAssetPreviewsAPI
UsdMediaAssetPreviewsAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdMediaAssetPreviewsAPI();
    }
    return UsdMediaAssetPreviewsAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdMediaAssetPreviewsAPI::_GetSchemaKind() const
{
    return UsdMediaAssetPreviewsAPI::schemaKind;
}

/* static */
bool
UsdMediaAssetPreviewsAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdMediaAssetPreviewsAPI>(whyNot);
}

/* static */
UsdMediaAssetPreviewsAPI
UsdMediaAssetPreviewsAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdMediaAssetPreviewsAPI>()) {
        return UsdMediaAssetPreviewsAPI(prim);
    }
    return UsdMediaAssetPreviewsAPI();
}

/* static */
const TfType &
UsdMediaAssetPreviewsAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdMediaAssetPreviewsAPI>();
    return tfType;
}

/* static */
bool 
UsdMediaAssetPreviewsAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdMediaAssetPreviewsAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdMediaAssetPreviewsAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdAPISchemaBase::GetSchemaAttributeNames(true);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/stagePopulationMask.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdMediaAssetPreviewsAPI::Thumbnails::Thumbnails(
    const SdfAssetPath &defaultImage)
    : defaultImage(defaultImage)
{
}

bool 
UsdMediaAssetPreviewsAPI::GetDefaultThumbnails(
    Thumbnails *defaultThumbnails) const
{
    if (!defaultThumbnails){
        TF_CODING_ERROR("Failed to provide valid out-parameter "
                        "`defaultThumbnails`");
        return false;
    }

    UsdPrim prim = GetPrim();

    if (!prim.HasAPI<UsdMediaAssetPreviewsAPI>()){
        return false;
    }

    VtValue thumbnailsVal = 
        prim.GetAssetInfoByKey(UsdMediaTokens->previewThumbnailsDefault);

    if (thumbnailsVal.IsHolding<VtDictionary>()){
        const VtDictionary &thumbnailsDict = 
            thumbnailsVal.UncheckedGet<VtDictionary>();
        // currently only care about the one key
        if (VtDictionaryIsHolding<SdfAssetPath>(thumbnailsDict, 
                                                UsdMediaTokens->defaultImage)){
            defaultThumbnails->defaultImage =
                thumbnailsDict.GetValueAtPath(UsdMediaTokens->defaultImage)->
                UncheckedGet<SdfAssetPath>();
            return true;
        }
    }

    return false;
}


void 
UsdMediaAssetPreviewsAPI::SetDefaultThumbnails(
    const Thumbnails &defaultThumbnails) const
{
    UsdPrim prim = GetPrim();

    VtDictionary  thumbnails;
    
    thumbnails[UsdMediaTokens->defaultImage] = defaultThumbnails.defaultImage;
    prim.SetAssetInfoByKey(UsdMediaTokens->previewThumbnailsDefault, 
                           VtValue(thumbnails));
}
    
void 
UsdMediaAssetPreviewsAPI::ClearDefaultThumbnails() const
{
    UsdPrim prim = GetPrim();

    prim.ClearAssetInfoByKey(UsdMediaTokens->previewThumbnailsDefault);
}
    
/* static */
UsdMediaAssetPreviewsAPI 
UsdMediaAssetPreviewsAPI::GetAssetDefaultPreviews(const std::string &layerPath)
{
    return GetAssetDefaultPreviews(SdfLayer::FindOrOpen(layerPath));
}
    
/* static */
UsdMediaAssetPreviewsAPI 
UsdMediaAssetPreviewsAPI::GetAssetDefaultPreviews(const SdfLayerHandle &layer)
{
    if (!layer) {
        return UsdMediaAssetPreviewsAPI();
    }
    
    SdfPath defaultPrimPath = layer->GetDefaultPrimAsPath();
    if (defaultPrimPath.IsEmpty()) {
        return UsdMediaAssetPreviewsAPI();
    }
    
    static const TfToken noSuchPrim("__No_Such_Prim__");
    // Technique to limit population to a maximum depth
    SdfPath maskPath = defaultPrimPath.AppendChild(noSuchPrim);
    UsdStagePopulationMask   mask({ maskPath });

    if (UsdStageRefPtr minimalStage = UsdStage::OpenMasked(layer, mask)){
        UsdPrim defaultPrim = minimalStage->GetDefaultPrim();
        UsdMediaAssetPreviewsAPI ap(defaultPrim);
        // Hold the stage in the schema object so that it will stay
        // alive for the schema object
        ap._defaultMaskedStage = std::move(minimalStage);
        return ap;
    }

    return UsdMediaAssetPreviewsAPI();
}

PXR_NAMESPACE_CLOSE_SCOPE
