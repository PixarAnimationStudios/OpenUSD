//
// Copyright 2025 Apple
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

/// \file usdPmc/usdPmcDecoder.hpp

#ifndef USD_PMC_DECODER_H
#define USD_PMC_DECODER_H

#include "api.h"

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/tokens.h"

// PMC specific includes
#include <pmc/pmDecoder.hpp>
#include <pmc/pmTypesAndConstants.hpp>
#include <string_view>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdPmcMeshDecoder
///
/// Decode PMC compressed mesh data into UsdGeomMesh.
///
/// This class provides functionality to decode AOMedia Polygonal Mesh Coding
/// compressed mesh data and convert it into USD geometry. It handles the
/// conversion of compressed geometry, attributes, and metadata from the
/// PMC format to USD's native representation.
class UsdPmcMeshDecoder {
public:
    USDPMC_API UsdPmcMeshDecoder();
    USDPMC_API ~UsdPmcMeshDecoder();

    /// Check if buffer contains valid PMC data that can be decoded.
    /// \param buffer Pointer to the PMC data buffer
    /// \param length Size of the buffer in bytes
    /// \return true if the buffer contains valid PMC data, false otherwise

    USDPMC_API
    bool CanDecode(const char* buffer, size_t length);

    /// Decode PMC buffer into UsdGeomMesh.
    /// \param buffer Pointer to the PMC data buffer
    /// \param length Size of the buffer in bytes
    /// \param decodedMesh Pointer to the UsdGeomMesh to populate with decoded data
    /// \return true if decoding was successful, false otherwise

    USDPMC_API
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

/// Convert USD buffer to PMC ArrayBuffer format.
/// \param usdBuffer The USD buffer to convert
/// \param cpv Components per vector
/// \param dt PMC data type
/// \return PMC ArrayBuffer structure
template<typename T>
inline pmc::ArrayBuffer
UsdPmc_ToPmcBuffer(T& usdBuffer, int cpv, pmc::DataType dt) {
    pmc::ArrayBuffer buf;
    buf.data = (uint8_t*)usdBuffer.data();
    buf.offset = 0;
    buf.stride = sizeof(int) * cpv;
    buf.vectorCount = usdBuffer.size() / cpv;
    buf.componentsPerVector = cpv;
    buf.dataType = dt;
    return buf;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_PMC_DECODER_H
