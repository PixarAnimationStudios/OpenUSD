//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/usd/usdGeom/xformOp.h"
#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/usd/usd/prim.h"

#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec4d.h"

#include <vector>

TF_DEFINE_PUBLIC_TOKENS(UsdGeomXformOpTypes, USDGEOM_XFORM_OP_TYPES);

TF_REGISTRY_FUNCTION(TfEnum)
{
    // Type
    TF_ADD_ENUM_NAME(UsdGeomXformOp::TypeTranslate, "translate");
    TF_ADD_ENUM_NAME(UsdGeomXformOp::TypeScale,     "scale");
    TF_ADD_ENUM_NAME(UsdGeomXformOp::TypeRotateX,   "rotateX");
    TF_ADD_ENUM_NAME(UsdGeomXformOp::TypeRotateY,   "rotateY");
    TF_ADD_ENUM_NAME(UsdGeomXformOp::TypeRotateZ,   "rotateZ");
    TF_ADD_ENUM_NAME(UsdGeomXformOp::TypeRotateXYZ, "rotateXYZ");
    TF_ADD_ENUM_NAME(UsdGeomXformOp::TypeRotateXZY, "rotateXZY");
    TF_ADD_ENUM_NAME(UsdGeomXformOp::TypeRotateYXZ, "rotateYXZ");
    TF_ADD_ENUM_NAME(UsdGeomXformOp::TypeRotateYZX, "rotateYZX");
    TF_ADD_ENUM_NAME(UsdGeomXformOp::TypeRotateZXY, "rotateZXY");
    TF_ADD_ENUM_NAME(UsdGeomXformOp::TypeRotateZYX, "rotateZYX");
    TF_ADD_ENUM_NAME(UsdGeomXformOp::TypeOrient,    "orient");
    TF_ADD_ENUM_NAME(UsdGeomXformOp::TypeTransform, "transform");

    TF_ADD_ENUM_NAME(UsdGeomXformOp::PrecisionDouble, "Double");
    TF_ADD_ENUM_NAME(UsdGeomXformOp::PrecisionFloat, "Float");
    TF_ADD_ENUM_NAME(UsdGeomXformOp::PrecisionHalf, "Half");
};

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((xformOpPrefix, "xformOp:"))
    ((inverseXformOpPrefix, "!invert!xformOp:"))
    ((invertPrefix, "!invert!"))

    // This following tokens are not used here, but they're listed so they 
    // become immortal and are not ref-counted.
    // Tokens for the xformOps that are missing here (eg, RotateXYZ, translate,
    // scale etc.)are added in UsdGeomXformCommonAPI.
    ((xformOpTransform, "xformOp:transform"))
    ((xformOpRotateX, "xformOp:rotateX"))
    ((xformOpRotateY, "xformOp:rotateY"))
    ((xformOpRotateZ, "xformOp:rotateZ"))
    ((xformOpOrient, "xformOp:orient"))

    // XXX: backwards compatibility
    (transform)

);

// Validate that the given \p name contains the xform namespace.
// Does not validate name as a legal property identifier
static
bool
_IsNamespaced(const TfToken& opName)
{
    return TfStringStartsWith(opName, _tokens->xformOpPrefix);
}
    
static
TfToken
_MakeNamespaced(const TfToken& name)
{
    return _IsNamespaced(name) ? name : 
        TfToken(_tokens->xformOpPrefix.GetString() + 
                name.GetString());
}

// Returns whether the given op is an inverse operation. i.e, it starts with 
// "!invert!:xformOp:".
static
bool 
_IsInverseOp(TfToken const &opName)
{
    return TfStringStartsWith(opName, _tokens->inverseXformOpPrefix);
}

UsdGeomXformOp::UsdGeomXformOp(const UsdAttribute &attr, bool isInverseOp)
    : _attr(attr),
      _isInverseOp(isInverseOp)
{
    if (!attr) {
        TF_CODING_ERROR("UsdGeomXformOp created with invalid UsdAttribute.");
        return;
    }

    // _Initialize _opType.
    const TfToken &name = GetName();
    const std::vector<std::string> &opNameComponents = SplitName();

    if (_IsNamespaced(name)) {
        _opType = GetOpTypeEnum(TfToken(opNameComponents[1]));
    } else {
        TF_CODING_ERROR("Invalid xform op: <%s>.", attr.GetPath().GetText());
    }
}

