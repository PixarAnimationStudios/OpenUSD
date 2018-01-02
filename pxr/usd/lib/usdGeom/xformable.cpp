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
#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomXformable,
        TfType::Bases< UsdGeomImageable > >();
    
}

/* virtual */
UsdGeomXformable::~UsdGeomXformable()
{
}

/* static */
UsdGeomXformable
UsdGeomXformable::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomXformable();
    }
    return UsdGeomXformable(stage->GetPrimAtPath(path));
}


/* static */
const TfType &
UsdGeomXformable::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomXformable>();
    return tfType;
}

/* static */
bool 
UsdGeomXformable::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomXformable::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomXformable::GetXformOpOrderAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->xformOpOrder);
}

UsdAttribute
UsdGeomXformable::CreateXformOpOrderAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->xformOpOrder,
                       SdfValueTypeNames->TokenArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

namespace {
static inline TfTokenVector
_ConcatenateAttributeNames(const TfTokenVector& left,const TfTokenVector& right)
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());
    result.insert(result.end(), right.begin(), right.end());
    return result;
}
}

/*static*/
const TfTokenVector&
UsdGeomXformable::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->xformOpOrder,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdGeomImageable::GetSchemaAttributeNames(true),
            localNames);

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

#include "pxr/base/tf/envSetting.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (transform)
    ((invertPrefix, "!invert!"))
);

TF_DEFINE_ENV_SETTING(
    USD_READ_OLD_STYLE_TRANSFORM, false, "Whether xform reading code should "
        "consider old-style transform attribute values if they're available.");

using std::vector;

TF_MAKE_STATIC_DATA(GfMatrix4d, _IDENTITY) {
    *_IDENTITY = GfMatrix4d(1.0);
}

bool 
UsdGeomXformable::_GetXformOpOrderValue(
    VtTokenArray *xformOpOrder,
    bool *hasAuthoredValue) const
{
    UsdAttribute xformOpOrderAttr = GetXformOpOrderAttr();
    if (!xformOpOrderAttr)
        return false;

    if (hasAuthoredValue)
        *hasAuthoredValue = xformOpOrderAttr.HasAuthoredValueOpinion();

    xformOpOrderAttr.Get(xformOpOrder, UsdTimeCode::Default());
    return true;
}

UsdGeomXformOp 
UsdGeomXformable::AddXformOp(
    UsdGeomXformOp::Type const opType, 
    UsdGeomXformOp::Precision const precision, 
    TfToken const &opSuffix, 
    bool isInverseOp) const
{
    VtTokenArray xformOpOrder;
    _GetXformOpOrderValue(&xformOpOrder);

    // Check if the xformOp we're about to add already exists in xformOpOrder
    TfToken opName = UsdGeomXformOp::GetOpName(opType, opSuffix, isInverseOp);
    VtTokenArray::iterator it = std::find(xformOpOrder.begin(), 
        xformOpOrder.end(), opName);
    if (it != xformOpOrder.end()) {
        TF_CODING_ERROR("The xformOp '%s' already exists in xformOpOrder [%s].",
            opName.GetText(), TfStringify(xformOpOrder).c_str());
        return UsdGeomXformOp();
    }

    TfToken const &xformOpAttrName = UsdGeomXformOp::GetOpName(opType, opSuffix);
    UsdGeomXformOp result;
    if (UsdAttribute xformOpAttr = GetPrim().GetAttribute(xformOpAttrName)) {
        // Check if the attribute's typeName has the requested precision level.
        UsdGeomXformOp::Precision existingPrecision = 
            UsdGeomXformOp::GetPrecisionFromValueTypeName(
                xformOpAttr.GetTypeName());

        if (existingPrecision != precision) {
            TF_CODING_ERROR("XformOp <%s> has typeName '%s' which does not "
                            "match the requested precision '%s'. Proceeding to "
                            "use existing typeName / precision.",
                            xformOpAttr.GetPath().GetText(),
                            xformOpAttr.GetTypeName().GetAsToken().GetText(),
                            TfEnum::GetName(precision).c_str());
        }

        result = UsdGeomXformOp(xformOpAttr, isInverseOp);
    } else {
        result = UsdGeomXformOp(GetPrim(), opType, precision, opSuffix, 
                                isInverseOp);
    }

    if (result) {
        xformOpOrder.push_back(result.GetOpName());
        CreateXformOpOrderAttr().Set(xformOpOrder);
    } else {
        TF_CODING_ERROR("Unable to add xform op of type %s and precision %s on "
            "prim at path <%s>. opSuffix=%s, isInverseOp=%d", 
            TfEnum::GetName(opType).c_str(), TfEnum::GetName(precision).c_str(), 
            GetPath().GetText(), opSuffix.GetText(), isInverseOp);
        return UsdGeomXformOp();
    }

    return result;
}

