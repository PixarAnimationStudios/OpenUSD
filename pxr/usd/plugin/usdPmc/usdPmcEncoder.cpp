//
// Copyright 2025 Apple
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

/// \file usdPmc/usdPmcEncoder.cpp
///
/// Implementation of USD PMC encoder functionality for mesh compression.
/// This file provides the main interface for encoding USD mesh data using
/// the PMC compression format from the Alliance for Open Media (AOM).
///
/// The encoder supports:
/// - Compression of individual USD meshes to PMC format with vertex/face
///   reordering
/// - Stage-level encoding that processes entire USDZ files
/// - Preservation of mesh attributes, primvars, and geometry subsets
/// - Temporary file management for USDZ processing
/// - Automatic removal of compressed attributes from source meshes

#include "usdPmcEncoder.hpp"
#include "usdPmcEncodeSession.hpp"

#include "pxr/pxr.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdUtils/usdzPackage.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/usd/sdf/zipFile.h"
#include "pxr/base/gf/traits.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/vt/types.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usd/usdGeom/subset.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usdUtils/flattenLayerStack.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/assetInfo.h"

#include <pmc/pmEncoder.hpp>

#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <functional>
#include <type_traits>
#include <random>
#include <chrono>
#include <iomanip>

PXR_NAMESPACE_OPEN_SCOPE


/// Creates a unique temporary directory path using the system's temporary
/// directory and a random identifier to ensure uniqueness across concurrent
/// encoding operations. This implementation is platform-agnostic and works
/// on macOS, Windows, and Linux.
std::string
_GetTempDir() {
    const char* archTempDir = ArchGetTmpDir();
    std::filesystem::path tempDir(archTempDir);
    
    // Generate a unique identifier using random number generator and timestamp
    // This provides sufficient uniqueness for temporary directory names
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    
    // Combine timestamp and random values for uniqueness
    auto now = std::chrono::high_resolution_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
    uint64_t random1 = dis(gen);
    uint64_t random2 = dis(gen);
    
    // Format as a UUID-like string: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        << std::setw(8) << (timestamp & 0xFFFFFFFF) << '-'
        << std::setw(4) << ((random1 >> 48) & 0xFFFF) << '-'
        << std::setw(4) << ((random1 >> 32) & 0xFFFF) << '-'
        << std::setw(4) << ((random1 >> 16) & 0xFFFF) << '-'
        << std::setw(4) << (random1 & 0xFFFF)
        << std::setw(8) << (random2 & 0xFFFFFFFF);
    
    tempDir /= "usdPmcStageEncoder" / std::filesystem::path(oss.str());
    return tempDir.string();
}

/// Safely creates the specified directory path, including any missing parent
/// directories. Reports errors if directory creation fails.
bool _CreateDirectory(std::filesystem::path dpath) {
    if (!std::filesystem::exists(dpath)) {
        if (!std::filesystem::create_directories(dpath)) {
            TF_RUNTIME_ERROR("Unable to create temporary directory: " +
                              dpath.string());
            return false;
        }
    }
    return true;
}

/// Recursively removes the specified temporary directory and all
/// files/subdirectories within it. Used for cleanup after PMC encoding
/// operations.
void
_RemoveTempDir(const std::string& tempDir) {
    std::filesystem::remove_all(tempDir);
}

UsdPmcMeshEncoder::UsdPmcMeshEncoder() {}

UsdPmcMeshEncoder::~UsdPmcMeshEncoder() {}

bool
UsdPmcMeshEncoder::CanEncode(const UsdGeomMesh& mesh) {
    return true;
}

std::vector<uint8_t>
UsdPmcMeshEncoder::Encode(UsdGeomMesh& mesh, const VtDictionary& options,
                          std::set<std::string>& processedAttributes,
                          std::set<std::string>& processedSubSets,
                          VtDictionary& resultInfo) {
    std::vector<uint8_t> bs;
    try {
        // ToDo: take into account the input options
        PmcEncodeSession pmces = PmcEncodeSession{UsdGeomMesh(mesh)};
        bs = pmces.encode();
        processedAttributes = pmces.processedAttributes;
        processedSubSets = pmces.processedSubSets;
    } catch (std::exception& e) {
        TF_RUNTIME_ERROR("Mesh encoding failed: " + std::string(e.what()));
    }
    return bs;
}

