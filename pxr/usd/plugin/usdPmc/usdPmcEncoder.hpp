//
// Copyright 2025 Apple
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

/// \file usdPmc/usdPmcEncoder.hpp

#ifndef USD_PMC_ENCODER_H
#define USD_PMC_ENCODER_H

#include "api.h"

#include "pxr/pxr.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/usd/usdGeom/mesh.h"

#include <filesystem>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdPmcMeshEncoder
///
/// Encode UsdGeomMesh data into PMC compressed format.
///
/// This class provides functionality to encode USD geometry into PMC
/// compressed mesh data. It handles the conversion of USD
/// geometry, attributes, and metadata into the PMC format for efficient
/// storage and transmission.
class UsdPmcMeshEncoder {
public:
    USDPMC_API UsdPmcMeshEncoder();
    USDPMC_API ~UsdPmcMeshEncoder();
    /// Check if the given mesh can be encoded to PMC format.
    /// \param mesh The UsdGeomMesh to check for encoding compatibility
    /// \return true if the mesh can be encoded, false otherwise
    USDPMC_API bool CanEncode(const UsdGeomMesh& mesh);
    /// Encode a UsdGeomMesh into PMC compressed format.
    /// \param mesh The UsdGeomMesh to encode
    /// \param options Encoding options and parameters
    /// \param processedAttributes Pointer to set to store names of processed attributes
    /// \param processedSubSets Pointer to set to store names of processed subsets
    /// \param resultInfo Pointer to dictionary to store encoding result information
    /// \return Encoded PMC data as a byte vector
    USDPMC_API std::vector<uint8_t> Encode(UsdGeomMesh& mesh,
                                           const VtDictionary& options,
                                           std::set<std::string>* processedAttributes,
                                           std::set<std::string>* processedSubSets,
                                           VtDictionary* resultInfo);
    /// Encode an entire USD stage from USDZ file to PMC-compressed USDZ.
    /// \param inUSDZFile Path to input USDZ file
    /// \param outUSDZFile Path to output USDZ file with PMC compression
    /// \return true if encoding was successful, false otherwise
    USDPMC_API bool EncodeStage(std::filesystem::path inUSDZFile,
                                std::filesystem::path outUSDZFile,
                                const VtDictionary& options);
private:
    std::filesystem::path   _inUSDZFile;
    std::filesystem::path   _outUSDZFile;
    std::filesystem::path   _outRootFile;
    std::list<std::string>  _references;
    std::filesystem::path   _tempDir;
    std::string             _entryFileName;
    /// Extract all files from the input USDZ archive to temporary directory
    bool _ExtractUSDZFiles();
    /// Process and compress a single mesh, writing PMC data and updating
    /// references
    bool _ProcessMesh(UsdGeomMesh& mesh, const VtDictionary& options,
                      const uint32_t meshCounter,
                      std::set<std::string>* processedAttributes,
                      std::set<std::string>* processedSubSets,
                      VtDictionary* resultInfo);
    /// Remove processed attributes and subsets from the original mesh after
    /// compression
    bool _RemoveAttributes(UsdGeomMesh& mesh,
                           std::set<std::string>* processedAttributes,
                           std::set<std::string>* processedSubSets);
    /// Pack all processed files back into the output USDZ archive
    bool _PackUSDZ();
    /// Write PMC files as siblings of the output file for non-USDZ output
    bool _WriteNonUsdzOutput();
    /// Remove all the non flatten references from the list
    bool _RemoveFlattenedReferences();
    bool _CreateFlattenOutput (std::filesystem::path input,
                               std::filesystem::path* output);
};
PXR_NAMESPACE_CLOSE_SCOPE
#endif // USD_PMC_ENCODER_H