UsdGeomXformOp 
UsdGeomXformable::AddTranslateOp(UsdGeomXformOp::Precision const precision,
    TfToken const &opSuffix, bool isInverseOp) const
{
    return AddXformOp(UsdGeomXformOp::TypeTranslate, precision, opSuffix,
                      isInverseOp);
}

UsdGeomXformOp 
UsdGeomXformable::AddScaleOp(UsdGeomXformOp::Precision const precision,
    TfToken const &opSuffix, bool isInverseOp) const
{
    return AddXformOp(UsdGeomXformOp::TypeScale, precision, opSuffix, 
                      isInverseOp);
}

UsdGeomXformOp 
UsdGeomXformable::AddRotateXOp(UsdGeomXformOp::Precision const precision,
    TfToken const &opSuffix, bool isInverseOp) const
{
    return AddXformOp(UsdGeomXformOp::TypeRotateX, precision, opSuffix, 
                      isInverseOp);
}

UsdGeomXformOp 
UsdGeomXformable::AddRotateYOp(UsdGeomXformOp::Precision const precision,
    TfToken const &opSuffix, bool isInverseOp) const
{
    return AddXformOp(UsdGeomXformOp::TypeRotateY, precision, opSuffix, 
                      isInverseOp);
}

UsdGeomXformOp 
UsdGeomXformable::AddRotateZOp(UsdGeomXformOp::Precision const precision,
    TfToken const &opSuffix, bool isInverseOp) const
{
    return AddXformOp(UsdGeomXformOp::TypeRotateZ, precision, opSuffix, isInverseOp);
}

UsdGeomXformOp 
UsdGeomXformable::AddRotateXYZOp(UsdGeomXformOp::Precision const precision,
    TfToken const &opSuffix, bool isInverseOp) const
{
    return AddXformOp(UsdGeomXformOp::TypeRotateXYZ, precision, opSuffix, 
                      isInverseOp);
}

UsdGeomXformOp 
UsdGeomXformable::AddRotateXZYOp(UsdGeomXformOp::Precision const precision,
    TfToken const &opSuffix, bool isInverseOp) const
{
    return AddXformOp(UsdGeomXformOp::TypeRotateXZY, precision, opSuffix,
                      isInverseOp);
}

UsdGeomXformOp 
UsdGeomXformable::AddRotateYXZOp(UsdGeomXformOp::Precision const precision,
    TfToken const &opSuffix, bool isInverseOp) const
{
    return AddXformOp(UsdGeomXformOp::TypeRotateYXZ, precision, opSuffix,
                      isInverseOp);
}

UsdGeomXformOp 
UsdGeomXformable::AddRotateYZXOp(UsdGeomXformOp::Precision const precision,
    TfToken const &opSuffix, bool isInverseOp) const
{
    return AddXformOp(UsdGeomXformOp::TypeRotateYZX, precision, opSuffix,
                      isInverseOp);
}