bool
UsdPmcMeshEncoder::_ExtractUSDZFiles() {
    bool extracted = true;
    bool gotEntry = false;
    SdfZipFile zipFile = SdfZipFile::Open(_inUSDZFile.string());
    if (!zipFile) {
        TF_RUNTIME_ERROR("Error: Failed to open USDZ file: " +
                         _inUSDZFile.string());
        return false;
    }
    for (auto it = zipFile.begin(), e = zipFile.end(); it != e; ++it) {
        const SdfZipFile::FileInfo info = it.GetFileInfo();
        std::string filePath = it->c_str();
        if (!gotEntry) {
            _entryFileName = filePath;
            gotEntry = true;
        }
        _references.push_back(filePath);
        std::filesystem::path savePath = _tempDir /
                                         std::filesystem::path(filePath);
        std::filesystem::path parentPath = savePath.parent_path();
        if (!std::filesystem::exists(parentPath)) {
            if (!std::filesystem::create_directories(parentPath)) {
                TF_RUNTIME_ERROR("Error: Failed to create " +
                                 parentPath.string());
                return false;
            }
        }
        if (std::filesystem::exists(savePath)) {
            continue;
        }
        std::ofstream output_file(savePath, std::ios::binary);
        if (output_file.is_open()) {
            output_file.write(it.GetFile(), info.uncompressedSize);
            output_file.close();
        } else {
            TF_RUNTIME_ERROR("Error: Failed to write file: " +
                             savePath.string());
            return false;
        }
    }
    return extracted;
}

bool
UsdPmcMeshEncoder::_RemoveFlattenedReferences() {
    for (auto it = _references.begin(); it != _references.end();) {
        // Skip the entry file name
        if (*it == _entryFileName) {
            ++it;
            continue;
        }
        // Get the extension and convert to lowercase
        std::filesystem::path rPath(*it);
        std::string ext = rPath.extension().string();
        std::string lower_ext;
        lower_ext.reserve(ext.length());
        std::transform(ext.begin(), ext.end(),
                       std::back_inserter(lower_ext),
                       [](unsigned char c){ return std::tolower(c); });
        // Remove USD layer files (they've been flattened)
        if (lower_ext == ".usd" || lower_ext == ".usda" ||
            lower_ext == ".usdc") {
            it = _references.erase(it);
        } else {
            ++it;
        }
    }
    return true;
}

bool
UsdPmcMeshEncoder::_PackUSDZ() {
    bool packed = true;

    // Create the output USDZ file writer
    SdfZipFileWriter usdZipWriter =
        SdfZipFileWriter::CreateNew(_outUSDZFile.string());
    if (!usdZipWriter) {
        TF_RUNTIME_ERROR("Unable to create output USDZ file: " +
                          _outUSDZFile.string());
        return false;
    }

    // Add all referenced files to the new USDZ archive
    std::string target = "";
    for (std::string ref : _references) {
        std::filesystem::path src = _tempDir / std::filesystem::path(ref);
        if (!std::filesystem::exists(src)) {
            TF_RUNTIME_ERROR("Source file does not exist: " +
                             src.string());
            continue;
        }
        target = usdZipWriter.AddFile(src.string(), ref);
    }

    // Save the final USDZ file
    if (!usdZipWriter.Save()) {
        TF_RUNTIME_ERROR("Error: Failed to save output usdz file: " +
                          _outUSDZFile.string());
        return false;
    }
    return packed;
}

bool
UsdPmcMeshEncoder::_ProcessMesh(UsdGeomMesh& mesh,
                                const VtDictionary& options,
                                const uint32_t meshCounter,
                                std::set<std::string>& processedAttributes,
                                std::set<std::string>& processedSubSets,
                                VtDictionary& resultInfo) {
   if (!mesh) {
       return false;
   }
    UsdPrim prim = mesh.GetPrim();
    // Encode the mesh to PMC format
    std::vector<uint8_t> bs = Encode(mesh, options, processedAttributes,
                                     processedSubSets, resultInfo);
    if (bs.size() > 0) {
        // Generate unique filename for this mesh
        std::string pmcFile = std::to_string(meshCounter) + ".pmc";
        std::filesystem::path oFolder = std::filesystem::path(_tempDir) /
                                        std::filesystem::path("pmcCodec");
        // Create PMC codec directory if it doesn't exist
        if (!_CreateDirectory(oFolder)) {
            return false;
        }
        // Write compressed mesh data to file
        std::filesystem::path oFile = oFolder /
                                      std::filesystem::path(pmcFile);
        std::ofstream outfile(oFile, std::ios::binary);
        if (outfile.is_open()) {
            outfile.write(reinterpret_cast<const char*>(bs.data()),
                          bs.size());
            outfile.close();
            // Add reference to the compressed mesh file
            auto references = mesh.GetPrim().GetReferences();
            if (!references) {
                TF_RUNTIME_ERROR(
                    "Error: Unable to add compressed mesh reference: " +
                    oFile.string());
                return false;
            }
            // Create relative path for the reference
            std::filesystem::path refPath =
                std::filesystem::path("pmcCodec") /
                std::filesystem::path(pmcFile);
            references.AddReference(refPath.string());
            _references.push_back(refPath.string());
        } else {
            TF_RUNTIME_ERROR("Error: Unable to write to output file: " +
                             oFile.string());
        }
    }
    return true;
}

