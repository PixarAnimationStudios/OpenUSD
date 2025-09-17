//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/usdaData.h"
#include "pxr/usd/sdf/usdaFileFormat.h"

#include "pxr/usd/sdf/fileIO.h"

PXR_NAMESPACE_OPEN_SCOPE

SdfUsdaData::SdfUsdaData()
{
    // Note that _layerVersion is invalid for newly constructed SdfUsdaData
    // objects.
}

// virtual
SdfUsdaData::~SdfUsdaData()
{
    // nothing
}

// static
SdfFileVersion
SdfUsdaData::ValidateLayerVersionString(const std::string& versionStr,
                                        std::string* reason)
{
    SdfFileVersion version = SdfFileVersion::FromString(versionStr);
    if (!version) {
        *reason = TfStringPrintf(
            "Unable to parse layer version from '%s'.",
            versionStr.c_str());
    } else if (version < SdfUsdaFileFormat::GetMinInputVersion()) {
        *reason = TfStringPrintf(
            "Cannot parse layer version '%s'. The minimum supported"
            " version is '%s'.",
            version.AsString().c_str(),
            SdfUsdaFileFormat::GetMinInputVersion().AsString().c_str());
    } else if (!SdfUsdaFileFormat::GetMaxInputVersion().CanRead(version)) {
        *reason = TfStringPrintf(
            "Cannot parse layer version '%s'. The maximum supported"
            " version is '%s'.",
            version.AsString().c_str(),
            SdfUsdaFileFormat::GetMaxInputVersion().AsString().c_str());
    } else {
        // Success, clear reason and return.
        reason->clear();
        return version;
    }

    return SdfFileVersion();
}

void
SdfUsdaData::SetLayerVersion(const SdfFileVersion& version)
{
    // Allowed to set it to invalid version.
    if (!version || SdfUsdaFileFormat::GetMaxOutputVersion().CanWrite(version))
    {
        _layerVersion = version;
    }
    else
    {
        // Coding error because this method is internal only and we
        // shouldn't make this mistake.
        TF_CODING_ERROR("Version '%s' is not a valid version for a usda file.",
                        version.AsString().c_str());
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