TfToken 
UsdGeomXformOp::GetOpName() const
{
    return _isInverseOp ? TfToken(_tokens->invertPrefix.GetString() + 
                                  GetName().GetString()) 
                        : GetName();
}

/* static */
bool 
UsdGeomXformOp::IsXformOp(const UsdAttribute &attr)
{
    if (!attr)
        return false;

    return IsXformOp(attr.GetName());
}

/* static */
bool
UsdGeomXformOp::IsXformOp(const TfToken &attrName) 
{
    return _IsNamespaced(attrName);
}

/* static */
UsdAttribute
UsdGeomXformOp::_GetXformOpAttr(UsdPrim const& prim, const TfToken &opName,
                                bool *isInverseOp)
{
    *isInverseOp = _IsInverseOp(opName);

    // Is it is an inverse operation, strip off the "invert:" at the beginning 
    // of opName to get the associated attribute's name.
    const TfToken &xformOpAttrName = *isInverseOp 
        ? TfToken(opName.GetString().substr(
            _tokens->invertPrefix.GetString().size())) 
        : opName;
  
    return prim.GetAttribute(xformOpAttrName);
}

/* static */
UsdGeomXformOp::Precision 
UsdGeomXformOp::GetPrecisionFromValueTypeName(const SdfValueTypeName &typeName)
{
    if (typeName == SdfValueTypeNames->Matrix4d)
        return PrecisionDouble;
    else if (typeName == SdfValueTypeNames->Double3)
        return PrecisionDouble;
    else if (typeName == SdfValueTypeNames->Float3)
        return PrecisionFloat;
    else if (typeName == SdfValueTypeNames->Half3)
        return PrecisionHalf;
    else if (typeName == SdfValueTypeNames->Double)
        return PrecisionDouble;
    else if (typeName == SdfValueTypeNames->Float)
        return PrecisionFloat;
    else if (typeName == SdfValueTypeNames->Half)
        return PrecisionHalf;
    else if (typeName == SdfValueTypeNames->Quatd)
        return PrecisionDouble;
    else if (typeName == SdfValueTypeNames->Quatf)
        return PrecisionFloat;
    else if (typeName == SdfValueTypeNames->Quath)
        return PrecisionHalf;
    
    TF_CODING_ERROR("Invalid typeName '%s' specified.", typeName.GetAsToken().GetText());
    // Return default precision, which is double.
    return PrecisionDouble;
}

/* static */
TfToken const &
UsdGeomXformOp::GetOpTypeToken(UsdGeomXformOp::Type const opType)
{
    switch(opType) {
        case TypeTransform: return UsdGeomXformOpTypes->transform;
        case TypeTranslate: return UsdGeomXformOpTypes->translate;
        case TypeScale: return UsdGeomXformOpTypes->scale;
        case TypeRotateX: return UsdGeomXformOpTypes->rotateX;
        case TypeRotateY: return UsdGeomXformOpTypes->rotateY;
        case TypeRotateZ: return UsdGeomXformOpTypes->rotateZ;
        case TypeRotateXYZ: return UsdGeomXformOpTypes->rotateXYZ;
        case TypeRotateXZY: return UsdGeomXformOpTypes->rotateXZY;
        case TypeRotateYXZ: return UsdGeomXformOpTypes->rotateYXZ;
        case TypeRotateYZX: return UsdGeomXformOpTypes->rotateYZX;
        case TypeRotateZXY: return UsdGeomXformOpTypes->rotateZXY;
        case TypeRotateZYX: return UsdGeomXformOpTypes->rotateZYX;
        case TypeOrient: return UsdGeomXformOpTypes->orient;
        case TypeInvalid: 
        default: 
            static TfToken empty;
            return empty;
    }
}