UsdGeomXformOp 
UsdGeomXformable::AddRotateZXYOp(UsdGeomXformOp::Precision const precision,
    TfToken const &opSuffix, bool isInverseOp) const
{
    return AddXformOp(UsdGeomXformOp::TypeRotateZXY, precision, opSuffix,
                      isInverseOp);
}

UsdGeomXformOp 
UsdGeomXformable::AddRotateZYXOp(UsdGeomXformOp::Precision const precision,
    TfToken const &opSuffix, bool isInverseOp) const
{
    return AddXformOp(UsdGeomXformOp::TypeRotateZYX, precision, opSuffix,
                      isInverseOp);
}

UsdGeomXformOp 
UsdGeomXformable::AddOrientOp(UsdGeomXformOp::Precision const precision,
    TfToken const &opSuffix, bool isInverseOp) const
{
    return AddXformOp(UsdGeomXformOp::TypeOrient, precision, opSuffix, isInverseOp);
}
    
UsdGeomXformOp 
UsdGeomXformable::AddTransformOp(UsdGeomXformOp::Precision const precision,
    TfToken const &opSuffix, bool isInverseOp) const
{
    return AddXformOp(UsdGeomXformOp::TypeTransform, precision, opSuffix, 
                      isInverseOp);
}

// Returns whether "!resetXformStack! exists in opOrderVec.
static 
bool 
_XformOpOrderHasResetXformStack(const VtTokenArray &opOrderVec)
{
    return std::find(opOrderVec.begin(), opOrderVec.end(), 
                     UsdGeomXformOpTypes->resetXformStack) != opOrderVec.end();
}

bool
UsdGeomXformable::SetResetXformStack(bool resetXformStack) const
{
    VtTokenArray opOrderVec;
    _GetXformOpOrderValue(&opOrderVec);

    bool result = true;
    if (resetXformStack) {
        // Nothing to do if resetXformStack already exists in xformOpOrder
        if (_XformOpOrderHasResetXformStack(opOrderVec))
            return true;

        VtTokenArray newOpOrderVec(opOrderVec.size() + 1);

        newOpOrderVec[0] = UsdGeomXformOpTypes->resetXformStack;
        for (size_t i = 0; i < opOrderVec.size(); i++)
            newOpOrderVec[i+1] = opOrderVec[i];

        result = CreateXformOpOrderAttr().Set(newOpOrderVec);
    } else {
        VtTokenArray newOpOrderVec;
        bool foundResetXformStack = false;
        for (size_t i = 0; i < opOrderVec.size(); i++) {
            if (opOrderVec[i] == UsdGeomXformOpTypes->resetXformStack) {
                foundResetXformStack = true;
                newOpOrderVec.clear();
            } else if (foundResetXformStack) {
                newOpOrderVec.push_back(opOrderVec[i]);
            }
        }

        if (foundResetXformStack) {
            result = CreateXformOpOrderAttr().Set(newOpOrderVec);
        } else {
            // This is a no-op if "!resetXformStack!" isn't present in 
            // xformOpOrder.
        }
    }

    return result;
}

bool
UsdGeomXformable::GetResetXformStack() const
{
    VtTokenArray opOrderVec;
    if (!_GetXformOpOrderValue(&opOrderVec))
        return false;

    return _XformOpOrderHasResetXformStack(opOrderVec);
}

bool 
UsdGeomXformable::SetXformOpOrder(
    vector<UsdGeomXformOp> const &orderedXformOps, 
    bool resetXformStack) const
{
    VtTokenArray ops;
    ops.reserve(orderedXformOps.size() + (resetXformStack ? 1: 0));

    if (resetXformStack)
        ops.push_back(UsdGeomXformOpTypes->resetXformStack);

    TF_FOR_ALL(it, orderedXformOps) {
        // Check to make sure that the xformOp being added to xformOpOrder 
        // belongs to this prim.
        if (it->GetAttr().GetPrim() == GetPrim()) {
            ops.push_back(it->GetOpName());
        } else {
            TF_CODING_ERROR("XformOp attribute <%s> does not belong to schema "
                            "prim <%s>.",  it->GetAttr().GetPath().GetText(),
                            GetPath().GetText()); 
            return false;
        }
    }

    return CreateXformOpOrderAttr().Set(ops);
}

