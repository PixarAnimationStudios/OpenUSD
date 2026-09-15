//
// Copyright 2025 Apple
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

/// \file usdPmc/usdPmcFileFormat.cpp
///
/// Implementation of the USD file format plugin for PMC (compressed mesh)
/// files. This file format allows USD to directly read and interpret PMC
/// compressed mesh data as if it were a native USD layer, enabling seamless
/// integration of compressed meshes into USD workflows.
///
/// The file format:
/// - Registers the .pmc file extension with USD's file format system
/// - Provides read-only access to PMC compressed mesh data
/// - Automatically decompresses PMC data into USD mesh primitives

#include "usdPmcFileFormat.hpp"
#include "usdPmcDecoder.hpp"

#include "pxr/pxr.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/usdaFileFormat.h"
#include "pxr/usd/usdGeom/mesh.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(
    UsdPmcFileFormatTokens,
    USDPMC_FILE_FORMAT_TOKENS
);

TF_REGISTRY_FUNCTION(TfType) {
    SDF_DEFINE_FILE_FORMAT(UsdPmcFileFormat, SdfFileFormat);
}

UsdPmcFileFormat::UsdPmcFileFormat() :
    SdfFileFormat(UsdPmcFileFormatTokens->Id,
                  UsdPmcFileFormatTokens->Version,
                  UsdPmcFileFormatTokens->Target,
                  UsdPmcFileFormatTokens->Id) { }

UsdPmcFileFormat::~UsdPmcFileFormat() { }

bool
UsdPmcFileFormat::CanRead(const std::string& filePath) const {
    try {
        std::shared_ptr<ArAsset> bitstreamAsset =
            ArGetResolver().OpenAsset(ArResolvedPath(filePath));
        if (!bitstreamAsset) {
            return false;
        }

        const size_t length = bitstreamAsset->GetSize();
        if (length == 0) {
            return false;
        }

        // Inspect the bitstream to confirm it is a valid PMC stream rather
        // than claiming any file with a .pmc extension.
        UsdPmcMeshDecoder decoder;
        return decoder.CanDecode(bitstreamAsset->GetBuffer().get(), length);
    } catch (...) {
        return false;
    }
}

bool
UsdPmcFileFormat::Read(SdfLayer* layer,
                       const std::string& resolvedPath,
                       bool metadataOnly) const {
    try {
        // Open the PMC file as an asset
        std::shared_ptr<ArAsset> bitstreamAsset =
            ArGetResolver().OpenAsset(ArResolvedPath(resolvedPath));
        if (!bitstreamAsset) {
            TF_RUNTIME_ERROR("Failed to open file \"%s\"",
                             resolvedPath.c_str());
            return false;
        }

        // Decompress the PMC data into the USD layer
        std::string error;
        if (!_ReadFromBuffer(layer, bitstreamAsset->GetBuffer().get(),
                             bitstreamAsset->GetSize(), metadataOnly,
                             &error)) {
            TF_RUNTIME_ERROR("Failed to read from PMC file \"%s\": %s",
                             resolvedPath.c_str(), error.c_str());
            return false;
        }
        return true;
    } catch(...) {
        TF_RUNTIME_ERROR("Exception: Failed to read from PMC file \"%s\"",
                         resolvedPath.c_str());
        return false;
    }
}

bool
UsdPmcFileFormat::ReadFromString(SdfLayer* layer,
                                 const std::string& str) const {
    try {
        return SdfFileFormat::FindById(SdfUsdaFileFormatTokens->Id)
            ->ReadFromString(layer, str);
    } catch(...) {
        TF_RUNTIME_ERROR("ReadFromString: exception");
        return false;
    }
}

// Defer to the usda file format for this.
bool
UsdPmcFileFormat::WriteToString(const SdfLayer& layer,
                                std::string* str,
                                const std::string& comment) const {
    return SdfFileFormat::FindById(SdfUsdaFileFormatTokens->Id)
        ->WriteToString(layer, str, comment);
}

// Defer to the usda file format for this.
bool
UsdPmcFileFormat::WriteToStream(const SdfSpecHandle& spec,
                                std::ostream& out,
                                size_t indent) const {
    return SdfFileFormat::FindById(SdfUsdaFileFormatTokens->Id)
        ->WriteToStream(spec, out, indent);
}

bool
UsdPmcFileFormat::_ReadFromBuffer(SdfLayer* layer,
                                  const char* buffer,
                                  size_t length,
                                  bool metadataOnly,
                                  std::string* outErr) const {
    // Validate input parameters
    if (!layer) {
        if (outErr) {
            *outErr = "No layer specified";
        }
        return false;
    }

    if (!buffer) {
        if (outErr) {
            *outErr = "No buffer specified";
        }
        return false;
    }

    if (length == 0) {
        if (outErr) {
            *outErr = "Buffer length is 0";
        }
        return false;
    }

    // Create temporary USD objects for decompression
    SdfLayerRefPtr decodedUsdMeshLayer =
        SdfLayer::CreateAnonymous(".usda");
    UsdStageRefPtr stage = UsdStage::Open(decodedUsdMeshLayer);
    UsdGeomMesh decodedUsdMesh = UsdGeomMesh::Define(stage,
        SdfPath("/DecodedUsdMesh"));

    // Create PMC decoder instance
    UsdPmcMeshDecoder decoder;

    // Decode the PMC bitstream buffer into the USD mesh
    if (!decoder.Decode(buffer, length, &decodedUsdMesh)) {
        return false;
    }

    // Set the decoded mesh as the default primitive
    stage->SetDefaultPrim(decodedUsdMesh.GetPrim());

    // Transfer the decoded mesh data to the target layer
    layer->TransferContent(decodedUsdMeshLayer);
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