/* static */
UsdGeomXformOp::Type 
UsdGeomXformOp::GetOpTypeEnum(TfToken const &opTypeToken)
{
    if (opTypeToken == UsdGeomXformOpTypes->transform)
        return TypeTransform;
    else if (opTypeToken == UsdGeomXformOpTypes->translate)
        return TypeTranslate;
    // RotateXYZ is expected to be more common than the remaining ops.
    else if (opTypeToken == UsdGeomXformOpTypes->rotateXYZ)
        return TypeRotateXYZ;
    else if (opTypeToken == UsdGeomXformOpTypes->scale)
        return TypeScale;
    else if (opTypeToken == UsdGeomXformOpTypes->rotateX)
        return TypeRotateX;
    else if (opTypeToken == UsdGeomXformOpTypes->rotateY)
        return TypeRotateY;
    else if (opTypeToken == UsdGeomXformOpTypes->rotateZ)
        return TypeRotateZ;
    else if (opTypeToken == UsdGeomXformOpTypes->rotateXZY)
        return TypeRotateXZY;
    else if (opTypeToken == UsdGeomXformOpTypes->rotateYXZ)
        return TypeRotateYXZ;
    else if (opTypeToken == UsdGeomXformOpTypes->rotateYZX)
        return TypeRotateYZX;
    else if (opTypeToken == UsdGeomXformOpTypes->rotateZXY)
        return TypeRotateZXY;
    else if (opTypeToken == UsdGeomXformOpTypes->rotateZYX)
        return TypeRotateZYX;
    else if (opTypeToken == UsdGeomXformOpTypes->orient)
        return TypeOrient;
    
    TF_CODING_ERROR("Invalid xform opType token %s.", opTypeToken.GetText());
    return TypeInvalid;
}

/* static */
const SdfValueTypeName &
UsdGeomXformOp::GetValueTypeName(
    const UsdGeomXformOp::Type opType,
    const UsdGeomXformOp::Precision precision)
{
    switch (opType) {
        case TypeTransform: {
            // Regardless of the requested precision, this must be Matrix4d,
            // because Matrix4f values are not supported in Sdf.
            if (precision != PrecisionDouble)
                TF_CODING_ERROR("Matrix transformations can only be encoded in "
                    "double precision. Overriding precision to double.");
            return SdfValueTypeNames->Matrix4d;
        }
        case TypeTranslate: 
        case TypeScale: 
        case TypeRotateXYZ: 
        case TypeRotateXZY: 
        case TypeRotateYXZ: 
        case TypeRotateYZX: 
        case TypeRotateZXY: 
        case TypeRotateZYX: {
            switch (precision) {
                case PrecisionFloat: return SdfValueTypeNames->Float3;
                case PrecisionHalf: return SdfValueTypeNames->Half3;
                case PrecisionDouble:
                default:
                    return SdfValueTypeNames->Double3;
            }
        }
        case TypeRotateX: 
        case TypeRotateY: 
        case TypeRotateZ: {
            switch (precision) {
                case PrecisionFloat: return SdfValueTypeNames->Float;
                case PrecisionHalf: return SdfValueTypeNames->Half;
                case PrecisionDouble:
                default:
                    return SdfValueTypeNames->Double;
            }
        }
        
        case TypeOrient: {
            switch (precision) {
                case PrecisionFloat: return SdfValueTypeNames->Quatf;
                case PrecisionHalf: return SdfValueTypeNames->Quath;
                case PrecisionDouble:
                default:
                    return SdfValueTypeNames->Quatd;
            }
        }

        case TypeInvalid: 
        default: {
            static SdfValueTypeName empty;
            return empty;
        }
    }
}

UsdGeomXformOp::UsdGeomXformOp(
    UsdPrim const& prim, 
    UsdGeomXformOp::Type const opType,
    UsdGeomXformOp::Precision const precision, 
    TfToken const &opSuffix,
    bool isInverseOp)
    : _opType(opType)
    , _isInverseOp(isInverseOp)
{
    // Determine the typeName of the xformOp attribute to be created.
    const SdfValueTypeName &typeName = GetValueTypeName(opType, precision);

    if (!typeName) { 
        TF_CODING_ERROR("Invalid xform-op: incompatible combination of "
            "opType (%s) and precision (%s).", 
            TfEnum::GetName(opType).c_str(),
            TfEnum::GetName(precision).c_str());
        return;
    } 

    TfToken attrName = UsdGeomXformOp::GetOpName(opType, opSuffix, 
        // isInverseOp is handled below
        /*isInverseOp*/ false);

    // attrName can never be empty.
    TF_VERIFY(!attrName.IsEmpty());

    // Create an  attribute in the xformOp: namespace with the
    // computed typeName.
    _attr = prim.CreateAttribute(attrName, typeName, /* custom */ false);

    // If a problem occurred, an error should already have been issued,
    // and _attr will be invalid, which is what we want
}

