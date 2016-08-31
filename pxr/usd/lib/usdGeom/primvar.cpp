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
#include "pxr/usd/usdGeom/primvar.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/relationship.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticTokens.h"

#include <vector>

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((primvarsPrefix, "primvars:"))
    ((idFrom, ":idFrom"))
    ((indicesSuffix, ":indices"))
);

UsdGeomPrimvar::UsdGeomPrimvar(const UsdAttribute &attr)
    : _attr(attr)
{
    _SetIdTargetRelName();
}



// Assumes that name is prefixed with the primvars namespace
inline static
bool _ContainsExtraNamespaces(const TfToken &name)
{
    static const size_t prefixSize = _tokens->primvarsPrefix.GetString().size();
    return (name.GetString().find(':', prefixSize) != std::string::npos);
}

/* static */
bool 
UsdGeomPrimvar::IsPrimvar(const UsdAttribute &attr)
{
    if (not attr)
        return false;
    
    TfToken const &name = attr.GetName();
    return _IsNamespaced(name) and not _ContainsExtraNamespaces(name);
}

/* static */
bool
UsdGeomPrimvar::_IsNamespaced(const TfToken& name)
{
    return TfStringStartsWith(name, _tokens->primvarsPrefix);
}
    
/* static */
TfToken
UsdGeomPrimvar::_MakeNamespaced(const TfToken& name, bool quiet)
{
    TfToken  result;
    if (_IsNamespaced(name)){
        result = name;
    }
    else {
        result = TfToken(_tokens->primvarsPrefix.GetString() + name.GetString());
    }

    if (_ContainsExtraNamespaces(result)){
        result = TfToken();
        if (not quiet){
            TF_CODING_ERROR("%s is not a valid name for a Primvar, because"
                            " it contains namespaces.",
                            name.GetText());
        }
    }
    
    return result;
}

/* static */
TfToken const&
UsdGeomPrimvar::_GetNamespacePrefix()
{
    return _tokens->primvarsPrefix;
}

TfToken
UsdGeomPrimvar::GetInterpolation() const
{
    TfToken interpolation;
    
    if (not _attr.GetMetadata(UsdGeomTokens->interpolation, &interpolation)){
        interpolation = UsdGeomTokens->constant;
    }

    return interpolation;
}

bool 
UsdGeomPrimvar::HasAuthoredInterpolation() const
{
    return _attr.HasAuthoredMetadata(UsdGeomTokens->interpolation);
}

bool
UsdGeomPrimvar::IsValidInterpolation(const TfToken &interpolation)
{
    return ((interpolation == UsdGeomTokens->constant) or
            (interpolation == UsdGeomTokens->uniform) or
            (interpolation == UsdGeomTokens->vertex) or
            (interpolation == UsdGeomTokens->varying) or
            (interpolation == UsdGeomTokens->faceVarying));
}

bool
UsdGeomPrimvar::SetInterpolation(const TfToken &interpolation)
{
    if (not IsValidInterpolation(interpolation)){
        TF_CODING_ERROR("Attempt to set invalid primvar interpolation "
                         "\"%s\" for attribute %s",
                         interpolation.GetText(),
                         _attr.GetPath().GetString().c_str());
        return false;
    }
    return _attr.SetMetadata(UsdGeomTokens->interpolation, interpolation);
}

int
UsdGeomPrimvar::GetElementSize() const
{
    int eltSize = 1;
    _attr.GetMetadata(UsdGeomTokens->elementSize, &eltSize);

    return eltSize;
}
    
bool
UsdGeomPrimvar::SetElementSize(int eltSize)
{
    if (eltSize < 1){
        TF_CODING_ERROR("Attempt to set elementSize to %d for attribute "
                         "%s (must be a positive, non-zero value)",
                         eltSize,
                         _attr.GetPath().GetString().c_str());
        return false;
    }
    return _attr.SetMetadata(UsdGeomTokens->elementSize, eltSize);
}

bool 
UsdGeomPrimvar::HasAuthoredElementSize() const
{
    return _attr.HasAuthoredMetadata(UsdGeomTokens->elementSize);
}


void
UsdGeomPrimvar::GetDeclarationInfo(TfToken *name, SdfValueTypeName *typeName,
                                 TfToken *interpolation, 
                                 int *elementSize) const
{
    TF_VERIFY(name and typeName and interpolation and elementSize);

    // We don't have any more efficient access pattern yet, but at least
    // we're still saving client some code
    *name = GetBaseName();
    *typeName = GetTypeName();
    *interpolation = GetInterpolation();
    *elementSize = GetElementSize();
}

UsdAttribute 
UsdGeomPrimvar::_GetIndicesAttr(bool create) const
{
    TfToken indicesAttrName(GetName().GetString() + 
        _tokens->indicesSuffix.GetString());

    if (create) {
        return _attr.GetPrim().CreateAttribute(indicesAttrName,
            SdfValueTypeNames->IntArray, /*custom*/ false, 
            SdfVariabilityVarying);
    }
    else {
        return _attr.GetPrim().GetAttribute(indicesAttrName);
    }
}

