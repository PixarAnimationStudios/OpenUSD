//
// Copyright 2026 Apple
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

/// \file usdPmc/usdPmcDecoder.h

#ifndef USD_PMC_DECODER_H
#define USD_PMC_DECODER_H

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/tokens.h"

// PMC specific includes
#include <pmc/pmDecoder.hpp>
#include <pmc/pmTypesAndConstants.hpp>
#include <string_view>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdPmc_MeshDecoder
///
/// Decode PMC compressed mesh data into UsdGeomMesh.
///
/// This class provides functionality to decode AOMedia Polygonal Mesh Coding
/// compressed mesh data and convert it into USD geometry. It handles the
/// conversion of compressed geometry, attributes, and metadata from the
/// PMC format to USD's native representation.
class UsdPmc_MeshDecoder {
public:
    UsdPmc_MeshDecoder();
    ~UsdPmc_MeshDecoder();

    /// Check if buffer contains valid PMC data that can be decoded.
    /// \param buffer Pointer to the PMC data buffer
    /// \param length Size of the buffer in bytes
    /// \return true if the buffer contains valid PMC data, false otherwise

    bool CanDecode(const char* buffer, size_t length);

    /// Decode PMC buffer into UsdGeomMesh.
    /// \param buffer Pointer to the PMC data buffer
    /// \param length Size of the buffer in bytes
    /// \param decodedMesh Pointer to the UsdGeomMesh to populate with decoded data
    /// \return true if decoding was successful, false otherwise

    bool Decode(const char* buffer, size_t length, UsdGeomMesh* decodedMesh);

private:
    /// Inspect bitstream header and validate format
    bool _InspectBitstream(const char* buffer, size_t length);

    /// Perform actual decoding of PMC bitstream
    bool _DecodeBitstream(const char* buffer, size_t length,
                          UsdGeomMesh* decodedMesh);

    /// Generate attribute name from PMC attribute info
    void _InferNameFromInfo(std::string* attrName,
                            const pmc::AttributeMeshpartInfo& info);

    /// Convert string type name to USD SdfValueTypeName
    bool _GetTypeNameFromString(const std::string_view strTypeName,
                                SdfValueTypeName* typeName);

    /// Extract type name from JSON user data
    bool _ExtractTypeNameFromUserData(SdfValueTypeName* attrTypeName,
                                      const VtDictionary& userData);

    /// Infer USD type name from attribute name
    void _InferTypeNameFromName(SdfValueTypeName* attrTypeName,
                                const std::string_view attrName);

    /// Parse JSON user data into VtDictionary
    bool _GetUserDataInfo(VtDictionary* userData,
                          const std::string& jsonUserData);

    pmc::Decoder _dec;
    uint32_t _unnamedAttributeCount;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_PMC_DECODER_H
