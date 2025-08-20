//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file sdf/fileIO.cpp

#include "pxr/pxr.h"

#include "pxr/usd/sdf/fileIO.h"

#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/fileIO_Common.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/usdaFileFormat.h"
#include "pxr/usd/sdf/variantSetSpec.h"
#include "pxr/usd/sdf/variantSpec.h"

#include "pxr/base/tf/stringUtils.h"

#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

// virtual
Sdf_StreamWritableAsset::~Sdf_StreamWritableAsset() = default;

static std::string
_ComposeHeader(const std::string cookie,
               const SdfFileVersion& version)
{
    TF_AXIOM(version);

    // Compose a suitable header string. See the long comment above titled
    // "Caveat developer!". This is the place to add padding or format the
    // version in base-16 or ...
    const std::string versionString = version.AsString();

    // Note that cookie includes a leading "#" (e.g., "#usda")
    std::string header = cookie + " " + versionString + "\n";

    return header;
}

bool
Sdf_TextOutput::WriteHeader(const std::string& cookie,
                            const SdfFileVersion version /*= SdfFileVersion()*/)
{
    SdfFileVersion outputVersion = version;
    if (!outputVersion) {
        // No explicit version, fall back to the default output version.
        outputVersion = SdfUsdaFileFormat::GetDefaultOutputVersion();
    }

    // XXX: should this be a TF_AXIOM instead?
    if (!TF_VERIFY(outputVersion,
                   "Could not get usda file version when writing to '%s'",
                   _name.c_str()))
    {
        return false;
    }

    if (!_FlushBuffer()) {
        return false;
    }

    // Remember what we're writing so we can tell if it has been upgraded
    _cookie = cookie;
    _requestedVersion = _writtenVersion = outputVersion;

    std::string header = _ComposeHeader(_cookie, _requestedVersion);

    return Write(header);
}

bool
Sdf_TextOutput::RequestWriteVersionUpgrade(const SdfFileVersion& ver,
                                           std::string reason)
{
    if (ver > SdfUsdaFileFormat::GetMaxOutputVersion()) {
        // The requested version cannot be written by this version of the
        // software. This is a coding error.
        TF_CODING_ERROR(
            "Failed upgrade of usda file '%s' to version %s. Version %s is the"
            " highest version that can be written.",
            _name.c_str(),
            ver.AsString().c_str(),
            SdfUsdaFileFormat::GetMaxOutputVersion().AsString().c_str());
        return false;
    }

    if (!_requestedVersion.CanRead(ver)) {
        TF_WARN("Upgrading usda file '%s' from version %s to %s: %s",
                _name.c_str(),
                _requestedVersion.AsString().c_str(),
                ver.AsString().c_str(),
                reason.c_str());
        _requestedVersion = ver;
    }

    return true;
}

bool
Sdf_TextOutput::_UpdateHeader()
{
    if (_writtenVersion && _requestedVersion > _writtenVersion) {
        bool ok = true;
        // Seek to the beginning (which flushes pending output first) and try to
        // write the header at the top of the file.
        if (_Seek(0)) {
            std::string header = _ComposeHeader(_cookie, _requestedVersion);
            ok = Write(header);
        } else {
            ok = false;
        }

        ok = _FlushBuffer() && ok;
        if (!ok) {
            TF_RUNTIME_ERROR("Failed to update the usda layer '%s' from"
                             " version '%s' to version '%s'.",
                             _name.c_str(),
                             _writtenVersion.AsString().c_str(),
                             _requestedVersion.AsString().c_str());
            return false;
        }

        _writtenVersion = _requestedVersion;

        return true;
    }

    // _UpdateHeader always flushes output.
    return _FlushBuffer();
}

bool
Sdf_WriteToStream(const SdfSpec &baseSpec, std::ostream& o, size_t indent)
{
    Sdf_TextOutput out(o, "<ostream>");

    const SdfSpecType type = baseSpec.GetSpecType();

    switch (type) {
    case SdfSpecTypePrim:
        {
            SdfPrimSpec spec =
                Sdf_CastAccess::CastSpec<SdfPrimSpec, SdfSpec>(baseSpec);
            return Sdf_WritePrim(spec, out, indent);
        }
    case SdfSpecTypeAttribute:
        {
            SdfAttributeSpec spec =
                Sdf_CastAccess::CastSpec<SdfAttributeSpec, SdfSpec>(baseSpec);
            return Sdf_WriteAttribute(spec, out, indent);
        }
    case SdfSpecTypeRelationship:
        {
            SdfRelationshipSpec spec =
                Sdf_CastAccess::CastSpec<SdfRelationshipSpec, SdfSpec>(baseSpec);
            return Sdf_WriteRelationship(spec, out, indent);
        }
    case SdfSpecTypeVariantSet:
        {
            SdfVariantSetSpec spec =
                Sdf_CastAccess::CastSpec<SdfVariantSetSpec, SdfSpec>(baseSpec);
            return Sdf_WriteVariantSet(spec, out, indent);
        }
    case SdfSpecTypeVariant:
        {
            SdfVariantSpec spec =
                Sdf_CastAccess::CastSpec<SdfVariantSpec, SdfSpec>(baseSpec);
            return Sdf_WriteVariant(spec, out, indent);
        }
    default:
        break;
    }

    TF_CODING_ERROR("Cannot write spec of type %s to stream", 
                    TfStringify(type).c_str());
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
