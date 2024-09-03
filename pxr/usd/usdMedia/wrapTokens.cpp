//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include <boost/python/class.hpp>
#include "pxr/usd/usdMedia/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

#define _ADD_TOKEN(cls, name) \
    cls.add_static_property(#name, +[]() { return UsdMediaTokens->name.GetString(); });

void wrapUsdMediaTokens()
{
    boost::python::class_<UsdMediaTokensType, boost::noncopyable>
        cls("Tokens", boost::python::no_init);
    _ADD_TOKEN(cls, auralMode);
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
    _ADD_TOKEN(cls, SpatialAudio);
}
