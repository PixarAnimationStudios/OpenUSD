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
#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/xformCommonAPI.h"

#include "pxr/base/gf/rotation.h"
#include "pxr/base/trace/trace.h"

#include <map>

PXR_NAMESPACE_OPEN_SCOPE


using std::vector;

const int UsdGeomXformCommonAPI::_InvalidIndex;

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(UsdGeomXformCommonAPI::RotationOrderXYZ, "XYZ");
    TF_ADD_ENUM_NAME(UsdGeomXformCommonAPI::RotationOrderXZY, "XZY");
    TF_ADD_ENUM_NAME(UsdGeomXformCommonAPI::RotationOrderYXZ, "YXZ");
    TF_ADD_ENUM_NAME(UsdGeomXformCommonAPI::RotationOrderYZX, "YZX");
    TF_ADD_ENUM_NAME(UsdGeomXformCommonAPI::RotationOrderZXY, "ZXY");
    TF_ADD_ENUM_NAME(UsdGeomXformCommonAPI::RotationOrderZYX, "ZYX");
};

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (pivot)
    ((xformOpTranslate, "xformOp:translate"))
    ((xformOpTranslatePivot, "xformOp:translate:pivot"))
    ((xformOpRotateXYZ, "xformOp:rotateXYZ"))
    ((xformOpScale, "xformOp:scale"))
    ((xformOpInvTranslatePivot, "!invert!xformOp:translate:pivot"))
);

// List of valid rotate op types.
TF_MAKE_STATIC_DATA(std::set<UsdGeomXformOp::Type>, _validRotateTypes) {
    *_validRotateTypes = {
        UsdGeomXformOp::TypeRotateXYZ,
        UsdGeomXformOp::TypeRotateXZY,
        UsdGeomXformOp::TypeRotateYXZ,
        UsdGeomXformOp::TypeRotateYZX,
        UsdGeomXformOp::TypeRotateZXY,
        UsdGeomXformOp::TypeRotateZYX
    };
}

TF_MAKE_STATIC_DATA(std::set<UsdGeomXformOp::Type>, _validSingleAxisRotateTypes) {
    *_validSingleAxisRotateTypes = {
        UsdGeomXformOp::TypeRotateX,
        UsdGeomXformOp::TypeRotateY,
        UsdGeomXformOp::TypeRotateZ
    };
}


/* virtual */
UsdGeomXformCommonAPI::~UsdGeomXformCommonAPI()
{
}

/* static */
UsdGeomXformCommonAPI
UsdGeomXformCommonAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomXformCommonAPI();
    }

    UsdGeomXformable xformable(stage->GetPrimAtPath(path));

    return UsdGeomXformCommonAPI(xformable);
}

/* virtual */
bool 
UsdGeomXformCommonAPI::_IsCompatible() const
{
    if (!UsdSchemaBase::_IsCompatible())
        return false;

    if (!_xformable)
        return false;

    if (_computedOpIndices)
        return true;
    
    return _ValidateAndComputeXformOpIndices();    
}

// Assumes rotationOrder is XYZ.
static void
_RotMatToRotXYZ(
    const GfMatrix4d &rotMat,
    GfVec3f *rotXYZ)
{
    GfRotation rot = rotMat.ExtractRotation();
    GfVec3d angles = rot.Decompose(GfVec3d::ZAxis(),
                                   GfVec3d::YAxis(),
                                   GfVec3d::XAxis());
    *rotXYZ = GfVec3f(angles[2], angles[1], angles[0]);
}

static void
_ConvertMatrixToComponents(const GfMatrix4d &matrix, 
                           GfVec3d *translation, 
                           GfVec3f *rotation,
                           GfVec3f *scale)
{
    GfMatrix4d rotMat(1.0);
    GfVec3d doubleScale(1.0);
    GfMatrix4d scaleOrientMatUnused, perspMatUnused;
    matrix.Factor(&scaleOrientMatUnused, &doubleScale, &rotMat, 
                    translation, &perspMatUnused);

    *scale = GfVec3f(doubleScale[0], doubleScale[1], doubleScale[2]);

    if (!rotMat.Orthonormalize(/* issueWarning */ false))
        TF_WARN("Failed to orthonormalize rotation matrix.");

    _RotMatToRotXYZ(rotMat, rotation);
}