bool
UsdGeomXformable::ClearXformOpOrder() const
{
    return SetXformOpOrder(vector<UsdGeomXformOp>());
}

UsdGeomXformOp
UsdGeomXformable::MakeMatrixXform() const
{
    ClearXformOpOrder();
    return AddTransformOp();
}

vector<UsdGeomXformOp>
UsdGeomXformable::GetOrderedXformOps(bool *resetsXformStack) const
{
    vector<UsdGeomXformOp> result;

    if (resetsXformStack) {
        *resetsXformStack = false;
    } else {
        TF_CODING_ERROR("resetsXformStack is NULL.");
    }

    bool xformOpOrderIsAuthored = false;
    VtTokenArray opOrderVec;
    if (!_GetXformOpOrderValue(&opOrderVec, &xformOpOrderIsAuthored)) {
        return result;
    }

    if (!xformOpOrderIsAuthored && 
        TfGetEnvSetting(USD_READ_OLD_STYLE_TRANSFORM)) 
    {
        // If a transform attribute exists, wrap it in a UsdGeomXformOp and 
        // return it.
        if (UsdAttribute transformAttr = _GetTransformAttr()) {
            UsdGeomXformOp xformOp;
            xformOp._attr = transformAttr;
            xformOp._opType = UsdGeomXformOp::TypeTransform;
            xformOp._isInverseOp = false;
            result.push_back(xformOp);
            return result;
        }
    }

    if (opOrderVec.size() == 0) {
        return result;
    }

    // Reserve space for the xform ops.
    result.reserve(opOrderVec.size());

    for (VtTokenArray::iterator it = opOrderVec.begin() ; 
         it != opOrderVec.end(); ++it) {

        const TfToken &opName = *it;

        // If this is the special resetXformStack op, then clear the currently
        // accreted xformOps and continue.
        if (opName == UsdGeomXformOpTypes->resetXformStack) { 
            if (resetsXformStack) {
                *resetsXformStack = true;
            }
            result.clear();
        } else {
            bool isInverseOp = false;
            if (UsdAttribute attr = UsdGeomXformOp::_GetXformOpAttr(
                    GetPrim(), opName, &isInverseOp)) {
                // Only add valid xform ops.                
                result.emplace_back(attr, isInverseOp);
            } else {
                // Skip invalid xform ops that appear in xformOpOrder, but issue
                // a warning.
                TF_WARN("Unable to get attribute associated with the xformOp "
                    "'%s', on the prim at path <%s>. Skipping xformOp in the "
                    "computation of the local transformation at prim.", 
                    opName.GetText(), GetPrim().GetPath().GetText());
            }
        }
    }

    return result;
}

UsdGeomXformable::XformQuery::XformQuery(const UsdGeomXformable &xformable):
    _resetsXformStack(false)
{
    vector<UsdGeomXformOp> orderedXformOps = 
        xformable.GetOrderedXformOps(&_resetsXformStack);

    if (!orderedXformOps.empty()) {
        _xformOps = orderedXformOps;

        // Create attribute queries for all the xform ops.
        TF_FOR_ALL(it, _xformOps) {
            it->_CreateAttributeQuery();
        }
    }
}

bool 
UsdGeomXformable::XformQuery::GetLocalTransformation(
    GfMatrix4d *transform,
    const UsdTimeCode time) const 
{
    return UsdGeomXformable::GetLocalTransformation(transform, _xformOps, time);
}

