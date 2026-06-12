//
// Copyright 2025 Apple
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

/// \file usdPmc/usdPmcDecoder.cpp
///
/// Implementation of USD PMC mesh decoder functionality.
/// This file provides the core decompression logic for converting PMC
/// compressed mesh data back into USD mesh primitives with full fidelity
/// reconstruction
/// of geometry, attributes, primvars, and subsets.
///
/// The decoder supports:
/// - Decompression of PMC bitstreams into USD mesh geometry
/// - Reconstruction of vertex positions, face indices, and face counts
/// - Recovery of mesh attributes including normals, texture coordinates,
///   colors
/// - Restoration of primvars with proper interpolation and indexing
/// - Reconstruction of geometry subsets and creases
/// - Proper scaling and quantization reversal for floating-point data

#include "usdPmcDecoder.hpp"
#include "usdPmcConstantPrivate.hpp"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/usdGeom/subset.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/base/js/json.h"
#include <string_view>

// PMC specific includes
#include <pmc/pmDecoder.hpp>

PXR_NAMESPACE_OPEN_SCOPE

/// This macro generates code to write scalar (single-component) attribute
/// values from PMC decoded data to USD attributes, with optional
/// scaling/quantization reversal.
#define WRITE_ATTRIBUTE_SCALAR(attribute_type, usd_attr_type, usd_attr, \
                                scalingRange) \
    if (cppTypeName == #attribute_type) \
        res = UsdPmc_WriteAttributeScalarValuesToUsdAttribute<attribute_type, \
                                                             usd_attr_type>( \
            attrPart, usd_attr, scalingRange);

/// This macro generates code to write vector (multi-component) attribute
/// values from PMC decoded data to USD attributes, with optional
/// scaling/quantization reversal.
#define WRITE_ATTRIBUTE_VECTOR(attribute_type, usd_attr_type, usd_attr, \
                                scalingRange) \
    if (cppTypeName == #attribute_type) \
        res = UsdPmc_WriteAttributeVectorValuesToUsdAttribute<attribute_type, \
                                                             usd_attr_type>( \
            attrPart, usd_attr, scalingRange);

/// This macro generates a comprehensive set of type-specific attribute writing
/// code that handles all supported USD attribute types (vectors and scalars)
/// with appropriate scaling for floating-point types and no scaling for
/// integer types.
#define WRITE_ATTRIBUTE(attributeType, attribute) \
    auto scale = ScalingRangeFrom(attrPart.info.coordSys); \
    WRITE_ATTRIBUTE_VECTOR(GfVec2i, attributeType, attribute, scale) \
    WRITE_ATTRIBUTE_VECTOR(GfVec3i, attributeType, attribute, scale) \
    WRITE_ATTRIBUTE_VECTOR(GfVec4i, attributeType, attribute, scale) \
    WRITE_ATTRIBUTE_VECTOR(GfVec2h, attributeType, attribute, scale) \
    WRITE_ATTRIBUTE_VECTOR(GfVec3h, attributeType, attribute, scale) \
    WRITE_ATTRIBUTE_VECTOR(GfVec4h, attributeType, attribute, scale) \
    WRITE_ATTRIBUTE_VECTOR(GfVec2f, attributeType, attribute, scale) \
    WRITE_ATTRIBUTE_VECTOR(GfVec3f, attributeType, attribute, scale) \
    WRITE_ATTRIBUTE_VECTOR(GfVec4f, attributeType, attribute, scale) \
    WRITE_ATTRIBUTE_VECTOR(GfVec2d, attributeType, attribute, scale) \
    WRITE_ATTRIBUTE_VECTOR(GfVec3d, attributeType, attribute, scale) \
    WRITE_ATTRIBUTE_VECTOR(GfVec4d, attributeType, attribute, scale) \
    WRITE_ATTRIBUTE_SCALAR(bool, attributeType, attribute, scale) \
    WRITE_ATTRIBUTE_SCALAR(int, attributeType, attribute, scale) \
    WRITE_ATTRIBUTE_SCALAR(uint8_t, attributeType, attribute, scale) \
    WRITE_ATTRIBUTE_SCALAR(unsigned int, attributeType, attribute, scale) \
    WRITE_ATTRIBUTE_SCALAR(unsigned char, attributeType, attribute, scale) \
    WRITE_ATTRIBUTE_SCALAR(GfHalf, attributeType, attribute, scale) \
    WRITE_ATTRIBUTE_SCALAR(float, attributeType, attribute, scale) \
    WRITE_ATTRIBUTE_SCALAR(double, attributeType, attribute, scale)


/// Convert coordinate system scaling
static std::pair<int,int>
ScalingRangeFrom(const std::optional<pmc::AttributeInfo::CoordinateSystem>& cs)
{
    if (!cs)
        return {0,0};
    return { cs->scale.q, cs->scale.p };
    // xxx need to handle offset
}

UsdPmcMeshDecoder::UsdPmcMeshDecoder() : _unnamedAttributeCount(0) {}
UsdPmcMeshDecoder::~UsdPmcMeshDecoder() {}
bool
UsdPmcMeshDecoder::CanDecode(const char* buffer, size_t length) {
    return _InspectBitstream(buffer, length);
}

bool
UsdPmcMeshDecoder::_InspectBitstream(const char* buffer, size_t length) {
    pmc::Decoder::InspectionDelegate inspector;

    // Set up geometry meshpart inspection
    inspector.onInspectGeometryMeshpart =
        [&](const pmc::GeometryMeshpartInfo& info,
            bool& decode) noexcept {
            auto slice = info.meshpartId;
            if (slice < 0) {
                return pmc::Error::GEOMETRY_MESHPART_ID_OUT_OF_RANGE;
            }
            decode = true;
            return pmc::Error::OK;
        };

    // Set up attribute meshpart inspection
    inspector.onInspectAttributeMeshpart =
        [&](const pmc::AttributeMeshpartInfo& info,
            bool& decode) noexcept {
            auto slice = info.meshpartId;
            if (slice < 0) {
                return pmc::Error::GEOMETRY_MESHPART_ID_OUT_OF_RANGE;
            }
            decode = true;
            return pmc::Error::OK;
        };

    // Create PMC byte buffer from input data
    pmc::ByteBuffer bitstream;
    bitstream.capacity = bitstream.size = length;
    bitstream.data = (uint8_t*)(const_cast<char*>(buffer));

    // Perform the inspection
    auto ret = _dec.inspect(bitstream, inspector);
    if (ret != pmc::Error::OK) {
        TF_RUNTIME_ERROR("Failed to inspect PMC bitstream: %d", int(ret));
        return false;
    }

    return true;
}

bool
UsdPmcMeshDecoder::Decode(const char* buffer, size_t length,
                          UsdGeomMesh* decodedMesh) {
    if (!decodedMesh) {
        TF_RUNTIME_ERROR("decodedMesh cannot be null");
        return false;
    }
    if (!_DecodeBitstream(buffer, length, decodedMesh)) {
        TF_RUNTIME_ERROR("Unable to decode PMC bitstream");
        return false;
    }
    return true;
}

bool
UsdPmcMeshDecoder::_DecodeBitstream(const char* buffer, size_t length,
                                    UsdGeomMesh* decodedMesh) {
    if (!decodedMesh) {
        TF_RUNTIME_ERROR("decodedMesh cannot be null");
        return false;
    }
    pmc::Decoder::InspectionDelegate inspectFns;
    VtIntArray sharpnessesDerivedIds;
    int creaseAttrId = -1;
    int cornerAttrId = -1;
    bool decodeCrease = false;
    bool decodeCorner = false;

    inspectFns.onInspectGeometryMeshpart =
        [&](const pmc::GeometryMeshpartInfo& info, bool& decode) noexcept {
            decode = true;
            return pmc::Error::OK;
        };

    inspectFns.onInspectAttributeMeshpart =
        [&](const pmc::AttributeMeshpartInfo& info, bool& decode) noexcept {
            if (info.type == pmc::AttributeType::CREASE
                && info.scope == pmc::AttributeScope::VERTEX
                && info.indicesInterpretation == pmc::IndicesInterpretation::SCOPE_INDEXING) {
                if ( info.name == UsdGeomTokens->cornerIndices ) {
                    // Corner creases
                    cornerAttrId = info.attributeId;
                } else {
                    // Crease lengths and indices, saving Id for future reference
                    creaseAttrId = info.attributeId;
                }
            }
            if (info.type == pmc::AttributeType::SHARPNESS
                && info.scope == pmc::AttributeScope::DERIVED
                && info.indicesInterpretation == pmc::IndicesInterpretation::VALUE_INDEXING) {
                
                // Sharpnesses (corner, crease?), saving derived Id
                sharpnessesDerivedIds.push_back(info.derivedScope.scopedAttributeId);
            }
            decode = true;
            return pmc::Error::OK;
        };

    pmc::ByteBuffer bitstream;
    bitstream.capacity = bitstream.size = length;
    bitstream.data = (uint8_t*)(const_cast<char*>(buffer));

    auto error = _dec.inspect(bitstream, inspectFns);
    if (error != pmc::Error::OK) {
        TF_RUNTIME_ERROR("Failed to inspect PMC bitstream: %d", int(error));
        return false;
    }

    // Check crease data
    if ( std::find(sharpnessesDerivedIds.begin(),
                   sharpnessesDerivedIds.end(),
                   creaseAttrId) != sharpnessesDerivedIds.end()) {
        // There is a combination of compatible Sharpnesses, Lengths and Indices for crease
        decodeCrease = true;
    }

    // Check corner data
    if ( std::find(sharpnessesDerivedIds.begin(),
                   sharpnessesDerivedIds.end(),
                   cornerAttrId) != sharpnessesDerivedIds.end()) {
        // There is a combination of compatible Sharpnesses and Corners
        decodeCorner = true;
    }

    pmc::Decoder::DecodingDelegate decodeFns;
    VtIntArray usdFaceVertexIndices;
    VtIntArray usdFaceVertexCounts;
    VtIntArray usdPoints;
    decodeFns.onStartGeometryMeshpartDecoding =
        [&](const pmc::GeometryMeshpartInfo& info,
            pmc::GeometryMeshpartBuffers& bufs) noexcept {
            try {
                // Points
                usdPoints.resize(info.vertexCount * 3);
                bufs.positions = UsdPmc_ToPmcBuffer(usdPoints, 3,
                                                   pmc::DataType::Int32);
        
                // Face vertex indices
                usdFaceVertexIndices.resize(info.indexCount);
                bufs.indices = UsdPmc_ToPmcBuffer(usdFaceVertexIndices, 1,
                                                 pmc::DataType::Int32);

                // Face counts
                usdFaceVertexCounts.resize(info.faceCount);
                bufs.faceDegrees = UsdPmc_ToPmcBuffer(usdFaceVertexCounts, 1,
                                                     pmc::DataType::Int32);

                return pmc::Error::OK;
            } catch (...) {
                return pmc::Error::STATE_ERROR;
            }
        };

    VtIntArray attrValues;
    VtIntArray attrIndices; 
    decodeFns.onStartAttributeMeshpartDecoding =
        [&](const pmc::AttributeMeshpartInfo& info,
            const pmc::GeometryMeshpartInfo& /*unused*/,
            pmc::AttributeMeshpartBuffers& bufs) noexcept {
            try {
                attrValues.resize(info.vectorCount *
                                  info.componentsPerVector);
                bufs.values = UsdPmc_ToPmcBuffer(attrValues,
                                                info.componentsPerVector,
                                                pmc::DataType::Int32);

                if (info.indexCount > 0) {
                    attrIndices.resize(info.indexCount);
                    bufs.indices = UsdPmc_ToPmcBuffer(attrIndices, 1,
                                                     pmc::DataType::Int32);
                } else {
                    attrIndices.resize(0);
                }
                return pmc::Error::OK;
            } catch (...) {
                TF_RUNTIME_ERROR("Unable to start decoding attribute");
                return pmc::Error::STATE_ERROR;
            }
        };

    decodeFns.onEndGeometryMeshpartDecoding =
        [&](const pmc::GeometryMeshpart& meshPart,
            size_t decodedByteCount) {
            auto usdPointsAttr = decodedMesh->CreatePointsAttr();

            std::pair<int,int> scalingRange {
                meshPart.info.coordSys.q, meshPart.info.coordSys.p
            };

            usdPointsAttr.Set(UsdPmc_FlattenToMultidimUsdArray<GfVec3f>(
                usdPoints, 3, scalingRange));

            auto usdFaceVertexIndicesAttr =
                decodedMesh->CreateFaceVertexIndicesAttr();
            usdFaceVertexIndicesAttr.Set(usdFaceVertexIndices);

            auto usdFaceVertexCountsAttr =
                decodedMesh->CreateFaceVertexCountsAttr();
            usdFaceVertexCountsAttr.Set(usdFaceVertexCounts);

            return pmc::Error::OK;
        };

    decodeFns.onEndAttributeMeshpartDecoding =
        [&](const pmc::AttributeMeshpart& attrPart,
            size_t decodedByteCount) {
            std::string attrName = attrPart.info.name;

            VtDictionary userData;
            if (!_GetUserDataInfo(&userData, attrPart.info.jsonCustomAui)) {
                TF_RUNTIME_ERROR("Error for attribute " + attrName +
                                 ": unable to parse user data");
                return pmc::Error::STATE_ERROR;
            }

            // UsdGeomSubsets (face only)
            if (attrPart.info.type == pmc::AttributeType::FACE_GROUP
                && attrPart.info.scope == pmc::AttributeScope::FACE
                && attrPart.info.indicesInterpretation == pmc::IndicesInterpretation::SCOPE_INDEXING) {
                
                if (userData.find(kUSDJsonSubmeshNamesKey) == userData.end() ||
                    !userData[kUSDJsonSubmeshNamesKey].IsHolding<
                        VtArray<std::string>>()) {
                    TF_RUNTIME_ERROR("Subset names list not found");
                    return pmc::Error::STATE_ERROR;
                }

                const auto subsetNames =
                    userData[kUSDJsonSubmeshNamesKey].Get<
                        VtArray<std::string>>();
                if (subsetNames.size() != attrPart.info.vectorCount) {
                    TF_RUNTIME_ERROR("Invalid number of subset names");
                    return pmc::Error::STATE_ERROR;
                }

                size_t indexPosition = 0;
                size_t currentLengthPosition = 0;
                for (const auto& ssn: subsetNames) {
                    VtIntArray subsetIndices;
                    size_t currentLength = attrValues[currentLengthPosition++];
                    subsetIndices.resize(currentLength);
                    for (auto pos = 0; pos < currentLength; pos++) {
                        subsetIndices[pos] = attrIndices[indexPosition++];
                    }
                    UsdGeomSubset::CreateGeomSubset(*decodedMesh, TfToken(ssn),
                                                    UsdGeomTokens->face,
                                                    subsetIndices);
                }

                return pmc::Error::OK;
            }

            // Holes
            if (attrPart.info.type == pmc::AttributeType::HOLE
                && attrPart.info.scope == pmc::AttributeScope::FACE
                && attrPart.info.indicesInterpretation == pmc::IndicesInterpretation::SCOPE_INDEXING) {
                UsdAttribute holeIndicesAttr = decodedMesh->CreateHoleIndicesAttr();
                if (!holeIndicesAttr.IsValid()) {
                    TF_RUNTIME_ERROR("Cannot create hole indices attribute");
                    return pmc::Error::STATE_ERROR;
                }
                holeIndicesAttr.Set(attrIndices);
                return pmc::Error::OK;
            }
            // Creases/Sharpnesses
            if ( decodeCrease ) {
                if (attrPart.info.type == pmc::AttributeType::SHARPNESS
                    && attrPart.info.derivedScope.scopedAttributeId == creaseAttrId) {
                    UsdAttribute creaseSharpness =
                        decodedMesh->CreateCreaseSharpnessesAttr();
                    if (!creaseSharpness.IsValid()) {
                        TF_RUNTIME_ERROR("Cannot create crease sharpnesses "
                                        "attribute");
                        return pmc::Error::STATE_ERROR;
                    }

                    auto scalingRange = ScalingRangeFrom(attrPart.info.coordSys);
                    if (!UsdPmc_WriteAttributeScalarValuesToUsdAttribute<float,
                                                                 UsdAttribute>(
                            attrPart, creaseSharpness, scalingRange)) {
                        TF_RUNTIME_ERROR("Cannot create crease sharpnesses "
                                        "attribute");
                        return pmc::Error::STATE_ERROR;
                    }
                    return pmc::Error::OK;
                }

                if (attrPart.info.attributeId == creaseAttrId) {
                    UsdAttribute creaseIndices =
                        decodedMesh->CreateCreaseIndicesAttr();
                    if (!creaseIndices.IsValid()) {
                        TF_RUNTIME_ERROR("Cannot create crease indices attribute");
                        return pmc::Error::STATE_ERROR;
                    }
                    creaseIndices.Set(attrIndices);
                    UsdAttribute creaseLengths =
                        decodedMesh->CreateCreaseLengthsAttr();
                    if (!creaseLengths.IsValid()) {
                        TF_RUNTIME_ERROR("Cannot create crease lengths attribute");
                        return pmc::Error::STATE_ERROR;
                    }
                    creaseLengths.Set(attrValues);
                    return pmc::Error::OK;
                }
            }

            // Corners/Sharpnesses
            if ( decodeCorner ) {
                if (attrPart.info.type == pmc::AttributeType::SHARPNESS
                    && attrPart.info.derivedScope.scopedAttributeId == cornerAttrId) {
                    UsdAttribute cornerSharpness =
                        decodedMesh->CreateCornerSharpnessesAttr();
                    if (!cornerSharpness.IsValid()) {
                        TF_RUNTIME_ERROR("Cannot create corner sharpnesses "
                                        "attribute");
                        return pmc::Error::STATE_ERROR;
                    }

                    auto scalingRange = ScalingRangeFrom(attrPart.info.coordSys);
                    if (!UsdPmc_WriteAttributeScalarValuesToUsdAttribute<float,
                                                                         UsdAttribute>(
                            attrPart, cornerSharpness, scalingRange)) {
                        TF_RUNTIME_ERROR("Cannot create crease sharpnesses "
                                        "attribute");
                        return pmc::Error::STATE_ERROR;
                    }
                    return pmc::Error::OK;
                }

                if (attrPart.info.attributeId == cornerAttrId) {
                    UsdAttribute cornerIndices =
                        decodedMesh->CreateCornerIndicesAttr();
                    if (!cornerIndices.IsValid()) {
                        TF_RUNTIME_ERROR("Cannot create corner indices attribute");
                        return pmc::Error::STATE_ERROR;
                    }
                    cornerIndices.Set(attrIndices);
                    return pmc::Error::OK;
                }
            }

            // General case
            if (attrName == "") {
                // Fallback to commonly used names
                _InferNameFromInfo(&attrName, attrPart.info);
            }

            auto attrTypeName = SdfValueTypeNames->IntArray;
            // Update attrTypeName
            if (!_ExtractTypeNameFromUserData(&attrTypeName, userData)) {
                _InferTypeNameFromName(&attrTypeName, attrName);
            }
            
            int16_t elementSize = -1;
            auto cpv = attrPart.info.componentsPerVector;
            auto dimension = attrTypeName.GetScalarType().GetDimensions();
            if ((dimension.size == 0 && cpv > 1) ||
                (dimension.size > 0 && dimension.d[0] != cpv)) {
                elementSize = cpv;
            }

            auto cppTypeName = attrTypeName.GetScalarType().GetCPPTypeName();

            if (auto usdAttribute = decodedMesh->GetPrim().CreateAttribute(
                    TfToken(attrName.c_str()), attrTypeName)) {
                // General case attribute
                if ( auto primVar = UsdGeomPrimvar(usdAttribute) ) {
                    if ( elementSize > 1 ) {
                        primVar.SetElementSize(elementSize);
                    }
                    bool res = false;
                    WRITE_ATTRIBUTE(UsdGeomPrimvar, primVar);

                    if ( res != true ) {
                        return pmc::Error::STATE_ERROR;
                    }

                    // Indices, if any
                    if ( attrIndices.size() > 0 ) {
                        primVar.SetIndices(attrIndices);
                    }

                    return pmc::Error::OK;
                } else {
                    bool res = false;
                    WRITE_ATTRIBUTE(UsdAttribute, usdAttribute);

                    if ( res != true ) {
                        return pmc::Error::STATE_ERROR;
                    }
                    if ( attrName == "normals" ) {
                        if ( attrIndices.size() > 0 ) {
                            auto normalsIndicesAttr = decodedMesh->GetPrim()
                                .CreateAttribute(TfToken("normals:indices"),
                                               SdfValueTypeNames->IntArray,
                                               false);
                            normalsIndicesAttr.Set(attrIndices);
                        }
                    }
                    return pmc::Error::OK;
                }
            } else {
                // Unable to create attribute
                TF_RUNTIME_ERROR("Error for attribute " + attrName +
                                 ": unable to create");
                return pmc::Error::STATE_ERROR;
            }
        };

    // Actual decoding step
    error = _dec.decode(bitstream, decodeFns);
    if ( error != pmc::Error::OK ) {
        TF_RUNTIME_ERROR("Failed to decode PMC bitstream: error %d",
                         int(error));
        return false;
    }

    return true;
}

void
UsdPmcMeshDecoder::_InferNameFromInfo(std::string* attrName,
                                      const pmc::AttributeMeshpartInfo& info) {
    if (!attrName) {
        TF_RUNTIME_ERROR("attrName cannot be null");
        return;
    }
    switch(info.type) {
        case pmc::AttributeType::TEX_COORD:
            *attrName = "primvars:st";
            break;
        case pmc::AttributeType::NORMAL:
            *attrName = "primvars:normals";
            break;
        case pmc::AttributeType::COLOR:
            *attrName = "primvars:displayColor";
            break;
        default:
            *attrName = "attribute_" + std::to_string(_unnamedAttributeCount++);
    }
}

/// This macro generates code to compare a string type name against a specific
/// USD value type name and set the output parameter if they match.
#define USD_PMC_CHECK_TYPENAME(typename) \
    if (strTypeName == SdfValueTypeNames->typename.GetAsToken().GetString()) { \
        typeName = SdfValueTypeNames->typename; \
        return true; \
    }

bool
UsdPmcMeshDecoder::_GetTypeNameFromString(const std::string_view strTypeName,
                                          SdfValueTypeName* typeName) {
    if (!typeName) {
        TF_RUNTIME_ERROR("typeName cannot be null");
        return false;
    }

    // Redefine the macro to use pointer syntax
    #undef USD_PMC_CHECK_TYPENAME
    #define USD_PMC_CHECK_TYPENAME(typename) \
        if (strTypeName == SdfValueTypeNames->typename.GetAsToken().GetString()) { \
            *typeName = SdfValueTypeNames->typename; \
            return true; \
        }

    // Basic array types
    USD_PMC_CHECK_TYPENAME(BoolArray);
    USD_PMC_CHECK_TYPENAME(UCharArray);
    USD_PMC_CHECK_TYPENAME(IntArray);
    USD_PMC_CHECK_TYPENAME(UIntArray);
    USD_PMC_CHECK_TYPENAME(Int64Array);
    USD_PMC_CHECK_TYPENAME(UInt64Array);
    USD_PMC_CHECK_TYPENAME(HalfArray);
    USD_PMC_CHECK_TYPENAME(FloatArray);
    USD_PMC_CHECK_TYPENAME(DoubleArray);
    USD_PMC_CHECK_TYPENAME(Double2Array);
    USD_PMC_CHECK_TYPENAME(Float2Array);
    USD_PMC_CHECK_TYPENAME(Half2Array);
    USD_PMC_CHECK_TYPENAME(Int2Array);
    USD_PMC_CHECK_TYPENAME(Double3Array);
    USD_PMC_CHECK_TYPENAME(Float3Array);
    USD_PMC_CHECK_TYPENAME(Half3Array);
    USD_PMC_CHECK_TYPENAME(Int3Array);
    USD_PMC_CHECK_TYPENAME(Double4Array);
    USD_PMC_CHECK_TYPENAME(Float4Array);
    USD_PMC_CHECK_TYPENAME(Half4Array);
    USD_PMC_CHECK_TYPENAME(Int4Array);

    // Role-based types for geometric data
    USD_PMC_CHECK_TYPENAME(Point3fArray);
    USD_PMC_CHECK_TYPENAME(Point3hArray);
    USD_PMC_CHECK_TYPENAME(Point3dArray);
    USD_PMC_CHECK_TYPENAME(Normal3fArray);
    USD_PMC_CHECK_TYPENAME(Normal3hArray);
    USD_PMC_CHECK_TYPENAME(Normal3dArray);
    USD_PMC_CHECK_TYPENAME(Vector3fArray);
    USD_PMC_CHECK_TYPENAME(Vector3hArray);
    USD_PMC_CHECK_TYPENAME(Vector3dArray);
    USD_PMC_CHECK_TYPENAME(Color3fArray);
    USD_PMC_CHECK_TYPENAME(Color3hArray);
    USD_PMC_CHECK_TYPENAME(Color3dArray);
    USD_PMC_CHECK_TYPENAME(Color4fArray);
    USD_PMC_CHECK_TYPENAME(Color4hArray);
    USD_PMC_CHECK_TYPENAME(Color4dArray);
    USD_PMC_CHECK_TYPENAME(TexCoord2fArray);
    USD_PMC_CHECK_TYPENAME(TexCoord2hArray);
    USD_PMC_CHECK_TYPENAME(TexCoord2dArray);
    USD_PMC_CHECK_TYPENAME(TexCoord3fArray);
    USD_PMC_CHECK_TYPENAME(TexCoord3hArray);
    USD_PMC_CHECK_TYPENAME(TexCoord3dArray);

    // TODO: Not yet supported by the plugin
    // USD_PMC_CHECK_TYPENAME(Matrix2dArray);
    // USD_PMC_CHECK_TYPENAME(Matrix3dArray);
    // USD_PMC_CHECK_TYPENAME(Matrix4dArray);
    // USD_PMC_CHECK_TYPENAME(QuatdArray);
    // USD_PMC_CHECK_TYPENAME(QuatfArray);
    // USD_PMC_CHECK_TYPENAME(QuathArray);

    // Default fallback type
    *typeName = SdfValueTypeNames->IntArray;
    return false;
}

bool
UsdPmcMeshDecoder::_ExtractTypeNameFromUserData(
    SdfValueTypeName* attrTypeName, const VtDictionary& userData) {
    if (!attrTypeName) {
        TF_RUNTIME_ERROR("attrTypeName cannot be null");
        return false;
    }
    const auto userTypeName = userData.find(kUSDJsonTypeNameKey);
    if ( userTypeName != userData.end() ) {
        return _GetTypeNameFromString(userTypeName->second.Get<std::string>(),
                                      attrTypeName);
    }
    *attrTypeName = SdfValueTypeNames->IntArray;
    return false;
}

void
UsdPmcMeshDecoder::_InferTypeNameFromName(SdfValueTypeName* attrTypeName,
                                          const std::string_view attrName) {
    if (!attrTypeName) {
        TF_RUNTIME_ERROR("attrTypeName cannot be null");
        return;
    }
    *attrTypeName = SdfValueTypeNames->IntArray;
    if ( attrName == "normals" || attrName == "primvars:normals" ) {
        *attrTypeName = SdfValueTypeNames->Normal3fArray;
    } else if ( attrName == "primvars:uv" || attrName == "primvars:st" ) {
        *attrTypeName = SdfValueTypeNames->TexCoord2fArray;
    } else if ( attrName == "primvars:displayColor" ) {
        *attrTypeName = SdfValueTypeNames->Color3fArray;
    } else if ( attrName == "primvars:displayOpacity" ) {
        *attrTypeName = SdfValueTypeNames->FloatArray;
    }
}

bool
UsdPmcMeshDecoder::_GetUserDataInfo(VtDictionary* userData,
                                    const std::string& jsonUserData) {
    if (!userData) {
        TF_RUNTIME_ERROR("userData cannot be null");
        return false;
    }
    try {
        if ( jsonUserData == "" ) {
            return true;
        }
        userData->clear();
        JsValue jsonData = JsParseString(jsonUserData);
        if ( jsonData.IsNull()
            || !jsonData.IsObject() ) {
            // Early exit: Nothing to parse
            return true;
        }

        const auto jsonDictionary = jsonData.GetJsObject();
        if ( jsonDictionary.find(kUSDJsonMainKey) == jsonDictionary.end() ) {
            return true;
        }
    
        auto usdJsObject = jsonDictionary.at(kUSDJsonMainKey);

        if ( usdJsObject.IsNull()
            || !usdJsObject.IsObject() ) {
            // Early exit: No usd json data
            return true;
        }

        auto mainUsdDictionary = usdJsObject.GetJsObject();

        if (mainUsdDictionary.find(kUSDJsonTypeNameKey) !=
                mainUsdDictionary.end()
            && !mainUsdDictionary.at(kUSDJsonTypeNameKey).IsNull()
            && mainUsdDictionary.at(kUSDJsonTypeNameKey).IsString() ) {
            userData->SetValueAtPath(kUSDJsonTypeNameKey,
                VtValue(mainUsdDictionary.at(kUSDJsonTypeNameKey).GetString()));
        }

        if (mainUsdDictionary.find(kUSDJsonSubmeshNamesKey) !=
                mainUsdDictionary.end()
            && !mainUsdDictionary.at(kUSDJsonSubmeshNamesKey).IsNull()
            && mainUsdDictionary.at(kUSDJsonSubmeshNamesKey)
                   .IsArrayOf<std::string>()) {
            VtArray<std::string> ssn;
            for (const auto& currentSSN: mainUsdDictionary
                     .at(kUSDJsonSubmeshNamesKey).GetArrayOf<std::string>()) {
                ssn.emplace_back(currentSSN);
            }
            userData->SetValueAtPath(kUSDJsonSubmeshNamesKey, VtValue(ssn));
        }

        return true;
    } catch(...) {
        return false;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
