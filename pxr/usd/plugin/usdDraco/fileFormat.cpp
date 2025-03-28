//
// Copyright 2019 Google LLC
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "fileFormat.h"
#include "importTranslator.h"

#include "pxr/pxr.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/usd/usdaFileFormat.h"
#include "pxr/usd/sdf/layer.h"

#include <draco/compression/decode.h>

#include <fstream>
#include <memory>
#include <string>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PUBLIC_TOKENS(
    UsdDracoFileFormatTokens,
    USDDRACO_FILE_FORMAT_TOKENS);

TF_REGISTRY_FUNCTION(TfType) {
    SDF_DEFINE_FILE_FORMAT(UsdDracoFileFormat, SdfFileFormat);
}

UsdDracoFileFormat::UsdDracoFileFormat() :
        SdfFileFormat(UsdDracoFileFormatTokens->Id,
                      UsdDracoFileFormatTokens->Version,
                      UsdDracoFileFormatTokens->Target,
                      UsdDracoFileFormatTokens->Id) {
}

UsdDracoFileFormat::~UsdDracoFileFormat() {
}

bool UsdDracoFileFormat::CanRead(const std::string &filePath) const {
    // TODO: Read enough data to parse the header and check Draco string and
    // version, see PointCloudDecoder::DecodeHeader.
    return true;
}

bool UsdDracoFileFormat::_ReadFromChars(
        SdfLayer *layer, const char *str, size_t size,
        bool metadataOnly, std::string *outErr) const {
    // Create Draco decoder buffer from the given string.
    draco::DecoderBuffer buffer;
    buffer.Init(str, size);

    // Determine whether Draco data is a mesh or a point cloud.
    const draco::StatusOr<draco::EncodedGeometryType> maybeGeometryType =
        draco::Decoder::GetEncodedGeometryType(&buffer);
    if (!maybeGeometryType.ok()) {
        *outErr = "Failed to determine geometry type from Draco stream.";
        return false;
    }
    if (maybeGeometryType.value() == draco::POINT_CLOUD) {
        *outErr = "Draco point clouds are currently not supported.";
        return false;
    }

    // Decode Draco mesh from buffer.
    SdfLayerRefPtr dracoAsUsd;
    if (maybeGeometryType.value() == draco::TRIANGULAR_MESH) {
        std::unique_ptr<draco::Mesh> mesh;

        // Scope to delete decoder before translation, to reduce peak memory.
        {
            draco::Decoder decoder;
            draco::StatusOr<std::unique_ptr<draco::Mesh>> maybeMesh =
                decoder.DecodeMeshFromBuffer(&buffer);
            mesh = std::move(maybeMesh).value();
            if (!maybeMesh.ok() || mesh == nullptr) {
                *outErr = "Failed to decode mesh from Draco stream.";
                return false;
            }
        }

        // Translate Draco mesh to USD.
        dracoAsUsd = UsdDracoImportTranslator::Translate(*mesh.get());
    }
    if (!dracoAsUsd) {
        *outErr = "Failed to translate from Draco to USD.";
        return false;
    }

    // Move generated content into final layer.
    layer->TransferContent(dracoAsUsd);
    return true;
}

bool UsdDracoFileFormat::Read(
        SdfLayer *layer, const std::string &resolvedPath,
        bool metadataOnly) const {
    // Open an asset with Draco data.
    std::shared_ptr <ArAsset> asset = ArGetResolver().OpenAsset(
        ArResolvedPath(resolvedPath));
    if (!asset) {
        TF_RUNTIME_ERROR("Failed to open file \"%s\"", resolvedPath.c_str());
        return false;
    }

    std::string error;
    if (!_ReadFromChars(layer, asset->GetBuffer().get(), asset->GetSize(),
                        metadataOnly, &error)) {
        TF_RUNTIME_ERROR("Failed to read from Draco file \"%s\": %s",
            resolvedPath.c_str(), error.c_str());
        return false;
    }
    return true;
}

bool UsdDracoFileFormat::ReadFromString(SdfLayer *layer,
                                        const std::string &str) const {
    std::string error;
    if (!_ReadFromChars(layer, str.c_str(), str.size(), false, &error)) {
        TF_RUNTIME_ERROR("Failed to read data from Draco string: %s",
            error.c_str());
        return false;
    }
    return true;
}

bool UsdDracoFileFormat::WriteToFile(
        const SdfLayer &layer, const std::string &filePath,
        const std::string &comment, const FileFormatArguments &args) const {
    return false;
}

bool UsdDracoFileFormat::WriteToString(
        const SdfLayer &layer, std::string *str,
        const std::string &comment) const {
    // Draco format can only describe a subset of USD content, so falling back
    // to USDA file format instead.
    return SdfFileFormat::FindById(
        UsdUsdaFileFormatTokens->Id)->WriteToString(layer, str, comment);
}

bool UsdDracoFileFormat::WriteToStream(
        const SdfSpecHandle &spec, std::ostream &out, size_t indent) const
{
    // Draco format can only describe a subset of USD content, so falling back
    // to USDA file format instead.
    return SdfFileFormat::FindById(
        UsdUsdaFileFormatTokens->Id)->WriteToStream(spec, out, indent);
}


PXR_NAMESPACE_CLOSE_SCOPE