bool 
UsdGeomPrimvar::SetIndices(const VtIntArray &indices, 
                           UsdTimeCode time) const
{
    // Check if the typeName is array valued here and issue a warning 
    // if it's not.
    SdfValueTypeName typeName = GetTypeName();
    if (not typeName.IsArray()) {
        TF_CODING_ERROR("Setting indices on non-array valued primvar of type "
            "'%s'.", typeName.GetAsToken().GetText());
        return false;
    }
    return _GetIndicesAttr(/*create*/ true).Set(indices, time);

}

void
UsdGeomPrimvar::BlockIndices() const
{
    // Check if the typeName is array valued here and issue a warning 
    // if it's not.
    SdfValueTypeName typeName = GetTypeName();
    if (not typeName.IsArray()) {
        TF_CODING_ERROR("Setting indices on non-array valued primvar of type "
            "'%s'.", typeName.GetAsToken().GetText());
        return;
    }
    _GetIndicesAttr(/*create*/ true).Block();
}

bool 
UsdGeomPrimvar::GetIndices(VtIntArray *indices,
                           UsdTimeCode time) const
{
    UsdAttribute indicesAttr = _GetIndicesAttr(/*create*/ false);
    if (indicesAttr)
        return indicesAttr.Get(indices, time);

    return false;
}

bool 
UsdGeomPrimvar::IsIndexed() const
{
    // XXX update this to api that can directly see if an attribute is blocked.
    VtIntArray dummy;
    return _GetIndicesAttr(/*create*/ false).Get(&dummy);
}

bool 
UsdGeomPrimvar::SetUnauthoredValuesIndex(int unauthoredValuesIndex) const
{
    return _attr.SetMetadata(UsdGeomTokens->unauthoredValuesIndex, 
                             unauthoredValuesIndex);
}

int 
UsdGeomPrimvar::GetUnauthoredValuesIndex() const
{
    int unauthoredValuesIndex = -1;
    _attr.GetMetadata(UsdGeomTokens->unauthoredValuesIndex, 
                      &unauthoredValuesIndex);

    return unauthoredValuesIndex;    
}

template <typename ArrayType>
/* static */
bool 
UsdGeomPrimvar::_ComputeFlattenedArray(const VtValue &attrVal,
                                        const VtIntArray &indices,
                                        VtValue *value)
{
    if (not attrVal.IsHolding<ArrayType>())
        return false;

    ArrayType result;
    if (_ComputeFlattenedHelper(attrVal.UncheckedGet<ArrayType>(), indices, 
                                &result)) {
        *value = VtValue::Take(result);
    }

    return true;
}

bool 
UsdGeomPrimvar::ComputeFlattened(VtValue *value, UsdTimeCode time) const
{
    VtValue attrVal;
    if (not Get(&attrVal, time)) {
        return false;
    }

    // If the primvar attr value is not an array or if the primvar isn't 
    // indexed, simply return the attribute value.
    if (not attrVal.IsArrayValued() or not IsIndexed()) {
        *value = VtValue::Take(attrVal);
        return true;
    }

    VtIntArray indices;
    if (not GetIndices(&indices, time)) {
        TF_CODING_ERROR("No indices authored for indexed primvar <%s>.", 
                        _attr.GetPath().GetText());
        return false;
    }

    // Handle all known supported array value types.
    bool foundSupportedType =
        _ComputeFlattenedArray<VtVec2fArray>(attrVal, indices, value) or
        _ComputeFlattenedArray<VtVec2dArray>(attrVal, indices, value) or
        _ComputeFlattenedArray<VtVec2iArray>(attrVal, indices, value) or    
        _ComputeFlattenedArray<VtVec2hArray>(attrVal, indices, value) or    
        _ComputeFlattenedArray<VtVec3fArray>(attrVal, indices, value) or
        _ComputeFlattenedArray<VtVec3dArray>(attrVal, indices, value) or
        _ComputeFlattenedArray<VtVec3iArray>(attrVal, indices, value) or    
        _ComputeFlattenedArray<VtVec3hArray>(attrVal, indices, value) or    
        _ComputeFlattenedArray<VtVec4fArray>(attrVal, indices, value) or
        _ComputeFlattenedArray<VtVec4dArray>(attrVal, indices, value) or
        _ComputeFlattenedArray<VtVec4iArray>(attrVal, indices, value) or    
        _ComputeFlattenedArray<VtVec4hArray>(attrVal, indices, value) or
        _ComputeFlattenedArray<VtMatrix3dArray>(attrVal, indices, value) or    
        _ComputeFlattenedArray<VtMatrix4dArray>(attrVal, indices, value) or
        _ComputeFlattenedArray<VtStringArray>(attrVal, indices, value) or       
        _ComputeFlattenedArray<VtDoubleArray>(attrVal, indices, value) or 
        _ComputeFlattenedArray<VtIntArray>(attrVal, indices, value) or 
        _ComputeFlattenedArray<VtFloatArray>(attrVal, indices, value) or
        _ComputeFlattenedArray<VtHalfArray>(attrVal, indices, value);

    if (not foundSupportedType) {
        TF_WARN("Unsupported indexed primvar value type %s.", 
                attrVal.GetTypeName().c_str());
    }

    return not value->IsEmpty();
}