bool 
UsdGeomXformCommonAPI::SetXformVectors(
    const GfVec3d &translation,
    const GfVec3f &rotation,
    const GfVec3f &scale, 
    const GfVec3f &pivot,
    RotationOrder rotOrder,
    const UsdTimeCode time)
{
    if (!_VerifyCompatibility())
        return false;

    // SetRotate is called first, so that the rotation order compatibility is 
    // checked before any data is authored.
    return SetRotate(rotation, rotOrder, time) &&
           SetTranslate(translation, time)     &&
           SetScale(scale, time)               &&
           SetPivot(pivot, time);
}

static
bool
_IsMatrixIdentity(const GfMatrix4d& matrix)
{
    const GfMatrix4d IDENTITY(1.0);
    const double TOLERANCE = 1e-6;

    if (GfIsClose(matrix.GetRow(0), IDENTITY.GetRow(0), TOLERANCE)      &&
            GfIsClose(matrix.GetRow(1), IDENTITY.GetRow(1), TOLERANCE)  &&
            GfIsClose(matrix.GetRow(2), IDENTITY.GetRow(2), TOLERANCE)  &&
            GfIsClose(matrix.GetRow(3), IDENTITY.GetRow(3), TOLERANCE)) {
        return true;
    }

    return false;
}

static
bool
_MatricesAreInverses(const GfMatrix4d& matrix1, const GfMatrix4d& matrix2)
{
    GfMatrix4d mult = matrix1 * matrix2;
    return _IsMatrixIdentity(mult);
}

static
UsdGeomXformOp::Type
_GetRotateOpType(const vector<UsdGeomXformOp>& ops,
                 bool includeSingleAxisTypes=false)
{
    TF_FOR_ALL(it, ops) {
        if (_validRotateTypes->count(it->GetOpType()))
            return it->GetOpType();
        if (includeSingleAxisTypes &&
                _validSingleAxisRotateTypes->count(it->GetOpType()))
            return it->GetOpType();
    }
    return UsdGeomXformOp::TypeRotateXYZ;
}

static
UsdGeomXformCommonAPI::RotationOrder
_GetRotationOrderFromRotateOpType(const UsdGeomXformOp::Type opType)
{
    switch (opType) {
        case UsdGeomXformOp::TypeRotateXZY:
            return UsdGeomXformCommonAPI::RotationOrderXZY;
        case UsdGeomXformOp::TypeRotateYXZ:
            return UsdGeomXformCommonAPI::RotationOrderYXZ;
        case UsdGeomXformOp::TypeRotateYZX:
            return UsdGeomXformCommonAPI::RotationOrderYZX;
        case UsdGeomXformOp::TypeRotateZXY:
            return UsdGeomXformCommonAPI::RotationOrderZXY;
        case UsdGeomXformOp::TypeRotateZYX:
            return UsdGeomXformCommonAPI::RotationOrderZYX;
        case UsdGeomXformOp::TypeRotateX:
        case UsdGeomXformOp::TypeRotateY:
        case UsdGeomXformOp::TypeRotateZ:
        case UsdGeomXformOp::TypeRotateXYZ:
        default:
            // Default rotation order is XYZ.
            return UsdGeomXformCommonAPI::RotationOrderXYZ;
    }
}

static
UsdGeomXformCommonAPI::RotationOrder
_GetRotationOrderFromRotateOp(const UsdGeomXformOp& rotateOp)
{
    if (!rotateOp) {
        return UsdGeomXformCommonAPI::RotationOrderXYZ;
    }

    return _GetRotationOrderFromRotateOpType(rotateOp.GetOpType());
}

