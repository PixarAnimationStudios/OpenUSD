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
    authorship("authorship", TfToken::Immortal),
    authorship_MultipleApplyTemplate_Contact("authorship:__INSTANCE_NAME__:contact", TfToken::Immortal),
    authorship_MultipleApplyTemplate_CopyrightOwner("authorship:__INSTANCE_NAME__:copyrightOwner", TfToken::Immortal),
    authorship_MultipleApplyTemplate_Created("authorship:__INSTANCE_NAME__:created", TfToken::Immortal),
    authorship_MultipleApplyTemplate_Creator("authorship:__INSTANCE_NAME__:creator", TfToken::Immortal),
    authorship_MultipleApplyTemplate_Description("authorship:__INSTANCE_NAME__:description", TfToken::Immortal),
    authorship_MultipleApplyTemplate_DigitalSourceType("authorship:__INSTANCE_NAME__:digitalSourceType", TfToken::Immortal),
    authorship_MultipleApplyTemplate_InstanceID("authorship:__INSTANCE_NAME__:instanceID", TfToken::Immortal),
    authorship_MultipleApplyTemplate_PromptInputNames("authorship:__INSTANCE_NAME__:prompt:inputNames", TfToken::Immortal),
    authorship_MultipleApplyTemplate_PromptInputValues("authorship:__INSTANCE_NAME__:prompt:inputValues", TfToken::Immortal),
    authorship_MultipleApplyTemplate_SoftwarePackage("authorship:__INSTANCE_NAME__:softwarePackage", TfToken::Immortal),
    authorship_MultipleApplyTemplate_SoftwareVersion("authorship:__INSTANCE_NAME__:softwareVersion", TfToken::Immortal),
    authorship_MultipleApplyTemplate_UsageTerms("authorship:__INSTANCE_NAME__:usageTerms", TfToken::Immortal),
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
    AuthorshipAPI("AuthorshipAPI", TfToken::Immortal),
    SpatialAudio("SpatialAudio", TfToken::Immortal),
    allTokens({
        auralMode,
        authorship,
        authorship_MultipleApplyTemplate_Contact,
        authorship_MultipleApplyTemplate_CopyrightOwner,
        authorship_MultipleApplyTemplate_Created,
        authorship_MultipleApplyTemplate_Creator,
        authorship_MultipleApplyTemplate_Description,
        authorship_MultipleApplyTemplate_DigitalSourceType,
        authorship_MultipleApplyTemplate_InstanceID,
        authorship_MultipleApplyTemplate_PromptInputNames,
        authorship_MultipleApplyTemplate_PromptInputValues,
        authorship_MultipleApplyTemplate_SoftwarePackage,
        authorship_MultipleApplyTemplate_SoftwareVersion,
        authorship_MultipleApplyTemplate_UsageTerms,
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
        AuthorshipAPI,
        SpatialAudio
    })
{
}

TfStaticData<UsdMediaTokensType> UsdMediaTokens;

PXR_NAMESPACE_CLOSE_SCOPE
