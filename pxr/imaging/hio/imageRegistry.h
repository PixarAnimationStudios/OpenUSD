//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HIO_IMAGE_REGISTRY_H
#define PXR_IMAGING_HIO_IMAGE_REGISTRY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hio/api.h"
#include "pxr/base/tf/singleton.h"

#include <memory>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

using HioImageSharedPtr = std::shared_ptr<class HioImage>;

class HioRankedTypeMap;

/// \class HioImageRegistry
///
/// Manages plugin registration and loading for HioImage subclasses.
///
class HioImageRegistry : public TfSingleton<HioImageRegistry>
{
public:
    HIO_API
    static HioImageRegistry& GetInstance();

    HIO_API
    bool IsSupportedImageFile(std::string const & filename);

private:
    friend class TfSingleton<HioImageRegistry>;
    HioImageRegistry();

    friend class HioImage;

    HioImageSharedPtr _ConstructImage(std::string const & filename);

private:
    std::unique_ptr<HioRankedTypeMap> const _typeMap;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HIO_IMAGE_REGISTRY_H