// This helper method looks through the given xformOps and returns a vector of
// common op types that the xformOps could possibly be reduced to by
// accumulation.
static
vector<UsdGeomXformOp::Type>
_GetCommonOpTypesForOpOrder(const vector<UsdGeomXformOp>& xformOps,
                            int* translateIndex,
                            int* translatePivotIndex,
                            int* rotateIndex,
                            int* translateIdentityIndex,
                            int* scaleIndex,
                            int* translatePivotInvertIndex) {
    UsdGeomXformOp::Type rotateOpType = UsdGeomXformOp::TypeRotateXYZ;
    bool hasRotateOp = false;
    bool hasScaleOp = false;
    size_t numInverseTranslateOps = 0;

    TF_FOR_ALL(it, xformOps) {
        if (_validRotateTypes->count(it->GetOpType()) ||
                _validSingleAxisRotateTypes->count(it->GetOpType())) {
            hasRotateOp = true;
            rotateOpType = it->GetOpType();
        } else if (it->GetOpType() == UsdGeomXformOp::TypeScale) {
            hasScaleOp = true;
        } else if (it->GetOpType() == UsdGeomXformOp::TypeTranslate &&
                   it->IsInverseOp()) {
            ++numInverseTranslateOps;
        }
    }

    vector<UsdGeomXformOp::Type> commonOpTypes;
    size_t currentIndex = 0;

    // The translate and translatePivot will always be present, and so will
    // translatePivotInvert below. Initialize the rest to invalid.
    commonOpTypes.push_back(UsdGeomXformOp::TypeTranslate);
    commonOpTypes.push_back(UsdGeomXformOp::TypeTranslate);
    *translateIndex = currentIndex++;
    *translatePivotIndex = currentIndex++;
    *rotateIndex = -1;
    *translateIdentityIndex = -1;
    *scaleIndex = -1;

    if (hasRotateOp) {
        commonOpTypes.push_back(rotateOpType);
        *rotateIndex = currentIndex++;
    }

    if (numInverseTranslateOps > 1) {
        // If more than one inverse translate is present, assume that means
        // that both a rotate pivot and a scale pivot are specified. For it to
        // be reducible, they must be at the same location in space, in which
        // case they'll accumulate to identity in the translateIdentityIndex
        // position.
        commonOpTypes.push_back(UsdGeomXformOp::TypeTranslate);
        *translateIdentityIndex = currentIndex++;
    }

    if (hasScaleOp) {
        commonOpTypes.push_back(UsdGeomXformOp::TypeScale);
        *scaleIndex = currentIndex++;
    }

    commonOpTypes.push_back(UsdGeomXformOp::TypeTranslate);
    *translatePivotInvertIndex = currentIndex++;

    return commonOpTypes;
}

bool 
UsdGeomXformCommonAPI::GetXformVectors(
    GfVec3d *translation, 
    GfVec3f *rotation,
    GfVec3f *scale,
    GfVec3f *pivot,
    RotationOrder *rotOrder,
    const UsdTimeCode time)
{
    // Handle incompatible xform case first.
    // It's ok for an xform to be incompatible when extracting xform vectors. 
    if (!_IsCompatible()) {
        GfMatrix4d localXform(1.);

        // Do we want to be able to use a UsdGeomXformCache for this?
        bool resetsXformStack = false;
        _xformable.GetLocalTransformation(&localXform, &resetsXformStack, time);

        // We don't process (or return) resetsXformStack here. It is up to the 
        // clients to call GetResetXformStack() and process it suitably.
        _ConvertMatrixToComponents(localXform, translation, rotation, scale);

        *pivot = GfVec3f(0.,0.,0.);

        *rotOrder = RotationOrderXYZ;

        return true;
    }

    if (!_computedOpIndices &&
        !TF_VERIFY(_ComputeOpIndices(), "XformCommonAPI: Failed to"
            "initialize xformOps on a compatible xformable <%s>.",
            _xformable.GetPath().GetText())) {
        return false;
    }

    // If any of the ops don't exist or if no value is authored, then returning
    // identity values.

    if (!_HasTranslateOp() ||
        !_xformOps[_translateOpIndex].Get(translation, time)) 
    {
        *translation = GfVec3d(0.);
    }

    if (!_HasRotateOp() ||
        !_xformOps[_rotateOpIndex].Get(rotation, time)) 
    {
        *rotation = GfVec3f(0.);
    }

    if (!_HasScaleOp() ||
        !_xformOps[_scaleOpIndex].Get(scale, time)) 
    {
        *scale = GfVec3f(1.);
    }

    if (!_HasPivotOp() ||
        !_xformOps[_pivotOpIndex].Get(pivot, time)) 
    {
        *pivot = GfVec3f(0.);
    }

    *rotOrder = _HasRotateOp() ? _GetRotationOrderFromRotateOp(_xformOps[_rotateOpIndex])
                               : RotationOrderXYZ; 

    return true;
}