UsdGeomPrimvar::UsdGeomPrimvar(const UsdPrim& prim, 
                               const TfToken& baseName,
                               const SdfValueTypeName &typeName,
                               bool custom)
{
    TF_VERIFY(prim);

    TfToken attrName = _MakeNamespaced(baseName);

    if (not attrName.IsEmpty()){
        _attr = prim.CreateAttribute(attrName, typeName, custom);
    }
    // If a problem occurred, an error should already have been issued,
    // and _attr will be invalid, which is what we want

    _SetIdTargetRelName();
}

void
UsdGeomPrimvar::_SetIdTargetRelName()
{
    if (not _attr) {
        return;
    }

    const SdfValueTypeName& typeName = _attr.GetTypeName();
    if (typeName == SdfValueTypeNames->String or
            typeName == SdfValueTypeNames->StringArray) {
        std::string name(_attr.GetName().GetString());
        _idTargetRelName = TfToken(name.append(_tokens->idFrom.GetText()));
    }
}

UsdRelationship
UsdGeomPrimvar::_GetIdTargetRel(bool create) const
{
    if (create) {
        return _attr.GetPrim().CreateRelationship(_idTargetRelName);
    }
    else {
        return _attr.GetPrim().GetRelationship(_idTargetRelName);
    }
}

bool
UsdGeomPrimvar::IsIdTarget() const
{
    return not _idTargetRelName.IsEmpty() and _GetIdTargetRel(false);
}

bool
UsdGeomPrimvar::SetIdTarget(
        const SdfPath& path) const
{
    if (_idTargetRelName.IsEmpty()) {
        TF_CODING_ERROR("Can only set ID Target for string or string[] typed"
                        " primvars (primvar type is '%s')",
                        _attr.GetTypeName().GetAsToken().GetText());
        return false;
    }

    if (UsdRelationship rel = _GetIdTargetRel(true)) {
        SdfPathVector targets;
        targets.push_back(path.IsEmpty() ? _attr.GetPrimPath() : 
                path);
        return rel.SetTargets(targets);
    }
    return false;
}

template <>
bool
UsdGeomPrimvar::Get(
        std::string* value,
        UsdTimeCode time) const
{
    // check if there is a relationship and if so use the target path string to
    // get the string value.
    if (not _idTargetRelName.IsEmpty()) {
        if (UsdRelationship rel = _GetIdTargetRel(false)) {
            SdfPathVector targets;
            if (rel.GetForwardedTargets(&targets) and
                    targets.size() == 1) {
                *value = targets[0].GetString();
                return true;
            }
            return false;
        }
    }

    return _attr.Get(value, time);
}

// XXX: for now we just take the first.  here's an idea for how
// it'd work for multiple targets:
//   string[] primvars:handleids (interpolation = "uniform")
//   int[]    primvars:handleids:indices = [0, 1, 1, 1, 0, ...., 1]
//   rel      primvars:handleids:idFrom = [</a/t1>, </a/t2>]
template <>
bool
UsdGeomPrimvar::Get(
        VtStringArray* value,
        UsdTimeCode time) const
{
    // check if there is a relationship and if so use the target path string to
    // get the string value... Just take the first target, for now.
    if (not _idTargetRelName.IsEmpty()) {
        if (UsdRelationship rel = _GetIdTargetRel(false)) {
            value->clear();
            SdfPathVector targets;
            if (rel.GetForwardedTargets(&targets) and
                    targets.size() > 1) {
                value->push_back(targets[0].GetString());
                return true;
            }
            return false;
        }
    }

    return _attr.Get(value, time);
}

template <>
bool
UsdGeomPrimvar::Get(
        VtValue* value,
        UsdTimeCode time) const
{
    if (not _idTargetRelName.IsEmpty()) {
        const SdfValueTypeName& typeName = _attr.GetTypeName();
        if (typeName == SdfValueTypeNames->String) {
            std::string s;
            bool ret = Get(&s, time);
            if (ret) {
                *value = VtValue(s);
            }
            return ret;
        }
        else if (typeName == SdfValueTypeNames->StringArray) {
            VtStringArray s;
            bool ret = Get(&s, time);
            if (ret) {
                *value = VtValue(s);
            }
            return ret;
        }
    }

    return _attr.Get(value, time);
}

