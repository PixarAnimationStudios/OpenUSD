//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include "pxr/usd/usdMedia/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

#define _ADD_TOKEN(cls, name) \
    cls.def_prop_ro_static(#name, [](nanobind::handle) { return UsdMediaTokens->name.GetString(); }, nanobind::rv_policy::copy);

void wrapUsdMediaTokens(nanobind::module_ &m)
{
    nanobind::class_<UsdMediaTokensType> cls(m, "Tokens");
    cls.def("__init__", [](nanobind::args, nanobind::kwargs) {
        TfPyThrowRuntimeError("This class cannot be instantiated from Python");
    });
    _ADD_TOKEN(cls, auralMode);
    _ADD_TOKEN(cls, authorship);
    _ADD_TOKEN(cls, authorship_MultipleApplyTemplate_Contact);
    _ADD_TOKEN(cls, authorship_MultipleApplyTemplate_CopyrightOwner);
    _ADD_TOKEN(cls, authorship_MultipleApplyTemplate_Created);
    _ADD_TOKEN(cls, authorship_MultipleApplyTemplate_Creator);
    _ADD_TOKEN(cls, authorship_MultipleApplyTemplate_Description);
    _ADD_TOKEN(cls, authorship_MultipleApplyTemplate_DigitalSourceType);
    _ADD_TOKEN(cls, authorship_MultipleApplyTemplate_Inputs);
    _ADD_TOKEN(cls, authorship_MultipleApplyTemplate_InstanceID);
    _ADD_TOKEN(cls, authorship_MultipleApplyTemplate_SoftwarePackage);
    _ADD_TOKEN(cls, authorship_MultipleApplyTemplate_SoftwareVersion);
    _ADD_TOKEN(cls, authorship_MultipleApplyTemplate_UsageTerms);
    _ADD_TOKEN(cls, defaultImage);
    _ADD_TOKEN(cls, endTime);
    _ADD_TOKEN(cls, filePath);
    _ADD_TOKEN(cls, gain);
    _ADD_TOKEN(cls, loopFromStage);
    _ADD_TOKEN(cls, loopFromStart);
    _ADD_TOKEN(cls, loopFromStartToEnd);
    _ADD_TOKEN(cls, mediaOffset);
    _ADD_TOKEN(cls, nonSpatial);
    _ADD_TOKEN(cls, onceFromStart);
    _ADD_TOKEN(cls, onceFromStartToEnd);
    _ADD_TOKEN(cls, playbackMode);
    _ADD_TOKEN(cls, previews);
    _ADD_TOKEN(cls, previewThumbnails);
    _ADD_TOKEN(cls, previewThumbnailsDefault);
    _ADD_TOKEN(cls, spatial);
    _ADD_TOKEN(cls, startTime);
    _ADD_TOKEN(cls, thumbnails);
    _ADD_TOKEN(cls, AssetPreviewsAPI);
    _ADD_TOKEN(cls, AuthorshipAPI);
    _ADD_TOKEN(cls, SpatialAudio);
}