bool
UsdGeomXformCommonAPI::GetXformVectorsByAccumulation(
    GfVec3d* translation,
    GfVec3f* rotation,
    GfVec3f* scale,
    GfVec3f* pivot,
    UsdGeomXformCommonAPI::RotationOrder* rotOrder,
    const UsdTimeCode time)
{
    // If the xformOps are compatible as authored, then just use the usual
    // component extraction method.
    if (_IsCompatible()) {
        return GetXformVectors(translation, rotation, scale, pivot,
                               rotOrder, time);
    }

    // Note that we don't currently accumulate rotate ops, so we'll be looking
    // for one xformOp of a particular rotation type. Any xformOp order with
    // multiple rotates will be considered not to conform.
    const UsdGeomXformOp::Type rotateOpType = _GetRotateOpType(_xformOps, true);

    // The xformOp order expected by the common API is:
    // {Translate, Translate (pivot), Rotate, Scale, Translate (invert pivot)}
    // Depending on what we find in the xformOps (presence/absence of rotate,
    // scale(s), and number of inverse translates), we come up with an order
    // of common op types that we might be able to reduce the xformOps to.
    // We also maintain some named indices into that order which may be -1
    // (invalid) if that op is not present.
    int translateIndex, translatePivotIndex, rotateIndex,
        translateIdentityIndex, scaleIndex, translatePivotInvertIndex;
    vector<UsdGeomXformOp::Type> commonOpTypes =
        _GetCommonOpTypesForOpOrder(_xformOps,
            &translateIndex, &translatePivotIndex, &rotateIndex,
            &translateIdentityIndex, &scaleIndex, &translatePivotInvertIndex);

    // Keep a set of matrices that we'll accumulate the xformOp transforms into.
    vector<GfMatrix4d> commonOpMatrices(commonOpTypes.size(), GfMatrix4d(1.0));

    // Scan backwards through the xformOps and list of commonOpTypes
    // accumulating transforms as we go. We scan backwards so that we
    // accumulate the inverse pivot first and can then use that to determine
    // where to split the translates at the front between pivot and non-pivot.
    int xformOpIndex = _xformOps.size() - 1;
    int commonOpTypeIndex = commonOpTypes.size() - 1;

    while (xformOpIndex >= 0 && commonOpTypeIndex >= translateIndex) {
        const UsdGeomXformOp xformOp = _xformOps[xformOpIndex];
        UsdGeomXformOp::Type commonOpType = commonOpTypes[commonOpTypeIndex];

        if (xformOp.GetOpType() != commonOpType) {
            --commonOpTypeIndex;
            continue;
        }

        // The current op has the type we expect. Multiply its transform
        // into the results.
        commonOpMatrices[commonOpTypeIndex] *= xformOp.GetOpTransform(time);
        --xformOpIndex;

        if (commonOpType == rotateOpType) {
            // We currently do not allow rotate ops to accumulate, so as
            // soon as we match one, advance to the next commonOpType.
            --commonOpTypeIndex;
        } else if (commonOpType == UsdGeomXformOp::TypeTranslate) {
            if (xformOp.IsInverseOp()) {
                // We use the inverse-ness of translate ops to know when we
                // should move on to the next common op type. When we see an
                // inverse translate, we can assume that a valid order will
                // have its pair farther towards the front.
                --commonOpTypeIndex;
            } else if (commonOpTypeIndex == translatePivotIndex &&
                       _MatricesAreInverses(
                           commonOpMatrices[translatePivotIndex],
                           commonOpMatrices[translatePivotInvertIndex])) {
                // We've found a pair of pivot transforms, so we'll accumulate
                // the rest of the translates into regular translation.
                --commonOpTypeIndex;
            }
        }
    }

    bool reducible = true;

    if (xformOpIndex >= translateIndex) {
        // We didn't make it all the way through the xformOps, so there must
        // have been something in there that does not conform.
        reducible = false;
    }

    // Make sure that any translates between the rotate and scale ops
    // accumulated to identity.
    if (translateIdentityIndex >= 0 &&
            !_IsMatrixIdentity(commonOpMatrices[translateIdentityIndex])) {
        reducible = false;
    }

    // If all we saw while scanning were translates, then swap the accumulated
    // translation matrix from the "Translate (invert pivot)" position into the
    // "Translate" position.
    if (commonOpTypeIndex == translatePivotInvertIndex) {
        commonOpMatrices[translateIndex] = commonOpMatrices[commonOpTypeIndex];
        commonOpMatrices[commonOpTypeIndex] = GfMatrix4d(1.0);
    }

    // Verify that the translate pivot and inverse translate pivot are inverses
    // of each other. If there is no pivot, these should both still be identity.
    if (!_MatricesAreInverses(commonOpMatrices[translatePivotIndex],
                                 commonOpMatrices[translatePivotInvertIndex])) {
        reducible = false;
    }

    if (!reducible) {
        return GetXformVectors(translation, rotation, scale, pivot,
                               rotOrder, time);
    }

    if (translation) {
        *translation = commonOpMatrices[translateIndex].ExtractTranslation();
    }

    if (pivot) {
        GfVec3d result =
            commonOpMatrices[translatePivotIndex].ExtractTranslation();
        *pivot = GfVec3f(result[0], result[1], result[2]);
    }

    if (rotation) {
        if (rotateIndex >= 0) {
            GfRotation accumRot =
                commonOpMatrices[rotateIndex].ExtractRotation();
            GfVec3d result = accumRot.Decompose(
                GfVec3d::XAxis(), GfVec3d::YAxis(), GfVec3d::ZAxis());
            *rotation = GfVec3f(result[0], result[1], result[2]);
        } else {
            *rotation = GfVec3f(0.0, 0.0, 0.0);
        }
    }

    if (scale) {
        if (scaleIndex >= 0) {
            (*scale)[0] = commonOpMatrices[scaleIndex][0][0];
            (*scale)[1] = commonOpMatrices[scaleIndex][1][1];
            (*scale)[2] = commonOpMatrices[scaleIndex][2][2];
        } else {
            *scale = GfVec3f(1.0, 1.0, 1.0);
        }
    }

    if (rotOrder) {
        *rotOrder = _GetRotationOrderFromRotateOpType(rotateOpType);
    }

    return true;
}

