//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdMedia/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdMediaTokensType::UsdMediaTokensType() :
    auralMode("auralMode", TfToken::Immortal),
    defaultImage("defaultImage", TfToken::Immortal),
    endTime("endTime", TfToken::Immortal),
    filePath("filePath", TfToken::Immortal),
    gain("gain", TfToken::Immortal),
    loopFromStage("loopFromStage", TfToken::Immortal),
    loopFromStart("loopFromStart", TfToken::Immortal),
    loopFromStartToEnd("loopFromStartToEnd", TfToken::Immortal),
    mediaOffset("mediaOffset", TfToken::Immortal),
    nonSpatial("nonSpatial", TfToken::Immortal),
    onceFromStart("onceFromStart", TfToken::Immortal),
    onceFromStartToEnd("onceFromStartToEnd", TfToken::Immortal),
    playbackMode("playbackMode", TfToken::Immortal),
    previews("previews", TfToken::Immortal),
    previewThumbnails("previews:thumbnails", TfToken::Immortal),
    previewThumbnailsDefault("previews:thumbnails:default", TfToken::Immortal),
    spatial("spatial", TfToken::Immortal),
    startTime("startTime", TfToken::Immortal),
    thumbnails("thumbnails", TfToken::Immortal),
    AssetPreviewsAPI("AssetPreviewsAPI", TfToken::Immortal),
    SpatialAudio("SpatialAudio", TfToken::Immortal),
    allTokens({
        auralMode,
        defaultImage,
        endTime,
        filePath,
        gain,
        loopFromStage,
        loopFromStart,
        loopFromStartToEnd,
        mediaOffset,
        nonSpatial,
        onceFromStart,
        onceFromStartToEnd,
        playbackMode,
        previews,
        previewThumbnails,
        previewThumbnailsDefault,
        spatial,
        startTime,
        thumbnails,
        AssetPreviewsAPI,
        SpatialAudio
    })
{
}

TfStaticData<UsdMediaTokensType> UsdMediaTokens;

PXR_NAMESPACE_CLOSE_SCOPE