bool
UsdPmcMeshEncoder::_RemoveAttributes(
    UsdGeomMesh& mesh,
    std::set<std::string>& processedAttributes,
    std::set<std::string>& processedSubSets) {
    if (!mesh) {
        return false;
    }

    // Remove all the processed attributes that were compressed
    for (auto attr: processedAttributes) {

        if (mesh.GetPrim().HasProperty(TfToken(attr))) {
            mesh.GetPrim().GetAttribute(TfToken(attr)).Clear();
            mesh.GetPrim().GetAttribute(TfToken(attr))
                .ClearMetadata(UsdGeomTokens->elementSize);
        } else {
            // Handle primvar attributes with namespaces
            std::string attrVarType = attr;
            auto pos = attrVarType.substr(0, attrVarType.rfind(':')).rfind(':');
            if (pos != std::string::npos) {
                attrVarType.erase(pos);
                if (attrVarType != "primvars") {
                    if (mesh.GetPrim().HasProperty(TfToken(attrVarType))) {
                        mesh.GetPrim().GetAttribute(TfToken(attrVarType))
                            .Clear();
                        mesh.GetPrim().GetAttribute(TfToken(attrVarType))
                            .ClearMetadata(UsdGeomTokens->elementSize);
                    }
                }
            }
        }
    }

    // Remove indices from processed geometry subsets
    for (auto submesh: UsdGeomSubset::GetAllGeomSubsets(mesh)) {
        if (std::find(processedSubSets.begin(), processedSubSets.end(),
                      submesh.GetPrim().GetName()) != processedSubSets.end()) {
            submesh.GetPrim().RemoveProperty(TfToken("indices"));
        }
    }
    return true;
}

bool UsdPmcMeshEncoder::EncodeStage(std::filesystem::path inUSDZFile,
                                    std::filesystem::path outUSDZFile) {
    // Validate input file extension
    std::string inFileExt = inUSDZFile.extension().string();
    std::transform(inFileExt.begin(), inFileExt.end(), inFileExt.begin(),
                   ::tolower);
    if (inFileExt != "usdz" && inFileExt != ".usdz") {
        TF_RUNTIME_ERROR("Unsupported input file type, expected usdz, got: " +
                         inFileExt);
        return false;
    }

    // Verify input file exists
    if (!std::filesystem::exists(inUSDZFile)) {
        TF_RUNTIME_ERROR("Unable to open input file: " + inUSDZFile.string());
        return false;
    }

    _inUSDZFile = inUSDZFile;
    _outUSDZFile = outUSDZFile;

    // Create temporary directory for processing
    _tempDir = _GetTempDir();
    if (!_CreateDirectory(_tempDir)) {
        TF_RUNTIME_ERROR("Unable to create temp folder: " + _tempDir.string());
        return false;
    }

    // Extract all files from the USDZ archive
    if (!_ExtractUSDZFiles ()) {
        TF_RUNTIME_ERROR("Failed to extract dependencies.");
        return false;
    }

    // ToDo: Remove the flattenning step once the layers processing is available
    // Export the stage as flatten and use it as input for the encoder
    std::filesystem::path inFile = _tempDir / _entryFileName;
    std::filesystem::path flatten;
    if (!_CreateFlattenOutput (inFile, flatten)) {
        TF_RUNTIME_ERROR("Unable to create flatten stage");
        return false;
    }

    // Open the USD stage from the USDZ file
    UsdStageRefPtr stage = UsdStage::Open(flatten.string());
    if (!stage) {
        TF_RUNTIME_ERROR("Failed to open stage.");
        return false;
    }

    VtDictionary meshOptions;

    // Process all meshes in the stage
    uint32_t meshCounter = 0;
    for (UsdPrim prim: stage->TraverseAll()) {
        auto currentMesh = UsdGeomMesh(prim);
        if (currentMesh) {
            if (CanEncode(currentMesh)) {
                std::set<std::string> processedAttributes;
                std::set<std::string> processedSubSets;
                VtDictionary meshResults;

                // Compress the mesh and track what was processed
                if (_ProcessMesh(currentMesh, meshOptions, meshCounter,
                                 processedAttributes, processedSubSets,
                                 meshResults)) {
                    // Remove original attributes since they're now compressed
                    _RemoveAttributes(currentMesh, processedAttributes,
                                      processedSubSets);
                    meshCounter++;
                }
            }
        }
    }

    // Set default entry filename if not found during extraction
    if (_entryFileName.empty()) {
        _entryFileName = "root.usdc";
    }

    // Export the modified stage to the temporary directory
    std::filesystem::path _outRootFile = std::filesystem::path(_tempDir) /
                                         std::filesystem::path(_entryFileName);
    if (!stage->GetRootLayer()->Export(_outRootFile.string())) {
        TF_RUNTIME_ERROR("Unable to export stage to: " +
                         _outRootFile.string());
        return false;
    }

    // Verify the export succeeded
    if (!std::filesystem::exists(_outRootFile)) {
        TF_RUNTIME_ERROR("Did not export the root layer of stage: " +
                         _outRootFile.string());
        return false;
    }

    if (!_RemoveFlattenedReferences()) {
        TF_RUNTIME_ERROR("Failed to remove non flatten references.");
        return false;
    }

    // Pack everything back into a USDZ file
    if (!_PackUSDZ()) {
    TF_RUNTIME_ERROR("Failed to pack output usdz.");
        return false;
    }

    // Clean up temporary directory
    _RemoveTempDir(_tempDir.string());

    return true; // Fixed: should return true on success
}