bool
UsdGeomXformCommonAPI::GetResetXformStack() const
{
    return _xformable.GetResetXformStack();
}

bool
UsdGeomXformCommonAPI::SetResetXformStack(bool resetXformStack) const
{
    return _xformable.SetResetXformStack(resetXformStack);
}

static
UsdGeomXformOp::Type
_GetXformOpTypeForRotationOrder(UsdGeomXformCommonAPI::RotationOrder rotOrder)
{
    switch (rotOrder) {
        case UsdGeomXformCommonAPI::RotationOrderXYZ: 
            return UsdGeomXformOp::TypeRotateXYZ;
        case UsdGeomXformCommonAPI::RotationOrderXZY: 
            return UsdGeomXformOp::TypeRotateXZY;
        case UsdGeomXformCommonAPI::RotationOrderYXZ: 
            return UsdGeomXformOp::TypeRotateYXZ;
        case UsdGeomXformCommonAPI::RotationOrderYZX: 
            return UsdGeomXformOp::TypeRotateYZX;
        case UsdGeomXformCommonAPI::RotationOrderZXY: 
            return UsdGeomXformOp::TypeRotateZXY;
        case UsdGeomXformCommonAPI::RotationOrderZYX: 
            return UsdGeomXformOp::TypeRotateZYX;
        default:
            // Should never hit this.
            TF_CODING_ERROR("Invalid rotation order <%s>.",  
                TfEnum::GetName(rotOrder).c_str());
            // Default rotation order is XYZ.
            return UsdGeomXformOp::TypeRotateXYZ;
    }
}