static
bool 
_TransformMightBeTimeVarying(vector<UsdGeomXformOp> const &xformOps)
{
    // If any of the xform ops may vary, then the cumulative transform may vary.
    TF_FOR_ALL(it, xformOps) {
        if (it->MightBeTimeVarying())
            return true;
    }

    return false;
}

bool
UsdGeomXformable::XformQuery::TransformMightBeTimeVarying() const
{
    return _TransformMightBeTimeVarying(_xformOps);
}

bool
UsdGeomXformable::TransformMightBeTimeVarying() const
{
    VtTokenArray opOrderVec;
    if (!_GetXformOpOrderValue(&opOrderVec))
        return false;

    if (opOrderVec.size() == 0) {
        // XXX: backwards compatibility
        if (TfGetEnvSetting(USD_READ_OLD_STYLE_TRANSFORM)) {
            if (UsdAttribute transformAttr = _GetTransformAttr())
                return transformAttr.ValueMightBeTimeVarying();
        }
        return false;
    }

    for (VtTokenArray::reverse_iterator it = opOrderVec.rbegin() ; 
         it != opOrderVec.rend(); ++it) {

        const TfToken &opName = *it;

        // If this is the special resetXformStack op, return false to indicate 
        // that none of the xformOps that affect the local transformation are 
        // time-varying (since none of the (valid) xformOps after the last 
        // occurrence of !resetXformStack! are time-varying).
        if (opName == UsdGeomXformOpTypes->resetXformStack) { 
            return false;
        } else {
            bool isInverseOp = false;
            if (UsdAttribute attr = UsdGeomXformOp::_GetXformOpAttr(
                    GetPrim(), opName, &isInverseOp)) {
                // Only check valid xform ops for time-varyingness.
                UsdGeomXformOp op(attr, isInverseOp);
                if (op && op.MightBeTimeVarying()) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool
UsdGeomXformable::TransformMightBeTimeVarying(
    const vector<UsdGeomXformOp> &ops) const
{
    if (!ops.empty())
        return _TransformMightBeTimeVarying(ops);

    // Assume unvarying if neither orderedXformOps nor transform attribute is 
    // authored.
    return false;
}

/* static */
bool
UsdGeomXformable::GetTimeSamples(
    vector<UsdGeomXformOp> const &orderedXformOps,
    vector<double> *times)
{
    return GetTimeSamplesInInterval(orderedXformOps, 
            GfInterval::GetFullInterval(), times);
}

/* static */
bool 
UsdGeomXformable::GetTimeSamplesInInterval(
    std::vector<UsdGeomXformOp> const &orderedXformOps,
    const GfInterval &interval,
    std::vector<double> *times)
{
    // Optimize for the case where there's a single xformOp (typically a 4x4 
    // matrix op). 
    if (orderedXformOps.size() == 1) {
        return orderedXformOps.front().GetTimeSamplesInInterval(interval, 
            times);
    }

    vector<UsdAttribute> xformOpAttrs;
    xformOpAttrs.reserve(orderedXformOps.size());
    for (auto &xformOp : orderedXformOps) {
        xformOpAttrs.push_back(xformOp.GetAttr());
    }

    return UsdAttribute::GetUnionedTimeSamplesInInterval(xformOpAttrs, 
            interval, times);
}

bool 
UsdGeomXformable::GetTimeSamplesInInterval(
    const GfInterval &interval,
    std::vector<double> *times) const
{
    bool resetsXformStack=false;
    const vector<UsdGeomXformOp> &orderedXformOps= GetOrderedXformOps(
        &resetsXformStack);

    // XXX: backwards compatibility
    if (orderedXformOps.empty() && 
        TfGetEnvSetting(USD_READ_OLD_STYLE_TRANSFORM)) {
                
        if (UsdAttribute transformAttr = _GetTransformAttr())
            return transformAttr.GetTimeSamplesInInterval(interval, times);
    }

    return UsdGeomXformable::GetTimeSamplesInInterval(orderedXformOps, interval, 
            times);
}

bool 
UsdGeomXformable::XformQuery::GetTimeSamples(vector<double> *times) const
{
    return GetTimeSamplesInInterval(GfInterval::GetFullInterval(), times);
}

bool 
UsdGeomXformable::XformQuery::GetTimeSamplesInInterval(
    const GfInterval &interval, 
    vector<double> *times) const
{
    if (_xformOps.size() == 1) {
        _xformOps.front().GetTimeSamplesInInterval(interval, times);
    }

    vector<UsdAttributeQuery> xformOpAttrQueries;
    xformOpAttrQueries.reserve(_xformOps.size());
    for (auto &xformOp : _xformOps) {
        // This should never throw and exception because XformQuery's constructor
        // initializes an attribute query for all its xformOps.
        const UsdAttributeQuery &attrQuery = 
            boost::get<UsdAttributeQuery>(xformOp._attr);
        xformOpAttrQueries.push_back(attrQuery);
    }
    
    return UsdAttributeQuery::GetUnionedTimeSamplesInInterval(
            xformOpAttrQueries, interval, times);
}

bool 
UsdGeomXformable::GetTimeSamples(vector<double> *times) const
{
    bool resetsXformStack=false;
    const vector<UsdGeomXformOp> &orderedXformOps= GetOrderedXformOps(
        &resetsXformStack);

    // XXX: backwards compatibility
    if (orderedXformOps.empty() && 
        TfGetEnvSetting(USD_READ_OLD_STYLE_TRANSFORM)) {
                
        if (UsdAttribute transformAttr = _GetTransformAttr())
            return transformAttr.GetTimeSamples(times);
    }

    return GetTimeSamples(orderedXformOps, times);
}

bool 
UsdGeomXformable::XformQuery::IsAttributeIncludedInLocalTransform(
    const TfToken &attrName) const
{
    TF_FOR_ALL(it, _xformOps) {
        if (it->GetName() == attrName)
            return true;
    }

    return false;
}

// Given two UsdGeomXformOps, returns true if they are inverses of each other.
static bool
_AreInverseXformOps(const UsdGeomXformOp &a, const UsdGeomXformOp &b)
{
    // The two given ops are inverses of each other if they have the same 
    // underlying attribute and only if one of them is an inverseOp.
    return a.GetAttr() == b.GetAttr() &&
           a.IsInverseOp() != b.IsInverseOp();
}

// Given two xformOp names, returns true if they are inverses of each other.
static bool
_AreInverseXformOps(const TfToken &a, const TfToken &b)
{
    return _tokens->invertPrefix.GetString() + a.GetString() == b.GetString() 
        || _tokens->invertPrefix.GetString() + b.GetString() == a.GetString();
}

bool 
UsdGeomXformable::GetLocalTransformation(
    GfMatrix4d *transform,
    bool *resetsXformStack,
    const UsdTimeCode time) const
{
    TRACE_FUNCTION();
 
    VtTokenArray opOrderVec;
    if (!_GetXformOpOrderValue(&opOrderVec))
        return false;

    if (opOrderVec.size() == 0) {
        *resetsXformStack = false;

        // XXX: backwards compatibility
        if (TfGetEnvSetting(USD_READ_OLD_STYLE_TRANSFORM)) {
            if (UsdAttribute transformAttr = _GetTransformAttr()) {
                return transformAttr.Get(transform, time);
            }
        }

        transform->SetIdentity();
        return true;
    }
    
    GfMatrix4d localXform(1.);
    bool foundResetXformStack = false;
    for (VtTokenArray::reverse_iterator it = opOrderVec.rbegin() ; 
         it != opOrderVec.rend(); ++it) {
            
        const TfToken &opName = *it;

        // Skip the current xformOp and the next one if they're inverses of 
        // each other.
        if ((it+1) != opOrderVec.rend()) {
            const TfToken &nextOpName = *(it+1);
            if (_AreInverseXformOps(opName, nextOpName)) {
                ++it;
                continue;
            }
        }

        // If this is the special resetXformStack op, then the currently
        // accreted localXform is the local transformation of the prim.
        if (opName == UsdGeomXformOpTypes->resetXformStack) { 
            foundResetXformStack = true;
            break;
        } else {
            bool isInverseOp = false;
            if (UsdAttribute attr = UsdGeomXformOp::_GetXformOpAttr(
                    GetPrim(), opName, &isInverseOp)) {
                // Only add valid xform ops.                
                UsdGeomXformOp op(attr, isInverseOp);
                if (op) {
                    GfMatrix4d opTransform = op.GetOpTransform(time);
                    // Avoid multiplying by the identity matrix when possible.
                    if (opTransform != *_IDENTITY) {
                        localXform *= opTransform;
                    }
                }
            } else {
                // Skip invalid xform ops that appear in xformOpOrder, but issue
                // a warning.
                TF_WARN("Unable to get attribute associated with the xformOp "
                    "'%s', on the prim at path <%s>. Skipping xformOp in the "
                    "computation of the local transformation at prim.", 
                    opName.GetText(), GetPrim().GetPath().GetText());
            }
        }
    }

    if (transform) {
        *transform = localXform;
    } else {
        TF_CODING_ERROR("'transform' pointer is NULL.");
    }

    if (resetsXformStack) {
        *resetsXformStack = foundResetXformStack;
    } else {
        TF_CODING_ERROR("'resetsXformStack' pointer is NULL.");
    }
    
    return true;
}

bool 
UsdGeomXformable::GetLocalTransformation(
    GfMatrix4d *transform,
    bool *resetsXformStack,
    const vector<UsdGeomXformOp> &ops,
    const UsdTimeCode time) const
{
    TRACE_FUNCTION();
    
    if (resetsXformStack) {
        *resetsXformStack = GetResetXformStack();
    } else {
        TF_CODING_ERROR("resetsXformStack is NULL.");
    }
    return GetLocalTransformation(transform, ops, time);
}

/* static */
bool 
UsdGeomXformable::GetLocalTransformation(
    GfMatrix4d *transform,
    const vector<UsdGeomXformOp> &orderedXformOps, 
    const UsdTimeCode time)
{
    GfMatrix4d xform(1.);

    for (vector<UsdGeomXformOp>::const_reverse_iterator 
            it = orderedXformOps.rbegin();
         it != orderedXformOps.rend(); ++it) {

        const UsdGeomXformOp &xformOp = *it;

        // Skip the current xformOp and the next one if they're inverses of 
        // each other.
        if ((it+1) != orderedXformOps.rend()) {
            const UsdGeomXformOp &nextXformOp = *(it+1);
            if (_AreInverseXformOps(xformOp, nextXformOp)) {
                ++it;
                continue;
            }
        }

        GfMatrix4d opTransform = xformOp.GetOpTransform(time);
        // Avoid multiplying by the identity matrix when possible.
        if (opTransform != *_IDENTITY) {
            xform *= opTransform;
        }
    }
    
    if (transform) {
        *transform = xform;
        return true;
    } else {
        TF_CODING_ERROR("'transform' pointer is NULL.");
    } 

    return false;
}

/* static */
bool 
UsdGeomXformable::IsTransformationAffectedByAttrNamed(const TfToken &attrName)
{
    // XXX: backwards compatibility
    return (TfGetEnvSetting(USD_READ_OLD_STYLE_TRANSFORM) && 
            attrName == _tokens->transform)        ||
           attrName == UsdGeomTokens->xformOpOrder ||
           UsdGeomXformOp::IsXformOp(attrName);
}

UsdAttribute
UsdGeomXformable::_GetTransformAttr() const
{
    return GetPrim().GetAttribute(_tokens->transform);
}

PXR_NAMESPACE_CLOSE_SCOPE