UsdGeomXformOp::Precision 
UsdGeomXformOp::GetPrecision() const
{
    return GetPrecisionFromValueTypeName(GetTypeName());
}

/* static */
TfToken 
UsdGeomXformOp::GetOpName(
    const Type opType, 
    const TfToken &opSuffix,
    bool isInverseOp)
{
    TfToken opName = _MakeNamespaced(GetOpTypeToken(opType));

    if (!opSuffix.IsEmpty())
        opName = TfToken(opName.GetString() + ":" + opSuffix.GetString());

    if (isInverseOp)
        opName = TfToken(_tokens->invertPrefix.GetString() + opName.GetString());

    return opName;
}

bool
UsdGeomXformOp::HasSuffix(const TfToken &suffix) const
{
    return TfStringEndsWith(GetName(), suffix);
}

/* static */
GfMatrix4d 
UsdGeomXformOp::GetOpTransform(UsdGeomXformOp::Type const opType,
                               VtValue const &opVal,
                               bool isInverseOp)
{
    // This will be the most common case.
    if (opType == TypeTransform) {
        GfMatrix4d mat(1.);
        bool isMatrixVal = true;
        if (opVal.IsHolding<GfMatrix4d>()) {
            mat = opVal.UncheckedGet<GfMatrix4d>();
        } else if (opVal.IsHolding<GfMatrix4f>()) {
            mat = GfMatrix4d(opVal.UncheckedGet<GfMatrix4f>());
        } else {
            isMatrixVal = false;
            TF_CODING_ERROR("Invalid combination of opType (%s) "
                "and opVal (%s). Returning identity matrix.",
                TfEnum::GetName(opType).c_str(),
                TfStringify(opVal).c_str());
            return GfMatrix4d(1.);
        } 

        if (isMatrixVal && isInverseOp) {
            double determinant=0;
            mat = mat.GetInverse(&determinant);

            if (GfIsClose(determinant, 0.0, 1e-9)) {
                TF_CODING_ERROR("Cannot invert singular transform op with "
                    "value %s.", TfStringify(opVal).c_str());
            }
        }

        return mat;
    }

    double doubleVal = 0.;
    bool isScalarVal = true;
    if (opVal.IsHolding<double>()) {
        doubleVal  = opVal.UncheckedGet<double>();
    } else if (opVal.IsHolding<float>()) {
        doubleVal = opVal.UncheckedGet<float>();
    } else if (opVal.IsHolding<half>()) {
        doubleVal = opVal.UncheckedGet<half>();
    } else {
        isScalarVal = false;
    }

    if (isScalarVal) {
        if (isInverseOp) 
            doubleVal = -doubleVal;

        if (opType == TypeRotateX) {
            return GfMatrix4d(1.).SetRotate(GfRotation(GfVec3d::XAxis(), doubleVal));
        } else if (opType == TypeRotateY) {
            return GfMatrix4d(1.).SetRotate(GfRotation(GfVec3d::YAxis(), doubleVal));
        } else if (opType == TypeRotateZ) {
            return GfMatrix4d(1.).SetRotate(GfRotation(GfVec3d::ZAxis(), doubleVal));
        }
    }

    GfVec3d vec3dVal = GfVec3d(0.);
    bool isVecVal = true;
    if (opVal.IsHolding<GfVec3f>()) {
        vec3dVal = opVal.UncheckedGet<GfVec3f>();
    } else if (opVal.IsHolding<GfVec3d>()) {
        vec3dVal = opVal.UncheckedGet<GfVec3d>();
    } else if (opVal.IsHolding<GfVec3h>()) {
        vec3dVal = opVal.UncheckedGet<GfVec3h>();
    } else {
        isVecVal = false;
    }

    if (isVecVal) {
        switch(opType) {
            case TypeTranslate:
                if (isInverseOp) 
                    vec3dVal = -vec3dVal;
                return GfMatrix4d(1.).SetTranslate(vec3dVal);
            case TypeScale:
                if (isInverseOp) {
                    vec3dVal = GfVec3d(1/vec3dVal[0], 
                                       1/vec3dVal[1], 
                                       1/vec3dVal[2]);
                }
                return GfMatrix4d(1.).SetScale(vec3dVal);
            default: {
                if (isInverseOp) 
                    vec3dVal = -vec3dVal;
                // Must be one of the 3-axis rotates.
                GfMatrix3d xRot(GfRotation(GfVec3d::XAxis(), vec3dVal[0]));
                GfMatrix3d yRot(GfRotation(GfVec3d::YAxis(), vec3dVal[1]));
                GfMatrix3d zRot(GfRotation(GfVec3d::ZAxis(), vec3dVal[2]));
                GfMatrix3d rotationMat(1.);
                switch (opType) {
                    case TypeRotateXYZ: 
                        // Inv(ABC) = Inv(C) * Inv(B) * Inv(A)
                        rotationMat = !isInverseOp ? (xRot * yRot * zRot) 
                                                      : (zRot * yRot * xRot);
                        break;
                    case TypeRotateXZY: 
                        rotationMat = !isInverseOp ? (xRot * zRot * yRot)
                                                      : (yRot * zRot * xRot);
                        break;
                    case TypeRotateYXZ: 
                        rotationMat = !isInverseOp ? (yRot * xRot * zRot)
                                                      : (zRot * xRot * yRot);
                        break;
                    case TypeRotateYZX: 
                        rotationMat = !isInverseOp ? (yRot * zRot * xRot)
                                                      : (xRot * zRot * yRot);
                        break;
                    case TypeRotateZXY:
                        rotationMat = !isInverseOp ? (zRot * xRot * yRot)
                                                      : (yRot * xRot * zRot);
                        break;
                    case TypeRotateZYX: 
                        rotationMat = !isInverseOp ? (zRot * yRot * xRot)
                                                      : (xRot * yRot * zRot);
                        break;
                    default:
                        TF_CODING_ERROR("Invalid combination of opType (%s) "
                            "and opVal (%s). Returning identity matrix.",
                            TfEnum::GetName(opType).c_str(),
                            TfStringify(opVal).c_str());
                        return GfMatrix4d(1.);
                }
                return GfMatrix4d(1.).SetRotate(rotationMat);
            }
        }
    }

    if (opType == TypeOrient) {
        GfQuatd quatVal(0);
        if (opVal.IsHolding<GfQuatd>())
            quatVal = opVal.UncheckedGet<GfQuatd>();
        else if (opVal.IsHolding<GfQuatf>()) {
            const GfQuatf &quatf = opVal.UncheckedGet<GfQuatf>();
            quatVal = GfQuatd(quatf.GetReal(), quatf.GetImaginary());
        } else if (opVal.IsHolding<GfQuath>()) {
            const GfQuath &quath = opVal.UncheckedGet<GfQuath>();
            quatVal = GfQuatd(quath.GetReal(), quath.GetImaginary());
        }

        GfRotation quatRotation(quatVal);
        if (isInverseOp)
            quatRotation = quatRotation.GetInverse();

        return GfMatrix4d(quatRotation, GfVec3d(0.));
    }
    
    TF_CODING_ERROR("Invalid combination of opType (%s) and opVal (%s). "
        "Returning identity matrix.", TfEnum::GetName(opType).c_str(), 
        TfStringify(opVal).c_str());

    return GfMatrix4d(1.);
}

GfMatrix4d 
UsdGeomXformOp::GetOpTransform(UsdTimeCode time) const
{
    GfMatrix4d result(1);

    VtValue opVal;
    if (!Get(&opVal, time))
        return result;

    result = GetOpTransform(GetOpType(), opVal, _isInverseOp); 

    return result;
}