static
TfToken
_GetRotateOpNameToken(const vector<UsdGeomXformOp> &ops)
{
    TF_FOR_ALL(it, ops) {
        if (_validRotateTypes->count(it->GetOpType()))
            return it->GetOpName();
    }
    return _tokens->xformOpRotateXYZ;
}

bool
UsdGeomXformCommonAPI::_ValidateAndComputeXformOpIndices(
    int *translateOpIndex,
    int *pivotOpIndex,
    int *rotateOpIndex,
    int *scaleOpIndex) const
{
    TRACE_FUNCTION();

    if (_xformOps.size() > 5)
        return false;

    // The expected order is:
    // {Translate, TranslatePivot, Rotate, Scale, InvTranslatePivot}
    TfTokenVector opNameTokens = {
        _tokens->xformOpTranslate,
        _tokens->xformOpTranslatePivot,
        _GetRotateOpNameToken(_xformOps),
        _tokens->xformOpScale,
        _tokens->xformOpInvTranslatePivot
    };

    typedef std::map<TfToken, int> XformOpToIndexMap;
    XformOpToIndexMap xformOpToIndexMap;
    // Initialize all indices to _InvalidIndex.
    TF_FOR_ALL(it, opNameTokens)
        xformOpToIndexMap[*it] = _InvalidIndex;

    for(size_t index = 0; index < _xformOps.size(); ++index) {
        const TfToken opName = _xformOps[index].GetOpName();
        XformOpToIndexMap::iterator it = xformOpToIndexMap.find(opName);
        if (it == xformOpToIndexMap.end()) {
            // Unknown opname. Hence, incompatible.
            return false;
        } else {
            it->second = index;
        }
    }
    
    // Verify that the xformOps that do exist are in a compatible order.
    int lastNonNegativeIndex = _InvalidIndex;
    TF_FOR_ALL(opNameIt, opNameTokens) {
        int opIndex = xformOpToIndexMap[*opNameIt];
        if (opIndex != _InvalidIndex) {    
            TF_VERIFY(opIndex != lastNonNegativeIndex);

            if (opIndex < lastNonNegativeIndex)
                return false;

            lastNonNegativeIndex = opIndex;
        }
    }

    // Verify that translate pivot and inverse translate pivot are either both 
    // present or both absent.
    int hasPivotOp = (xformOpToIndexMap[opNameTokens[1]] != _InvalidIndex);
    int hasInversePivotOp = (xformOpToIndexMap[opNameTokens[4]] != _InvalidIndex);
    if (hasPivotOp != hasInversePivotOp)
        return false;

    if (translateOpIndex)
        *translateOpIndex = xformOpToIndexMap[opNameTokens[0]];

    if (pivotOpIndex)
        *pivotOpIndex = xformOpToIndexMap[opNameTokens[1]];

    if (rotateOpIndex)
        *rotateOpIndex = xformOpToIndexMap[opNameTokens[2]];
    
    if (scaleOpIndex)
        *scaleOpIndex = xformOpToIndexMap[opNameTokens[3]];

    return true;
}

