//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDVOL_TOKENS_H
#define USDVOL_TOKENS_H

/// \file usdVol/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "pxr/usd/usdVol/api.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdVolTokensType
///
/// \link UsdVolTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdVolTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use UsdVolTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(UsdVolTokens->bool_);
/// \endcode
struct UsdVolTokensType {
    USDVOL_API UsdVolTokensType();
    /// \brief "bool"
    /// 
    /// Possible value for UsdVolOpenVDBAsset::GetFieldDataTypeAttr()
    const TfToken bool_;
    /// \brief "cameraDistance"
    /// 
    /// Possible value for UsdVolParticleField3DGaussianSplat::GetSortingModeHintAttr()
    const TfToken cameraDistance;
    /// \brief "Color"
    /// 
    /// Possible value for UsdVolVolumeFieldAsset::GetVectorDataRoleHintAttr()
    const TfToken Color;
    /// \brief "double2"
    /// 
    /// Possible value for UsdVolOpenVDBAsset::GetFieldDataTypeAttr()
    const TfToken double2;
    /// \brief "double3"
    /// 
    /// Possible value for UsdVolField3DAsset::GetFieldDataTypeAttr(), Possible value for UsdVolOpenVDBAsset::GetFieldDataTypeAttr()
    const TfToken double3;
    /// \brief "double"
    /// 
    /// Possible value for UsdVolField3DAsset::GetFieldDataTypeAttr(), Possible value for UsdVolOpenVDBAsset::GetFieldDataTypeAttr()
    const TfToken double_;
    /// \brief "field"
    /// 
    /// This is the namespace prefix used to  specify the fields that make up a volume primitive.
    const TfToken field;
    /// \brief "fieldClass"
    /// 
    /// UsdVolOpenVDBAsset
    const TfToken fieldClass;
    /// \brief "fieldDataType"
    /// 
    /// UsdVolVolumeFieldAsset, UsdVolField3DAsset, UsdVolOpenVDBAsset
    const TfToken fieldDataType;
    /// \brief "fieldIndex"
    /// 
    /// UsdVolVolumeFieldAsset
    const TfToken fieldIndex;
    /// \brief "fieldName"
    /// 
    /// UsdVolVolumeFieldAsset
    const TfToken fieldName;
    /// \brief "fieldPurpose"
    /// 
    /// UsdVolField3DAsset
    const TfToken fieldPurpose;
    /// \brief "filePath"
    /// 
    /// UsdVolVolumeFieldAsset
    const TfToken filePath;
    /// \brief "float2"
    /// 
    /// Possible value for UsdVolOpenVDBAsset::GetFieldDataTypeAttr()
    const TfToken float2;
    /// \brief "float3"
    /// 
    /// Possible value for UsdVolField3DAsset::GetFieldDataTypeAttr(), Possible value for UsdVolOpenVDBAsset::GetFieldDataTypeAttr()
    const TfToken float3;
    /// \brief "float"
    /// 
    /// Possible value for UsdVolField3DAsset::GetFieldDataTypeAttr(), Possible value for UsdVolOpenVDBAsset::GetFieldDataTypeAttr()
    const TfToken float_;
    /// \brief "fogVolume"
    /// 
    /// Possible value for UsdVolOpenVDBAsset::GetFieldClassAttr()
    const TfToken fogVolume;
    /// \brief "half"
    /// 
    /// Possible value for UsdVolField3DAsset::GetFieldDataTypeAttr(), Possible value for UsdVolOpenVDBAsset::GetFieldDataTypeAttr()
    const TfToken half;
    /// \brief "half2"
    /// 
    /// Possible value for UsdVolOpenVDBAsset::GetFieldDataTypeAttr()
    const TfToken half2;
    /// \brief "half3"
    /// 
    /// Possible value for UsdVolField3DAsset::GetFieldDataTypeAttr(), Possible value for UsdVolOpenVDBAsset::GetFieldDataTypeAttr()
    const TfToken half3;
    /// \brief "int2"
    /// 
    /// Possible value for UsdVolOpenVDBAsset::GetFieldDataTypeAttr()
    const TfToken int2;
    /// \brief "int3"
    /// 
    /// Possible value for UsdVolOpenVDBAsset::GetFieldDataTypeAttr()
    const TfToken int3;
    /// \brief "int64"
    /// 
    /// Possible value for UsdVolOpenVDBAsset::GetFieldDataTypeAttr()
    const TfToken int64;
    /// \brief "int"
    /// 
    /// Possible value for UsdVolOpenVDBAsset::GetFieldDataTypeAttr()
    const TfToken int_;
    /// \brief "levelSet"
    /// 
    /// Possible value for UsdVolOpenVDBAsset::GetFieldClassAttr()
    const TfToken levelSet;
    /// \brief "mask"
    /// 
    /// Possible value for UsdVolOpenVDBAsset::GetFieldDataTypeAttr()
    const TfToken mask;
    /// \brief "matrix3d"
    /// 
    /// Possible value for UsdVolOpenVDBAsset::GetFieldDataTypeAttr()
    const TfToken matrix3d;
    /// \brief "matrix4d"
    /// 
    /// Possible value for UsdVolOpenVDBAsset::GetFieldDataTypeAttr()
    const TfToken matrix4d;
    /// \brief "None"
    /// 
    /// Fallback value for UsdVolVolumeFieldAsset::GetVectorDataRoleHintAttr()
    const TfToken None_;
    /// \brief "Normal"
    /// 
    /// Possible value for UsdVolVolumeFieldAsset::GetVectorDataRoleHintAttr()
    const TfToken Normal;
    /// \brief "opacities"
    /// 
    /// UsdVolParticleFieldOpacityAttributeAPI
    const TfToken opacities;
    /// \brief "opacitiesh"
    /// 
    /// UsdVolParticleFieldOpacityAttributeAPI
    const TfToken opacitiesh;
    /// \brief "orientations"
    /// 
    /// UsdVolParticleFieldOrientationAttributeAPI
    const TfToken orientations;
    /// \brief "orientationsh"
    /// 
    /// UsdVolParticleFieldOrientationAttributeAPI
    const TfToken orientationsh;
    /// \brief "perspective"
    /// 
    /// Fallback value for UsdVolParticleField3DGaussianSplat::GetProjectionModeHintAttr()
    const TfToken perspective;
    /// \brief "Point"
    /// 
    /// Possible value for UsdVolVolumeFieldAsset::GetVectorDataRoleHintAttr()
    const TfToken Point;
    /// \brief "positions"
    /// 
    /// UsdVolParticleFieldPositionAttributeAPI
    const TfToken positions;
    /// \brief "positionsh"
    /// 
    /// UsdVolParticleFieldPositionAttributeAPI
    const TfToken positionsh;
    /// \brief "projectionModeHint"
    /// 
    /// UsdVolParticleField3DGaussianSplat
    const TfToken projectionModeHint;
    /// \brief "quatd"
    /// 
    /// Possible value for UsdVolOpenVDBAsset::GetFieldDataTypeAttr()
    const TfToken quatd;
    /// \brief "radiance:sphericalHarmonicsCoefficients"
    /// 
    /// UsdVolParticleFieldSphericalHarmonicsAttributeAPI
    const TfToken radianceSphericalHarmonicsCoefficients;
    /// \brief "radiance:sphericalHarmonicsCoefficientsh"
    /// 
    /// UsdVolParticleFieldSphericalHarmonicsAttributeAPI
    const TfToken radianceSphericalHarmonicsCoefficientsh;
    /// \brief "radiance:sphericalHarmonicsDegree"
    /// 
    /// UsdVolParticleFieldSphericalHarmonicsAttributeAPI
    const TfToken radianceSphericalHarmonicsDegree;
    /// \brief "scales"
    /// 
    /// UsdVolParticleFieldScaleAttributeAPI
    const TfToken scales;
    /// \brief "scalesh"
    /// 
    /// UsdVolParticleFieldScaleAttributeAPI
    const TfToken scalesh;
    /// \brief "sortingModeHint"
    /// 
    /// UsdVolParticleField3DGaussianSplat
    const TfToken sortingModeHint;
    /// \brief "staggered"
    /// 
    /// Possible value for UsdVolOpenVDBAsset::GetFieldClassAttr()
    const TfToken staggered;
    /// \brief "string"
    /// 
    /// Possible value for UsdVolOpenVDBAsset::GetFieldDataTypeAttr()
    const TfToken string;
    /// \brief "tangential"
    /// 
    /// Possible value for UsdVolParticleField3DGaussianSplat::GetProjectionModeHintAttr()
    const TfToken tangential;
    /// \brief "uint"
    /// 
    /// Possible value for UsdVolOpenVDBAsset::GetFieldDataTypeAttr()
    const TfToken uint;
    /// \brief "unknown"
    /// 
    /// Possible value for UsdVolOpenVDBAsset::GetFieldClassAttr()
    const TfToken unknown;
    /// \brief "Vector"
    /// 
    /// Possible value for UsdVolVolumeFieldAsset::GetVectorDataRoleHintAttr()
    const TfToken Vector;
    /// \brief "vectorDataRoleHint"
    /// 
    /// UsdVolVolumeFieldAsset
    const TfToken vectorDataRoleHint;
    /// \brief "zDepth"
    /// 
    /// Fallback value for UsdVolParticleField3DGaussianSplat::GetSortingModeHintAttr()
    const TfToken zDepth;
    /// \brief "Field3DAsset"
    /// 
    /// Schema identifer and family for UsdVolField3DAsset
    const TfToken Field3DAsset;
    /// \brief "FieldAsset"
    /// 
    /// Schema identifer and family for UsdVolFieldAsset
    const TfToken FieldAsset;
    /// \brief "FieldBase"
    /// 
    /// Schema identifer and family for UsdVolFieldBase
    const TfToken FieldBase;
    /// \brief "OpenVDBAsset"
    /// 
    /// Schema identifer and family for UsdVolOpenVDBAsset
    const TfToken OpenVDBAsset;
    /// \brief "ParticleField"
    /// 
    /// Schema identifer and family for UsdVolParticleField
    const TfToken ParticleField;
    /// \brief "ParticleField3DGaussianSplat"
    /// 
    /// Schema identifer and family for UsdVolParticleField3DGaussianSplat
    const TfToken ParticleField3DGaussianSplat;
    /// \brief "ParticleFieldKernelBaseAPI"
    /// 
    /// Schema identifer and family for UsdVolParticleFieldKernelBaseAPI
    const TfToken ParticleFieldKernelBaseAPI;
    /// \brief "ParticleFieldKernelConstantSurfletAPI"
    /// 
    /// Schema identifer and family for UsdVolParticleFieldKernelConstantSurfletAPI
    const TfToken ParticleFieldKernelConstantSurfletAPI;
    /// \brief "ParticleFieldKernelGaussianEllipsoidAPI"
    /// 
    /// Schema identifer and family for UsdVolParticleFieldKernelGaussianEllipsoidAPI
    const TfToken ParticleFieldKernelGaussianEllipsoidAPI;
    /// \brief "ParticleFieldKernelGaussianSurfletAPI"
    /// 
    /// Schema identifer and family for UsdVolParticleFieldKernelGaussianSurfletAPI
    const TfToken ParticleFieldKernelGaussianSurfletAPI;
    /// \brief "ParticleFieldOpacityAttributeAPI"
    /// 
    /// Schema identifer and family for UsdVolParticleFieldOpacityAttributeAPI
    const TfToken ParticleFieldOpacityAttributeAPI;
    /// \brief "ParticleFieldOrientationAttributeAPI"
    /// 
    /// Schema identifer and family for UsdVolParticleFieldOrientationAttributeAPI
    const TfToken ParticleFieldOrientationAttributeAPI;
    /// \brief "ParticleFieldPositionAttributeAPI"
    /// 
    /// Schema identifer and family for UsdVolParticleFieldPositionAttributeAPI
    const TfToken ParticleFieldPositionAttributeAPI;
    /// \brief "ParticleFieldPositionBaseAPI"
    /// 
    /// Schema identifer and family for UsdVolParticleFieldPositionBaseAPI
    const TfToken ParticleFieldPositionBaseAPI;
    /// \brief "ParticleFieldRadianceBaseAPI"
    /// 
    /// Schema identifer and family for UsdVolParticleFieldRadianceBaseAPI
    const TfToken ParticleFieldRadianceBaseAPI;
    /// \brief "ParticleFieldScaleAttributeAPI"
    /// 
    /// Schema identifer and family for UsdVolParticleFieldScaleAttributeAPI
    const TfToken ParticleFieldScaleAttributeAPI;
    /// \brief "ParticleFieldSphericalHarmonicsAttributeAPI"
    /// 
    /// Schema identifer and family for UsdVolParticleFieldSphericalHarmonicsAttributeAPI
    const TfToken ParticleFieldSphericalHarmonicsAttributeAPI;
    /// \brief "Volume"
    /// 
    /// Schema identifer and family for UsdVolVolume
    const TfToken Volume;
    /// \brief "VolumeFieldAsset"
    /// 
    /// Schema identifer and family for UsdVolVolumeFieldAsset
    const TfToken VolumeFieldAsset;
    /// \brief "VolumeFieldBase"
    /// 
    /// Schema identifer and family for UsdVolVolumeFieldBase
    const TfToken VolumeFieldBase;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdVolTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdVolTokensType
extern USDVOL_API TfStaticData<UsdVolTokensType> UsdVolTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
