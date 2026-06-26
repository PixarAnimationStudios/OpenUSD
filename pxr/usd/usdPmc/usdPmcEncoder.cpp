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

PXR_NAMESPACE_OPEN_SCOPE

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
                          std::set<std::string>* processedAttributes,
                          std::set<std::string>* processedSubSets,
                          VtDictionary* resultInfo) {
    std::vector<uint8_t> bs;
    try {
        PmcEncodeSession pmces = PmcEncodeSession{UsdGeomMesh(mesh), options};
        bs = pmces.encode();
        // resultInfo is currently unused but could be populated in the future
        if (processedAttributes)
            *processedAttributes = std::move(pmces.GetProcessedAttributeNames());

        if (processedSubSets)
            *processedSubSets = std::move(pmces.GetProcessedGeomSubsetNames());
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
                                std::set<std::string>* processedAttributes,
                                std::set<std::string>* processedSubSets,
                                VtDictionary* resultInfo) {
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
    std::set<std::string>* processedAttributes,
    std::set<std::string>* processedSubSets) {
    if (!mesh) {
        return false;
    }

    // Remove all the processed attributes that were compressed
    if (processedAttributes) {
        for (auto attr: *processedAttributes) {

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
    }

    // Remove indices from processed geometry subsets
    if (processedSubSets) {
        for (auto submesh: UsdGeomSubset::GetAllGeomSubsets(mesh)) {
            if (std::find(processedSubSets->begin(), processedSubSets->end(),
                          submesh.GetPrim().GetName()) != processedSubSets->end()) {
                submesh.GetPrim().RemoveProperty(TfToken("indices"));
            }
        }
    }
    return true;
}

bool UsdPmcMeshEncoder::EncodeStage(std::filesystem::path inFile,
                                    std::filesystem::path outFile,
                                    const VtDictionary& options) {
    // Verify input file exists
    if (!std::filesystem::exists(inFile)) {
        TF_RUNTIME_ERROR("Unable to open input file: " + inFile.string());
        return false;
    }

    _inUSDZFile = inFile;
    _outUSDZFile = outFile;

    // Create temporary directory for processing
    const std::string tmpDirStr =
        ArchMakeTmpSubdir(ArchGetTmpDir(), "usdPmcStageEncoder");
    if (tmpDirStr.empty()) {
        TF_RUNTIME_ERROR("Unable to create temp folder");
        return false;
    }
    _tempDir = tmpDirStr;

    // Determine input format
    std::string inExt = inFile.extension().string();
    std::transform(inExt.begin(), inExt.end(), inExt.begin(), ::tolower);
    const bool isUsdzInput = (inExt == ".usdz");

    // For USDZ input, extract the archive; otherwise use the file directly
    std::filesystem::path stageSource;
    if (isUsdzInput) {
        if (!_ExtractUSDZFiles()) {
            TF_RUNTIME_ERROR("Failed to extract dependencies.");
            return false;
        }
        stageSource = _tempDir / _entryFileName;
    } else {
        _entryFileName = inFile.filename().string();
        stageSource = inFile;
    }

    // Flatten the stage
    std::filesystem::path flattenedPath;
    if (!_CreateFlattenOutput(stageSource, &flattenedPath)) {
        TF_RUNTIME_ERROR("Unable to create flatten stage");
        return false;
    }

    // Open the flattened USD stage
    UsdStageRefPtr stage = UsdStage::Open(flattenedPath.string());
    if (!stage) {
        TF_RUNTIME_ERROR("Failed to open stage.");
        return false;
    }

    // Process all meshes in the stage
    uint32_t meshCounter = 0;
    for (UsdPrim prim : stage->TraverseAll()) {
        auto currentMesh = UsdGeomMesh(prim);
        if (currentMesh && CanEncode(currentMesh)) {
            std::set<std::string> processedAttributes;
            std::set<std::string> processedSubSets;
            VtDictionary meshResults;

            if (_ProcessMesh(currentMesh, options, meshCounter,
                             &processedAttributes, &processedSubSets,
                             &meshResults)) {
                _RemoveAttributes(currentMesh, &processedAttributes,
                                  &processedSubSets);
                meshCounter++;
            }
        }
    }

    if (_entryFileName.empty()) {
        _entryFileName = "root.usdc";
    }

    // Determine output format
    std::string outExt = outFile.extension().string();
    std::transform(outExt.begin(), outExt.end(), outExt.begin(), ::tolower);
    const bool isUsdzOutput = (outExt == ".usdz");

    if (isUsdzOutput) {
        // Export the modified stage to the temporary directory for packing
        std::filesystem::path outRootFile = _tempDir / _entryFileName;
        if (!stage->GetRootLayer()->Export(outRootFile.string())) {
            TF_RUNTIME_ERROR("Unable to export stage to: " +
                             outRootFile.string());
            return false;
        }
        if (!std::filesystem::exists(outRootFile)) {
            TF_RUNTIME_ERROR("Did not export the root layer of stage: " +
                             outRootFile.string());
            return false;
        }

        if (isUsdzInput) {
            // Remove USD layers that were flattened into the entry
            if (!_RemoveFlattenedReferences()) {
                TF_RUNTIME_ERROR("Failed to remove non flatten references.");
                return false;
            }
        } else {
            // For non-USDZ input, build the references list:
            // PMC files were appended by _ProcessMesh; prepend the entry layer
            _references.push_front(_entryFileName);
        }

        if (!_PackUSDZ()) {
            TF_RUNTIME_ERROR("Failed to pack output usdz.");
            return false;
        }
    } else {
        // Non-USDZ output: export stage directly to the output path
        std::filesystem::path outDir = outFile.parent_path();
        if (!outDir.empty() && !_CreateDirectory(outDir)) {
            TF_RUNTIME_ERROR("Unable to create output directory: " +
                             outDir.string());
            return false;
        }
        if (!stage->GetRootLayer()->Export(outFile.string())) {
            TF_RUNTIME_ERROR("Unable to export stage to: " + outFile.string());
            return false;
        }
        // Copy PMC files as siblings of the output file
        if (!_WriteNonUsdzOutput()) {
            TF_RUNTIME_ERROR("Failed to write PMC files.");
            return false;
        }
    }

    // Clean up temporary directory
    _RemoveTempDir(_tempDir.string());

    return true;
}

bool
UsdPmcMeshEncoder::_WriteNonUsdzOutput() {
    std::filesystem::path outDir = _outUSDZFile.parent_path();
    if (outDir.empty()) {
        outDir = std::filesystem::current_path();
    }

    // Copy PMC files from temp dir into a sibling pmcCodec directory
    std::filesystem::path pmcSrcDir = _tempDir / "pmcCodec";
    if (std::filesystem::exists(pmcSrcDir)) {
        std::filesystem::path pmcOutDir = outDir / "pmcCodec";
        if (!_CreateDirectory(pmcOutDir)) {
            return false;
        }
        for (const auto& entry :
             std::filesystem::directory_iterator(pmcSrcDir)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            std::filesystem::path dest =
                pmcOutDir / entry.path().filename();
            std::error_code ec;
            std::filesystem::copy_file(
                entry.path(), dest,
                std::filesystem::copy_options::overwrite_existing, ec);
            if (ec) {
                TF_RUNTIME_ERROR(
                    "Failed to copy PMC file: " +
                    entry.path().string() + " -> " + dest.string() +
                    ": " + ec.message());
                return false;
            }
        }
    }
    return true;
}

bool
UsdPmcMeshEncoder::_CreateFlattenOutput(std::filesystem::path input,
                                        std::filesystem::path* output) {
    if (!output) {
        TF_RUNTIME_ERROR("output cannot be null");
        return false;
    }
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
    *output = _tempDir / std::filesystem::path(flattenedFileName);

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
    if (!flattenedLayer->Export(output->string())) {
        TF_RUNTIME_ERROR("Failed to export flattened layer to: " +
                         output->string());
        return false;
    }

    // Verify the export succeeded
    if (!std::filesystem::exists(*output)) {
        TF_RUNTIME_ERROR("Flattened file was not created: " +
                         output->string());
        return false;
    }
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
