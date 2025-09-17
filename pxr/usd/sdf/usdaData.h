//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_USDA_DATA_H
#define PXR_USD_SDF_USDA_DATA_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/data.h"
#include "pxr/usd/sdf/fileVersion.h"
#include "pxr/usd/ar/asset.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(SdfUsdaData);

// SdfUsdaData is an SdfData for text files. It has several static methods that
// are convenient for determining if a text file can be read or for parsing the
// header of a text file. Otherwise, it is just an SdfData that also keeps track
// of an SdfFileVersion for the layer.
class SdfUsdaData: public SdfData
{
public:
    SDF_API
    SdfUsdaData();

    SDF_API
    virtual ~SdfUsdaData();

    SDF_API
    static bool
    CanRead(const std::string& assetPath);

    SDF_API
    static bool
    CanRead(const std::string& assetPath,
            const std::shared_ptr<ArAsset>& asset);

    // Validate a layer's version string.
    //
    // If the version string represents a version that we can read, return the
    // SdfFileVersion and empty the string pointed to by reason. If the version
    // string cannot be read, return an invalid version and store an appropriate
    // error message in *reason.
    SDF_API
    static SdfFileVersion
    ValidateLayerVersionString(const std::string& versionStr,
                      std::string* reason);

    // The version of this layer.
    SDF_API
    SdfFileVersion
    GetLayerVersion() const
    {
        return _layerVersion;
    }

    // Set the version as parsed from the file (if it is a legal version)
    SDF_API
    void
    SetLayerVersion(const SdfFileVersion& version);

private:
    SdfFileVersion _layerVersion;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_USD_SDF_USDA_DATA_H
