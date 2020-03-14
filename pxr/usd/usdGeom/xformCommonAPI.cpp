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
#include "pxr/usd/usdGeom/xformCommonAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomXformCommonAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

TF_DEFINE_PRIVATE_TOKENS(
    _schemaTokens,
    (XformCommonAPI)
);

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
    return UsdGeomXformCommonAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaType UsdGeomXformCommonAPI::_GetSchemaType() const {
    return UsdGeomXformCommonAPI::schemaType;
}

/* static */
const TfType &
UsdGeomXformCommonAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomXformCommonAPI>();
    return tfType;
}

/* static */
bool 
UsdGeomXformCommonAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomXformCommonAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdGeomXformCommonAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdAPISchemaBase::GetSchemaAttributeNames(true);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include "pxr/base/gf/rotation.h"
#include "pxr/base/trace/trace.h"

#include <map>

using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(UsdGeomXformCommonAPI::RotationOrderXYZ, "XYZ");
    TF_ADD_ENUM_NAME(UsdGeomXformCommonAPI::RotationOrderXZY, "XZY");
    TF_ADD_ENUM_NAME(UsdGeomXformCommonAPI::RotationOrderYXZ, "YXZ");
    TF_ADD_ENUM_NAME(UsdGeomXformCommonAPI::RotationOrderYZX, "YZX");
    TF_ADD_ENUM_NAME(UsdGeomXformCommonAPI::RotationOrderZXY, "ZXY");
    TF_ADD_ENUM_NAME(UsdGeomXformCommonAPI::RotationOrderZYX, "ZYX");

    TF_ADD_ENUM_NAME(UsdGeomXformCommonAPI::OpTranslate);
    TF_ADD_ENUM_NAME(UsdGeomXformCommonAPI::OpRotate);
    TF_ADD_ENUM_NAME(UsdGeomXformCommonAPI::OpScale);
    TF_ADD_ENUM_NAME(UsdGeomXformCommonAPI::OpPivot);
};

static
bool
_GetCommonXformOps(
    const UsdGeomXformable& xformable,
    UsdGeomXformOp* translateOp=nullptr,
    UsdGeomXformOp* pivotOp=nullptr,
    UsdGeomXformOp* rotateOp=nullptr,
    UsdGeomXformOp* scaleOp=nullptr,
    UsdGeomXformOp* pivotInvOp=nullptr,
    bool* resetXformStack=nullptr);

static
UsdGeomXformCommonAPI::Ops
_GetOrAddCommonXformOps(
    const UsdGeomXformable& xformable,
    const UsdGeomXformCommonAPI::RotationOrder* rotOrder,
    bool createTranslate,
    bool createPivot,
    bool createRotate,
    bool createScale);

