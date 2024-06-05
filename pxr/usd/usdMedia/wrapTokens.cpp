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

namespace {

// Helper to return a static token as a string.  We wrap tokens as Python
// strings and for some reason simply wrapping the token using def_readonly
// bypasses to-Python conversion, leading to the error that there's no
// Python type for the C++ TfToken type.  So we wrap this functor instead.
class _WrapStaticToken {
public:
    _WrapStaticToken(const TfToken* token) : _token(token) { }

    std::string operator()() const
    {
        return _token->GetString();
    }

private:
    const TfToken* _token;
};

template <typename T>
void
_AddToken(T& cls, const char* name, const TfToken& token)
{
    cls.add_static_property(name,
                            boost::python::make_function(
                                _WrapStaticToken(&token),
                                boost::python::return_value_policy<
                                    boost::python::return_by_value>(),
                                boost::mpl::vector1<std::string>()));
}

} // anonymous

void wrapUsdMediaTokens()
{
    boost::python::class_<UsdMediaTokensType, boost::noncopyable>
        cls("Tokens", boost::python::no_init);
    _AddToken(cls, "auralMode", UsdMediaTokens->auralMode);
    _AddToken(cls, "defaultImage", UsdMediaTokens->defaultImage);
    _AddToken(cls, "endTime", UsdMediaTokens->endTime);
    _AddToken(cls, "filePath", UsdMediaTokens->filePath);
    _AddToken(cls, "gain", UsdMediaTokens->gain);
    _AddToken(cls, "loopFromStage", UsdMediaTokens->loopFromStage);
    _AddToken(cls, "loopFromStart", UsdMediaTokens->loopFromStart);
    _AddToken(cls, "loopFromStartToEnd", UsdMediaTokens->loopFromStartToEnd);
    _AddToken(cls, "mediaOffset", UsdMediaTokens->mediaOffset);
    _AddToken(cls, "nonSpatial", UsdMediaTokens->nonSpatial);
    _AddToken(cls, "onceFromStart", UsdMediaTokens->onceFromStart);
    _AddToken(cls, "onceFromStartToEnd", UsdMediaTokens->onceFromStartToEnd);
    _AddToken(cls, "playbackMode", UsdMediaTokens->playbackMode);
    _AddToken(cls, "previews", UsdMediaTokens->previews);
    _AddToken(cls, "previewThumbnails", UsdMediaTokens->previewThumbnails);
    _AddToken(cls, "previewThumbnailsDefault", UsdMediaTokens->previewThumbnailsDefault);
    _AddToken(cls, "spatial", UsdMediaTokens->spatial);
    _AddToken(cls, "startTime", UsdMediaTokens->startTime);
    _AddToken(cls, "thumbnails", UsdMediaTokens->thumbnails);
    _AddToken(cls, "AssetPreviewsAPI", UsdMediaTokens->AssetPreviewsAPI);
    _AddToken(cls, "SpatialAudio", UsdMediaTokens->SpatialAudio);
}
