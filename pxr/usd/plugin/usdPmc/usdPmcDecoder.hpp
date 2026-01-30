//
// Copyright 2025 Apple
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

/// \file usdPmc/usdPmcDecoder.hpp

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

/// \class UsdPmcMeshDecoder
///
/// Decode PMC compressed mesh data into UsdGeomMesh.
///
/// This class provides functionality to decode AOMedia PMC compressed
/// mesh data and convert it into USD geometry. It handles the conversion
/// of compressed geometry, attributes, and metadata from the PMC format
/// to USD's native representation.
class UsdPmcMeshDecoder {
public:
    UsdPmcMeshDecoder();
    ~UsdPmcMeshDecoder();

    /// Check if buffer contains valid PMC data that can be decoded.
    /// \param buffer Pointer to the PMC data buffer
    /// \param length Size of the buffer in bytes
    /// \return true if the buffer contains valid PMC data, false otherwise
    bool CanDecode(const char* buffer, size_t length);
    
    /// Decode PMC buffer into UsdGeomMesh.
    /// \param buffer Pointer to the PMC data buffer
    /// \param length Size of the buffer in bytes
    /// \param decodedMesh The UsdGeomMesh to populate with decoded data
    /// \return true if decoding was successful, false otherwise
    bool Decode(const char* buffer, size_t length, UsdGeomMesh& decodedMesh);

protected:
    /// Inspect bitstream header and validate format
    bool _InspectBitstream(const char* buffer, size_t length);
    
    /// Perform actual decoding of PMC bitstream
    bool _DecodeBitstream(const char* buffer, size_t length,
                          UsdGeomMesh& decodedMesh);

    /// Generate attribute name from PMC attribute info
    void _InferNameFromInfo(std::string& attrName,
                            const pmc::AttributeMeshpartInfo& info);
    
    /// Convert string type name to USD SdfValueTypeName
    bool _GetTypeNameFromString(const std::string_view strTypeName,
                                SdfValueTypeName& typeName);
    
    /// Extract type name from JSON user data
    bool _ExtractTypeNameFromUserData(SdfValueTypeName& attrTypeName,
                                      const VtDictionary& userData);
    
    /// Infer USD type name from attribute name
    void _InferTypeNameFromName(SdfValueTypeName& attrTypeName,
                                const std::string_view attrName);
    
    /// Extract custom attribute flag from user data
    void _ExtractCustomFromUserData(bool& attrCustom,
                                    const VtDictionary& userData);
    
    /// Parse JSON user data into VtDictionary
    bool _GetUserDataInfo(VtDictionary& userData,
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
ToPmcBuffer(T& usdBuffer, int cpv, pmc::DataType dt) {
    pmc::ArrayBuffer buf;
    buf.data = (uint8_t*)usdBuffer.data();
    buf.offset = 0;
    buf.stride = sizeof(int) * cpv;
    buf.vectorCount = usdBuffer.size();
    buf.componentsPerVector = cpv;
    buf.dataType = dt;
    return buf;
}

/// Convert flat integer buffer to multidimensional USD array with scaling.
/// \param buffer The flat integer buffer to convert
/// \param cpv Components per vector
/// \param scalingRange Scaling range for value conversion
/// \return Converted multidimensional USD array
template<typename T>
inline VtArray<T> flattenToMultidimUsdArray(const VtIntArray& buffer, int cpv,
                                             std::pair<int,int> scalingRange) {
    VtArray<T> outBuffer;
    if (cpv <= 0) {
        return outBuffer;
    }
    outBuffer.resize(buffer.size() / cpv);
    if (scalingRange.second == 0 ||
        scalingRange.first == scalingRange.second) {
        for (int i = 0; i < buffer.size() / cpv; ++i) {
            T& val = outBuffer[i];
            for (int j = 0; j < cpv; j++) {
                val[j] = buffer[i*cpv + j];
            }
        }
    } else {
        float scaling = (float)scalingRange.first /
                        (float)scalingRange.second;
        for (int i = 0; i < buffer.size() / cpv; ++i) {
            T& val = outBuffer[i];
            for (int j = 0; j < cpv; ++j) {
                val[j] = float(buffer[i*cpv + j]) * scaling;
            }
        }
    }
    return outBuffer;
}

/// Write PMC scalar attribute values to USD attribute with scaling.
/// \param pmcAttr The PMC attribute containing the data
/// \param usdAttr The USD attribute to write to
/// \param scalingRange Scaling range for value conversion
/// \return true if successful, false otherwise
template<typename T, typename U>
inline bool _WriteAttributeScalarValuesToUsdAttribute(
    const pmc::AttributeMeshpart& pmcAttr, U& usdAttr,
    std::pair<int,int> scalingRange) {
    try {
        VtArray<T> tmpBuffer;
        const int* values = (int*)(pmcAttr.buffers.values.data);
        const size_t elementsCount = pmcAttr.info.vectorCount *
                                      pmcAttr.info.componentsPerVector;
        if (scalingRange.second == 0 ||
            scalingRange.first == scalingRange.second) {
            for (auto idx = 0; idx < elementsCount; ++idx) {
                tmpBuffer.push_back((T)(values[idx]));
            }
        } else {
            float scaling = (float)scalingRange.first /
                            (float)scalingRange.second;
            for (auto idx = 0; idx < elementsCount; ++idx) {
                tmpBuffer.push_back(float(values[idx]) * scaling);
            }
        }
        usdAttr.Set(tmpBuffer);
        return true;
    }
    catch(...) {
        TF_RUNTIME_ERROR("Unable to assign value to attribute");
        return false;
    }
}

/// Write PMC vector attribute values to USD attribute with scaling.
/// \param pmcAttr The PMC attribute containing the data
/// \param usdAttr The USD attribute to write to
/// \param scalingRange Scaling range for value conversion
/// \return true if successful, false otherwise
template<typename T, typename U>
inline bool _WriteAttributeVectorValuesToUsdAttribute(
    const pmc::AttributeMeshpart& pmcAttr, U& usdAttr,
    std::pair<int,int> scalingRange) {
    VtArray<T> tmpBuffer;
    try {
        int* values = (int*)(pmcAttr.buffers.values.data);
        size_t elementsCount = pmcAttr.info.vectorCount;
        size_t elementDimension = pmcAttr.info.componentsPerVector;

        if (scalingRange.second == 0 ||
            scalingRange.first == scalingRange.second) {
            for (auto idx = 0; idx < elementsCount * elementDimension;
                 idx += elementDimension) {
                T element;
                for (auto elementIdx = 0; elementIdx < elementDimension;
                     ++elementIdx) {
                    element[elementIdx] = values[idx + elementIdx];
                }
                tmpBuffer.push_back(element);
            }
        } else {
            float scaling = (float)scalingRange.first /
                            (float)scalingRange.second;
            for (auto idx = 0; idx < elementsCount * elementDimension;
                 idx += elementDimension) {
                T element;
                for (auto elementIdx = 0; elementIdx < elementDimension;
                     ++elementIdx) {
                    element[elementIdx] = float(values[idx + elementIdx]) *
                                          scaling;
                }
                tmpBuffer.push_back(element);
            }
        }
        usdAttr.Set(tmpBuffer);
        return true;
    } catch (...) {
        TF_RUNTIME_ERROR("Unable to assign values to attribute");
        return false;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_PMC_DECODER_H