/* virtual */
bool 
UsdGeomXformCommonAPI::_IsCompatible() const
{
    if (!UsdAPISchemaBase::_IsCompatible()) {
        return false;
    }

    UsdGeomXformable xformable(GetPrim());
    if (!xformable) {
        return false;
    }

    return _GetCommonXformOps(xformable);
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
    const UsdTimeCode time) const
{
    // The call below will check rotation order compatibility before any data
    // is authored.
    const Ops ops = CreateXformOps(
        rotOrder,
        OpTranslate, OpRotate, OpScale, OpPivot);
    if (!ops.translateOp || !ops.rotateOp || !ops.scaleOp || !ops.pivotOp) {
        return false;
    }

    return ops.translateOp.Set(translation, time) &&
           ops.rotateOp.Set(rotation, time) &&
           ops.scaleOp.Set(scale, time) &&
           ops.pivotOp.Set(pivot, time);
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

static constexpr
bool
_IsThreeAxisRotateOpType(UsdGeomXformOp::Type opType)
{
    static_assert(
        UsdGeomXformOp::TypeRotateZYX - UsdGeomXformOp::TypeRotateXYZ == 5,
        "Exactly six three-axis rotate op types");
    return opType >= UsdGeomXformOp::TypeRotateXYZ &&
           opType <= UsdGeomXformOp::TypeRotateZYX;
}

static constexpr
bool
_IsRotateOpType(UsdGeomXformOp::Type opType)
{
    static_assert(
        UsdGeomXformOp::TypeRotateZYX - UsdGeomXformOp::TypeRotateX == 8,
        "Exactly nine rotate op types");
    return opType >= UsdGeomXformOp::TypeRotateX &&
           opType <= UsdGeomXformOp::TypeRotateZYX;
}

static_assert(_IsRotateOpType(UsdGeomXformOp::TypeRotateX) &&
             !_IsThreeAxisRotateOpType(UsdGeomXformOp::TypeRotateX), "");
static_assert(_IsRotateOpType(UsdGeomXformOp::TypeRotateY) &&
             !_IsThreeAxisRotateOpType(UsdGeomXformOp::TypeRotateY), "");
static_assert(_IsRotateOpType(UsdGeomXformOp::TypeRotateZ) &&
             !_IsThreeAxisRotateOpType(UsdGeomXformOp::TypeRotateZ), "");
static_assert(_IsRotateOpType(UsdGeomXformOp::TypeRotateXYZ) &&
              _IsThreeAxisRotateOpType(UsdGeomXformOp::TypeRotateXYZ), "");
static_assert(_IsRotateOpType(UsdGeomXformOp::TypeRotateXZY) &&
              _IsThreeAxisRotateOpType(UsdGeomXformOp::TypeRotateXZY), "");
static_assert(_IsRotateOpType(UsdGeomXformOp::TypeRotateYXZ) &&
              _IsThreeAxisRotateOpType(UsdGeomXformOp::TypeRotateYXZ), "");
static_assert(_IsRotateOpType(UsdGeomXformOp::TypeRotateYZX) &&
              _IsThreeAxisRotateOpType(UsdGeomXformOp::TypeRotateYZX), "");
static_assert(_IsRotateOpType(UsdGeomXformOp::TypeRotateZXY) &&
              _IsThreeAxisRotateOpType(UsdGeomXformOp::TypeRotateZXY), "");
static_assert(_IsRotateOpType(UsdGeomXformOp::TypeRotateZYX) &&
              _IsThreeAxisRotateOpType(UsdGeomXformOp::TypeRotateZYX), "");

static_assert(!_IsRotateOpType(UsdGeomXformOp::TypeTranslate) &&
              !_IsThreeAxisRotateOpType(UsdGeomXformOp::TypeTranslate), "");
static_assert(!_IsRotateOpType(UsdGeomXformOp::TypeScale) &&
              !_IsThreeAxisRotateOpType(UsdGeomXformOp::TypeScale), "");

static
UsdGeomXformOp::Type
_GetRotateOpType(const vector<UsdGeomXformOp>& ops)
{
    for (const UsdGeomXformOp& op : ops) {
        if (_IsRotateOpType(op.GetOpType())) {
            return op.GetOpType();
        }
    }
    return UsdGeomXformOp::TypeRotateXYZ;
}

/* static */
UsdGeomXformOp::Type
UsdGeomXformCommonAPI::ConvertRotationOrderToOpType(
    RotationOrder rotOrder)
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

/* static */
UsdGeomXformCommonAPI::RotationOrder
UsdGeomXformCommonAPI::ConvertOpTypeToRotationOrder(
    UsdGeomXformOp::Type opType)
{
    switch (opType) {
        case UsdGeomXformOp::TypeRotateXYZ:
            return UsdGeomXformCommonAPI::RotationOrderXYZ;
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
        default:
            TF_CODING_ERROR("'%s' is not a three-axis rotate op type",
                TfEnum::GetName(opType).c_str());
            // Default rotation order is XYZ.
            return UsdGeomXformCommonAPI::RotationOrderXYZ;
    }
}

/* static */
bool
UsdGeomXformCommonAPI::CanConvertOpTypeToRotationOrder(
    UsdGeomXformOp::Type opType)
{
    // _IsThreeAxisRotateOpType must be a separate function because it is
    // constexpr (so that we can static_assert) but we want to keep the
    // definition out of the header (constexpr implies inline, so it needs to be
    // defined where it's declared).
    return _IsThreeAxisRotateOpType(opType);
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
        if (_IsRotateOpType(it->GetOpType())) {
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
    const UsdTimeCode time) const
{
    if (!TF_VERIFY(translation && rotation && scale && pivot && rotOrder)) {
        return false;
    }

    UsdGeomXformable xformable(GetPrim());

    // Handle incompatible xform case first.
    // It's ok for an xform to be incompatible when extracting xform vectors. 
    UsdGeomXformOp t, p, r, s;
    if (!_GetCommonXformOps(xformable, &t, &p, &r, &s)) {
        GfMatrix4d localXform(1.);

        // Do we want to be able to use a UsdGeomXformCache for this?
        bool resetsXformStack = false;
        xformable.GetLocalTransformation(&localXform, &resetsXformStack, time);

        // We don't process (or return) resetsXformStack here. It is up to the 
        // clients to call GetResetXformStack() and process it suitably.
        _ConvertMatrixToComponents(localXform, translation, rotation, scale);

        *pivot = GfVec3f(0.,0.,0.);

        *rotOrder = RotationOrderXYZ;

        return true;
    }

    // If any of the ops don't exist or if no value is authored, then returning
    // identity values.

    if (!t || !t.Get(translation, time)) {
        *translation = GfVec3d(0.);
    }

    if (!r || !r.Get(rotation, time)) {
        *rotation = GfVec3f(0.);
    }

    if (!s || !s.Get(scale, time)) {
        *scale = GfVec3f(1.);
    }

    if (!p || !p.Get(pivot, time)) {
        *pivot = GfVec3f(0.);
    }

    *rotOrder = r ? ConvertOpTypeToRotationOrder(r.GetOpType())
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
    const UsdTimeCode time) const
{
    // If the xformOps are compatible as authored, then just use the usual
    // component extraction method.
    if (_IsCompatible()) {
        return GetXformVectors(translation, rotation, scale, pivot,
                               rotOrder, time);
    }

    UsdGeomXformable xformable(GetPrim());
    bool unusedResetXformStack;
    const std::vector<UsdGeomXformOp> xformOps =
        xformable.GetOrderedXformOps(&unusedResetXformStack);

    // Note that we don't currently accumulate rotate ops, so we'll be looking
    // for one xformOp of a particular rotation type. Any xformOp order with
    // multiple rotates will be considered not to conform.
    const UsdGeomXformOp::Type rotateOpType = _GetRotateOpType(xformOps);

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
        _GetCommonOpTypesForOpOrder(xformOps,
            &translateIndex, &translatePivotIndex, &rotateIndex,
            &translateIdentityIndex, &scaleIndex, &translatePivotInvertIndex);

    // Keep a set of matrices that we'll accumulate the xformOp transforms into.
    vector<GfMatrix4d> commonOpMatrices(commonOpTypes.size(), GfMatrix4d(1.0));

    // Scan backwards through the xformOps and list of commonOpTypes
    // accumulating transforms as we go. We scan backwards so that we
    // accumulate the inverse pivot first and can then use that to determine
    // where to split the translates at the front between pivot and non-pivot.
    int xformOpIndex = xformOps.size() - 1;
    int commonOpTypeIndex = commonOpTypes.size() - 1;

    while (xformOpIndex >= 0 && commonOpTypeIndex >= translateIndex) {
        const UsdGeomXformOp xformOp = xformOps[xformOpIndex];
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
        *rotOrder = CanConvertOpTypeToRotationOrder(rotateOpType)
            ? ConvertOpTypeToRotationOrder(rotateOpType)
            : UsdGeomXformCommonAPI::RotationOrderXYZ;
    }

    return true;
}

bool
UsdGeomXformCommonAPI::GetResetXformStack() const
{
    return UsdGeomXformable(GetPrim()).GetResetXformStack();
}

bool
UsdGeomXformCommonAPI::SetResetXformStack(bool resetXformStack) const
{
    return UsdGeomXformable(GetPrim()).SetResetXformStack(resetXformStack);
}

// Retrieves the XformCommonAPI-compatible component ops for the given xformable
// prim. Returns true if the ops are in a compatible order or false if they're
// in an incompatible order. Populates the non-null out-parameters with the
// requested ops. (If an op does not exist, the corresponding out-parameter is
// populated with an invalid UsdGeomXformOp.) If resetXformStack is non-null,
// populates it with the value of UsdGeomXformable::GetResetXformStack().
static
bool
_GetCommonXformOps(
    const UsdGeomXformable& xformable,
    UsdGeomXformOp* translateOp,
    UsdGeomXformOp* pivotOp,
    UsdGeomXformOp* rotateOp,
    UsdGeomXformOp* scaleOp,
    UsdGeomXformOp* pivotInvOp,
    bool* resetXformStack)
{
    TRACE_FUNCTION();

    bool tempResetXformStack;
    std::vector<UsdGeomXformOp> xformOps =
        xformable.GetOrderedXformOps(&tempResetXformStack);
    if (xformOps.size() > 5)
        return false;

    // The expected order is:
    // ["xformOp:translate", "xformOp:translate:pivot", "xformOp:rotateABC",
    //  "xformOp:scale", "!invert!xformOp:translate:pivot"]
    auto it = xformOps.begin();

    // This holds the computed attribute name tokens so that we can avoid
    // hard-coding them.
    // The name for the rotate op is not computed here because it can vary.
    static const struct {
        TfToken translate = UsdGeomXformOp::GetOpName(
            UsdGeomXformOp::TypeTranslate);
        TfToken pivot = UsdGeomXformOp::GetOpName(
            UsdGeomXformOp::TypeTranslate, UsdGeomTokens->pivot);
        TfToken scale = UsdGeomXformOp::GetOpName(
            UsdGeomXformOp::TypeScale);
    } attrNames;

    // Search one-by-one for the ops in the correct order.
    // We can skip ops in the "expected" order (that is, all the common ops are
    // optional) but we can't skip ops in the "actual" order (that is, extra ops
    // aren't allowed).
    //
    // Note, in checks below, avoid using UsdGeomXformOp::GetOpName() because
    // it will construct strings in the case of an inverted op.
    UsdGeomXformOp t;
    if (it != xformOps.end() &&
            it->GetName() == attrNames.translate &&
            !it->IsInverseOp()) {
        t = std::move(*it);
        ++it;
    }

    UsdGeomXformOp p;
    if (it != xformOps.end() &&
            it->GetName() == attrNames.pivot &&
            !it->IsInverseOp()) {
        p = std::move(*it);
        ++it;
    }

    UsdGeomXformOp r;
    if (it != xformOps.end() &&
            UsdGeomXformCommonAPI::CanConvertOpTypeToRotationOrder(
                it->GetOpType()) &&
            !it->IsInverseOp()) {
        r = std::move(*it);
        ++it;
    }

    UsdGeomXformOp s;
    if (it != xformOps.end() &&
            it->GetName() == attrNames.scale &&
            !it->IsInverseOp()) {
        s = std::move(*it);
        ++it;
    }

    UsdGeomXformOp pInv;
    if (it != xformOps.end() &&
            it->GetName() == attrNames.pivot &&
            it->IsInverseOp()) {
        pInv = std::move(*it);
        ++it;
    }

    // If we did not reach the end of the xformOps vector, then there were
    // extra ops that did not match any of the expected ops.
    // This means that the xformOps vector isn't XformCommonAPI-compatible.
    if (it != xformOps.end()) {
        return false;
    }

    // Verify that translate pivot and inverse translate pivot are either both 
    // present or both absent.
    if ((bool) p != (bool) pInv) {
        return false;
    }

    if (translateOp) {
        *translateOp = std::move(t);
    }

    if (pivotOp) {
        *pivotOp = std::move(p);
    }

    if (rotateOp) {
        *rotateOp = std::move(r);
    }
    
    if (scaleOp) {
        *scaleOp = std::move(s);
    }

    if (pivotInvOp) {
        *pivotInvOp = std::move(pInv);
    }

    if (resetXformStack) {
        *resetXformStack = tempResetXformStack;
    }

    return true;
}

// Similar to _GetCommonXformOps, except also adds ops for any non-null out
// parameter whose op does not yet exist. If this returns true, then it
// guarantees that every op returned in an out-parameter is valid.
//
// When creating a rotate op and rotOrder is specified, then it will be used
// to choose the rotate op type (or to validate the existing rotate op type).
// If rotOrder is not specified, then a rotateXYZ op will be created (or
// any existing three-axis rotate returned).
static
UsdGeomXformCommonAPI::Ops
_GetOrAddCommonXformOps(
    const UsdGeomXformable& xformable,
    const UsdGeomXformCommonAPI::RotationOrder* rotOrder,
    bool createTranslate,
    bool createPivot,
    bool createRotate,
    bool createScale)
{
    TRACE_FUNCTION();

    // Can't get or add ops on an xformable with incompatible schema.
    UsdGeomXformOp t, p, r, s, pInv;
    bool resetXformStack = false;
    if (!_GetCommonXformOps(
            xformable, &t, &p, &r, &s, &pInv, &resetXformStack)) {
        TF_WARN("Could not determine xform ops for incompatible xformable <%s>",
                xformable.GetPath().GetText());
        return UsdGeomXformCommonAPI::Ops();
    }

    // If creating the rotate op and the rotate op already exists, we must check
    // that the existing rotation order matches the requested rotation order.
    // We do this first so that we can early-exit without modifying the xform
    // op order if we encounter an error.
    if (createRotate && rotOrder && r) {
        const UsdGeomXformCommonAPI::RotationOrder existingRotOrder =
            UsdGeomXformCommonAPI::ConvertOpTypeToRotationOrder(r.GetOpType());
        if (existingRotOrder != *rotOrder) {
            TF_CODING_ERROR("Rotation order mismatch on prim <%s> (%s != %s)",
                xformable.GetPath().GetText(), 
                TfEnum::GetName(*rotOrder).c_str(),
                TfEnum::GetName(existingRotOrder).c_str());
            return UsdGeomXformCommonAPI::Ops();
        }
    }

    // Add ops if they were requested but the ops do not yet exist.
    bool addedOps = false;
    if (createTranslate && !t) {
        addedOps = true;
        t = xformable.AddTranslateOp();
        if (!TF_VERIFY(t)) {
            return UsdGeomXformCommonAPI::Ops();
        }
    }
    if (createPivot && !p) {
        addedOps = true;
        p = xformable.AddTranslateOp(
            UsdGeomXformOp::PrecisionFloat, UsdGeomTokens->pivot);
        pInv = xformable.AddTranslateOp(
            UsdGeomXformOp::PrecisionFloat, UsdGeomTokens->pivot,
            /* isInverseOp */ true);
        if (!TF_VERIFY(p && pInv)) {
            return UsdGeomXformCommonAPI::Ops();
        }
    }
    if (createRotate && !r) {
        addedOps = true;
        const UsdGeomXformOp::Type rotateOpType = rotOrder
            ? UsdGeomXformCommonAPI::ConvertRotationOrderToOpType(*rotOrder)
            : UsdGeomXformOp::TypeRotateXYZ;
        r = xformable.AddXformOp(
            rotateOpType, 
            UsdGeomXformOp::PrecisionFloat);
        if (!TF_VERIFY(r)) {
            return UsdGeomXformCommonAPI::Ops();
        }
    }
    if (createScale && !s) {
        addedOps = true;
        s = xformable.AddScaleOp();
        if (!TF_VERIFY(s)) {
            return UsdGeomXformCommonAPI::Ops();
        }
    }

    // Only update the xform op order if we had to add new ops.
    if (addedOps) {
        std::vector<UsdGeomXformOp> newXformOps;
        if (t) newXformOps.push_back(t);
        if (p) newXformOps.push_back(p);
        if (r) newXformOps.push_back(r);
        if (s) newXformOps.push_back(s);
        if (pInv) newXformOps.push_back(pInv);
        xformable.SetXformOpOrder(newXformOps, resetXformStack);
    }

    return UsdGeomXformCommonAPI::Ops {
        std::move(t),
        std::move(p),
        std::move(r),
        std::move(s),
        std::move(pInv),
    };
}

bool 
UsdGeomXformCommonAPI::SetTranslate(
    const GfVec3d &translation, 
    const UsdTimeCode time/*=UsdTimeCode::Default()*/) const
{
    // Can't set translate on an xformable with incompatible schema.
    Ops ops = CreateXformOps(OpTranslate);
    if (!ops.translateOp) {
        return false;
    }

    return ops.translateOp.Set(translation, time);
}

bool 
UsdGeomXformCommonAPI::SetPivot(
    const GfVec3f &pivot, 
    const UsdTimeCode time/*=UsdTimeCode::Default()*/) const
{
    // Can't set pivot on an xformable with incompatible schema.
    Ops ops = CreateXformOps(OpPivot);
    if (!ops.pivotOp) {
        return false;
    }

    return ops.pivotOp.Set(pivot, time);
}

bool 
UsdGeomXformCommonAPI::SetRotate(
    const GfVec3f &rotation, 
    UsdGeomXformCommonAPI::RotationOrder rotOrder/*=RotationOrderXYZ*/,
    const UsdTimeCode time/*=UsdTimeCode::Default()*/) const
{
    // Can't set rotate on an xformable with incompatible schema.
    Ops ops = CreateXformOps(rotOrder, OpRotate);
    if (!ops.rotateOp) {
        return false;
    }

    return ops.rotateOp.Set(rotation, time);
}

bool 
UsdGeomXformCommonAPI::SetScale(
    const GfVec3f &scale, 
    const UsdTimeCode time/*=UsdTimeCode::Default()*/) const
{
    // Can't set scale on an xformable with incompatible schema.
    Ops ops = CreateXformOps(OpScale);
    if (!ops.scaleOp) {
        return false;
    }

    return ops.scaleOp.Set(scale, time);
}

UsdGeomXformCommonAPI::Ops
UsdGeomXformCommonAPI::CreateXformOps(
    RotationOrder rotOrder,
    OpFlags op1,
    OpFlags op2,
    OpFlags op3,
    OpFlags op4) const
{
    UsdGeomXformable xformable(GetPrim());
    if (!xformable) {
        return UsdGeomXformCommonAPI::Ops();
    }

    const auto flags = op1 | op2 | op3 | op4;
    return _GetOrAddCommonXformOps(
        xformable,
        &rotOrder,
        flags & OpTranslate,
        flags & OpPivot,
        flags & OpRotate,
        flags & OpScale);
}

UsdGeomXformCommonAPI::Ops
UsdGeomXformCommonAPI::CreateXformOps(
    OpFlags op1,
    OpFlags op2,
    OpFlags op3,
    OpFlags op4) const
{
    UsdGeomXformable xformable(GetPrim());
    if (!xformable) {
        return UsdGeomXformCommonAPI::Ops();
    }

    const auto flags = op1 | op2 | op3 | op4;
    return _GetOrAddCommonXformOps(
        xformable,
        nullptr,
        flags & OpTranslate,
        flags & OpPivot,
        flags & OpRotate,
        flags & OpScale);
}

/* static */
GfMatrix4d
UsdGeomXformCommonAPI::GetRotationTransform(
    const GfVec3f &rotation,
    const UsdGeomXformCommonAPI::RotationOrder rotationOrder)
{
    const UsdGeomXformOp::Type rotateOpType =
        UsdGeomXformCommonAPI::ConvertRotationOrderToOpType(rotationOrder);
    return UsdGeomXformOp::GetOpTransform(rotateOpType, VtValue(rotation));
}

PXR_NAMESPACE_CLOSE_SCOPE

