//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hio/image.h"
#include "pxr/imaging/hio/imageRegistry.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<HioImage>();
}

HioImage::~HioImage() = default;

/* static */
bool
HioImage::IsSupportedImageFile(std::string const & filename)
{
    HioImageRegistry & registry = HioImageRegistry::GetInstance();
    return registry.IsSupportedImageFile(filename);
}

/* static */
HioImageSharedPtr
HioImage::OpenForReading(std::string const & filename, int subimage,
                         int mip, SourceColorSpace sourceColorSpace, 
                         bool suppressErrors)
{
    HioImageRegistry & registry = HioImageRegistry::GetInstance();

    HioImageSharedPtr image = registry._ConstructImage(filename);
    if (!image || !image->_OpenForReading(filename, subimage, mip, 
                                          sourceColorSpace, suppressErrors)) {
        return HioImageSharedPtr();
    }

    return image;
}

/* static */
HioImageSharedPtr
HioImage::OpenForWriting(std::string const & filename)
{
    HioImageRegistry & registry = HioImageRegistry::GetInstance();

    HioImageSharedPtr image = registry._ConstructImage(filename);
    if (!image || !image->_OpenForWriting(filename)) {
        return HioImageSharedPtr();
    }

    return image;
}

PXR_NAMESPACE_CLOSE_SCOPE