bool
UsdGeomXformCommonAPI::_ComputeOpIndices()
{
    int translateOpIndex, pivotOpIndex, rotateOpIndex, scaleOpIndex;
    if (!_ValidateAndComputeXformOpIndices(&translateOpIndex, &pivotOpIndex,
                                              &rotateOpIndex, &scaleOpIndex)) {
        return false;
    }

    _translateOpIndex = translateOpIndex;
    _pivotOpIndex = pivotOpIndex;
    _rotateOpIndex = rotateOpIndex;
    _scaleOpIndex = scaleOpIndex;

    _computedOpIndices = true;

    return true;
}

bool
UsdGeomXformCommonAPI::_VerifyCompatibility()
{
    // Can't set xform op on an xformable with incompatible schema.
    if (!_IsCompatible())
        return false;

    if (!_computedOpIndices) 
        _ComputeOpIndices();

    return TF_VERIFY(_computedOpIndices);
}

bool 
UsdGeomXformCommonAPI::SetTranslate(
    const GfVec3d &translation, 
    const UsdTimeCode time/*=UsdTimeCode::Default()*/)
{
    // Can't set translate on an xformable with incompatible schema.
    if (!_VerifyCompatibility()) {
        TF_WARN("XformCommonAPI: Attempted to SetTranslate on an incompatible "
            "xformable <%s>.", _xformable.GetPath().GetText());    
        return false;
    }

    if (_HasTranslateOp()) {
        if (!TF_VERIFY(static_cast<size_t>(_translateOpIndex) < _xformOps.size()))
            return false;
        return _xformOps[_translateOpIndex].Set(translation, time);
    }

    UsdGeomXformOp translateOp = _xformable.AddTranslateOp();
    if (!TF_VERIFY(translateOp))
        return false;

    translateOp.Set(translation, time);

    _translateOpIndex = 0;

    // Bump up indices of other ops that come after this one.
    if (_HasPivotOp())
        ++_pivotOpIndex;
    if (_HasRotateOp())
        ++_rotateOpIndex;
    if (_HasScaleOp())
        ++_scaleOpIndex;

    _xformOps.insert(_xformOps.begin(), translateOp);
    
    // Preserve the existing resetsXformStack
    return _xformable.SetXformOpOrder(_xformOps, GetResetXformStack());
}

bool 
UsdGeomXformCommonAPI::SetPivot(
    const GfVec3f &pivot, 
    const UsdTimeCode time/*=UsdTimeCode::Default()*/)
{
    // Can't set pivot on an xformable with incompatible schema.
    if (!_VerifyCompatibility()) {
        TF_WARN("XformCommonAPI: Attempted to SetPivot on an incompatible "
            "xformable <%s>.", _xformable.GetPath().GetText());    
        return false;
    }

    if (_HasPivotOp()) {
        if (!TF_VERIFY(static_cast<size_t>(_pivotOpIndex) < _xformOps.size()))
            return false;
        return _xformOps[_pivotOpIndex].Set(pivot, time);
    }

    // Add scale-rotate pivot.
    UsdGeomXformOp pivotOp = _xformable.AddTranslateOp(
        UsdGeomXformOp::PrecisionFloat, /* opSuffix */ _tokens->pivot);
    if (!TF_VERIFY(pivotOp))
        return false;

    pivotOp.Set(pivot, time);

    // The pivot op comes after the translate op.
    _pivotOpIndex = 0;
    if (_HasTranslateOp())
        ++_pivotOpIndex;

    // Bump up indices of other ops that come after this one.
    if (_HasRotateOp())
        ++_rotateOpIndex;
    if (_HasScaleOp())
        ++_scaleOpIndex;

    _xformOps.insert(_xformOps.begin() + _pivotOpIndex, pivotOp);

    // Add inverse translate pivot. This is always inserted at the end.
    UsdGeomXformOp invPivotOp = _xformable.AddTranslateOp(
        UsdGeomXformOp::PrecisionFloat, _tokens->pivot, /* isInverseOp */true);
    _xformOps.insert(_xformOps.end(), invPivotOp);

    return _xformable.SetXformOpOrder(_xformOps, GetResetXformStack());
}