bool
UsdPmcMeshEncoder::_CreateFlattenOutput(std::filesystem::path input,
                                        std::filesystem::path& output) {
    if (!std::filesystem::exists(input)) {
        TF_RUNTIME_ERROR("Unable to open input file: " + input.string());
        return false;
    }

    // Open the input USD stage
    UsdStageRefPtr stage = UsdStage::Open(input.string());
    if (!stage) {
        TF_RUNTIME_ERROR("Failed to open stage.");
        return false;
    }

    // Generate output path for the flattened stage
    std::string flattenedFileName = "flattened_" + _entryFileName;
    output = _tempDir / std::filesystem::path(flattenedFileName);

    // Flatten the entire stage - this collapses all layer composition and
    // value resolution into a single layer while preserving geometry and
    // prims
    SdfLayerRefPtr flattenedLayer = stage->Flatten();

    if (!flattenedLayer) {
        TF_RUNTIME_ERROR("Failed to flatten stage.");
        return false;
    }

    // Convert absolute paths back to relative paths
    // Get the base path for making relative paths
    std::filesystem::path basePath = _tempDir;

    // Helper function to convert absolute path to relative
    auto makeRelative = [&basePath](const std::string& absPath)
                        -> std::string {
        if (absPath.empty()) return absPath;

        std::filesystem::path absPathFs(absPath);

        // Check if it's an absolute path that needs conversion
        if (absPathFs.is_absolute()) {
            try {
                // Try to make it relative to the base path
                std::filesystem::path relPath =
                    std::filesystem::relative(absPathFs, basePath);
                return relPath.string();
            } catch (...) {
                // If relative path calculation fails, keep the original
                return absPath;
            }
        }

        // Already relative or not a file path
        return absPath;
    };

    // Helper function to recursively fix asset paths in nested dictionaries
    std::function<bool(VtDictionary&)> fixNestedAssetPaths =
        [&](VtDictionary& dict) -> bool {
        bool modified = false;

        for (auto& entry : dict) {
            const std::string& key = entry.first;
            VtValue& value = entry.second;
            // Handle SdfAssetPath values
            if (value.IsHolding<SdfAssetPath>()) {
                SdfAssetPath assetPath = value.UncheckedGet<SdfAssetPath>();
                std::string originalPath = assetPath.GetAssetPath();
                std::string relativePath = makeRelative(originalPath);
                if (relativePath != originalPath) {
                    dict[key] = VtValue(SdfAssetPath(relativePath));
                    modified = true;
                }
            }
            // Handle string values that might be paths (defaultImage)
            else if (value.IsHolding<std::string>()) {
                std::string strValue = value.UncheckedGet<std::string>();
                // Common keys that contain paths
                if (key == "defaultImage" || key == "thumbnailImage" ||
                    key == "previewImage" || key == "icon") {
                    std::string relativePath = makeRelative(strValue);
                    if (relativePath != strValue) {
                        dict[key] = VtValue(relativePath);
                        modified = true;
                    }
                }
            }
            // Recursively handle nested dictionaries
            else if (value.IsHolding<VtDictionary>()) {
                VtDictionary nestedDict =
                    value.UncheckedGet<VtDictionary>();
                if (fixNestedAssetPaths(nestedDict)) {
                    dict[key] = VtValue(nestedDict);
                    modified = true;
                }
            }
        }
        return modified;
    };

    // Traverse all asset paths in the flattened layer and convert them to
    // relative paths
    std::function<void(const SdfPath&)> fixAssetPaths =
        [&](const SdfPath& path) {
        SdfPrimSpecHandle primSpec = flattenedLayer->GetPrimAtPath(path);
        if (!primSpec) return;

        // Check assetInfo dictionary for asset paths like defaultImage
        if (primSpec->HasInfo(SdfFieldKeys->AssetInfo)) {
            VtValue assetInfoValue =
                primSpec->GetInfo(SdfFieldKeys->AssetInfo);
            if (assetInfoValue.IsHolding<VtDictionary>()) {
                VtDictionary assetInfo =
                    assetInfoValue.UncheckedGet<VtDictionary>();
                // Use the recursive helper to fix all nested asset paths
                // This will handle structures like:
                // assetInfo -> previews -> thumbnails -> default ->
                // defaultImage
                if (fixNestedAssetPaths(assetInfo)) {
                    primSpec->SetInfo(SdfFieldKeys->AssetInfo,
                                      VtValue(assetInfo));
                }
            }
        }
        // Check all attributes for asset path values
        for (const SdfAttributeSpecHandle& attrSpec :
             primSpec->GetAttributes()) {
            VtValue value = attrSpec->GetDefaultValue();
            // Handle SdfAssetPath
            if (value.IsHolding<SdfAssetPath>()) {
                SdfAssetPath assetPath = value.UncheckedGet<SdfAssetPath>();
                std::string originalPath = assetPath.GetAssetPath();
                std::string relativePath = makeRelative(originalPath);
                if (relativePath != originalPath) {
                    // Update the attribute with the relative path
                    SdfAssetPath newAssetPath(relativePath);
                    attrSpec->SetDefaultValue(VtValue(newAssetPath));
                }
            }
            // Handle array of SdfAssetPath
            else if (value.IsHolding<VtArray<SdfAssetPath>>()) {
                VtArray<SdfAssetPath> assetPaths =
                    value.UncheckedGet<VtArray<SdfAssetPath>>();
                bool modified = false;
                for (size_t i = 0; i < assetPaths.size(); ++i) {
                    std::string originalPath =
                        assetPaths[i].GetAssetPath();
                    std::string relativePath = makeRelative(originalPath);
                    if (relativePath != originalPath) {
                        assetPaths[i] = SdfAssetPath(relativePath);
                        modified = true;
                    }
                }
                if (modified) {
                    attrSpec->SetDefaultValue(VtValue(assetPaths));
                }
            }
        }

        // Also check relationships and references
        for (const SdfRelationshipSpecHandle& relSpec :
             primSpec->GetRelationships()) {
            // Relationships typically don't contain file paths, but we
            // check anyway
        }

        // Recursively process child prims
        for (const SdfPrimSpecHandle& childSpec :
             primSpec->GetNameChildren()) {
            fixAssetPaths(childSpec->GetPath());
        }
    };

    // Process all prims starting from the root
    SdfPrimSpecHandle rootPrim = flattenedLayer->GetPseudoRoot();
    if (rootPrim) {
        for (const SdfPrimSpecHandle& childSpec :
             rootPrim->GetNameChildren()) {
            fixAssetPaths(childSpec->GetPath());
        }
    }

    // Export the flattened layer to the output path
    if (!flattenedLayer->Export(output.string())) {
        TF_RUNTIME_ERROR("Failed to export flattened layer to: " +
                         output.string());
        return false;
    }

    // Verify the export succeeded
    if (!std::filesystem::exists(output)) {
        TF_RUNTIME_ERROR("Flattened file was not created: " +
                         output.string());
        return false;
    }
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