bool 
UsdGeomXformCommonAPI::SetRotate(
    const GfVec3f &rotation, 
    UsdGeomXformCommonAPI::RotationOrder rotOrder/*=RotationOrderXYZ*/,
    const UsdTimeCode time/*=UsdTimeCode::Default()*/)
{
    // Can't set rotate on an xformable with incompatible schema.
    if (!_VerifyCompatibility()) {
        TF_WARN("XformCommonAPI: Attempted to SetRotate on an incompatible "
            "xformable <%s>.", _xformable.GetPath().GetText());    
        return false;
    }

    if (_HasRotateOp()) {
        if (!TF_VERIFY(static_cast<size_t>(_rotateOpIndex) < _xformOps.size()))
            return false;

        RotationOrder existingRotOrder =
            _GetRotationOrderFromRotateOp(_xformOps[_rotateOpIndex]);
        if (existingRotOrder != rotOrder) {
            TF_CODING_ERROR("Rotation order mismatch on prim <%s>. (%s != %s)",
                _xformable.GetPath().GetText(), 
                TfEnum::GetName(rotOrder).c_str(),
                TfEnum::GetName(existingRotOrder).c_str());
            return false;
        }

        return _xformOps[_rotateOpIndex].Set(rotation, time);
    }

    UsdGeomXformOp rotateOp = _xformable.AddXformOp(
        _GetXformOpTypeForRotationOrder(rotOrder), 
        UsdGeomXformOp::PrecisionFloat);
    if (!TF_VERIFY(rotateOp))
        return false;

    rotateOp.Set(rotation, time);

    // The rotate op comes after the translate op and the pivot op.
    _rotateOpIndex = 0;
    if (_HasTranslateOp())
        ++_rotateOpIndex;
    if (_HasPivotOp())
        ++_rotateOpIndex;

    // Bump up indices of other ops that come after this one.
    if (_HasScaleOp())
        ++_scaleOpIndex;

    _xformOps.insert(_xformOps.begin() + _rotateOpIndex, rotateOp);

    return _xformable.SetXformOpOrder(_xformOps, GetResetXformStack());
}

bool 
UsdGeomXformCommonAPI::SetScale(
    const GfVec3f &scale, 
    const UsdTimeCode time/*=UsdTimeCode::Default()*/)
{
    // Can't set scale on an xformable with incompatible schema.
    if (!_VerifyCompatibility()) {
        TF_WARN("XformCommonAPI: Attempted to SetScale on an incompatible "
            "xformable <%s>.", _xformable.GetPath().GetText());    
        return false;
    }

    if (_HasScaleOp()) {
        if (!TF_VERIFY(static_cast<size_t>(_scaleOpIndex) < _xformOps.size()))
            return false;

        return _xformOps[_scaleOpIndex].Set(scale, time);
    }

    UsdGeomXformOp scaleOp = _xformable.AddScaleOp();
    if (!TF_VERIFY(scaleOp))
        return false;

    scaleOp.Set(scale, time);

    // The scale op comes after the translateOp, pivotOp and scaleOp.
    _scaleOpIndex = 0;
    if (_HasTranslateOp())
        ++_scaleOpIndex;
    if (_HasPivotOp())
        ++_scaleOpIndex;
    if (_HasRotateOp())
        ++_scaleOpIndex;

    _xformOps.insert(_xformOps.begin() + _scaleOpIndex, scaleOp);

    return _xformable.SetXformOpOrder(_xformOps, GetResetXformStack());
}

PXR_NAMESPACE_CLOSE_SCOPE

