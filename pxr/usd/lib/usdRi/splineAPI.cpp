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
#include "pxr/usd/usdGeom/collectionAPI.h"

#include "pxr/base/tf/staticTokens.h"

#include <set>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


using std::set;
using std::string;
using std::vector;

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (targetFaceCounts)
    (targetFaceIndices)
);

/* virtual */
UsdLightSplineAPI::~UsdLightSplineAPI()
{ }

/* virtual */
bool
UsdLightSplineAPI::_IsCompatible(const UsdPrim &prim) const
{
    return GetPrim() && _GetTargetsRel();
}

UsdRelationship 
UsdLightSplineAPI::_GetTargetsRel(bool create /* =false */) const
{
    const TfToken &relName = _GetCollectionPropertyName();
    return create ? GetPrim().CreateRelationship(relName, /* custom */ false) :
                    GetPrim().GetRelationship(relName);
}

UsdAttribute 
UsdLightSplineAPI::_GetTargetFaceCountsAttr(
    bool create /* =false */) const
{
    const TfToken &propName = _GetCollectionPropertyName(
        _tokens->targetFaceCounts);
    return create ? CreateTargetFaceCountsAttr() :
                    GetPrim().GetAttribute(propName);    
}

UsdAttribute 
UsdLightSplineAPI::_GetTargetFaceIndicesAttr(
    bool create /* =false */) const
{
    const TfToken &propName = _GetCollectionPropertyName(
        _tokens->targetFaceIndices);
    return create ? CreateTargetFaceIndicesAttr() :
                    GetPrim().GetAttribute(propName);
}

TfToken 
UsdLightSplineAPI::_GetCollectionPropertyName(
    const TfToken &baseName /* =TfToken() */) const
{
    return TfToken(UsdGeomTokens->collection.GetString() + ":" + 
                   _name.GetString() + 
                   (baseName.IsEmpty() ? "" : (":" + baseName.GetString())));
}

bool 
UsdLightSplineAPI::IsEmpty() const
{
    UsdRelationship targetsRel = _GetTargetsRel();
    if (targetsRel) {
        SdfPathVector targets;
        targetsRel.GetTargets(&targets);
        return targets.empty();
    }
    return true;
}

bool 
UsdLightSplineAPI::SetTargets(const SdfPathVector &targets) const
{
    return _GetTargetsRel(/* create */ true).SetTargets(targets);
}

bool 
UsdLightSplineAPI::GetTargets(SdfPathVector *targets,
                                 bool forwardToObjectsInMasters) const
{
    UsdRelationship rel = _GetTargetsRel();
    return rel && rel.GetTargets(targets, forwardToObjectsInMasters);
}

bool 
UsdLightSplineAPI::SetTargetFaceCounts(
    const VtIntArray &targetFaceCounts, 
    const UsdTimeCode &time /* =UsdTimeCode::Default() */) const
{
    return _GetTargetFaceCountsAttr(/*create*/ true).Set(
        targetFaceCounts, time);
}

bool 
UsdLightSplineAPI::GetTargetFaceCounts(
    VtIntArray *targetFaceCounts, 
    const UsdTimeCode &time /* =UsdTimeCode::Default() */) const
{
    UsdAttribute attr = _GetTargetFaceCountsAttr();
    return attr.Get(targetFaceCounts, time);
}

bool 
UsdLightSplineAPI::SetTargetFaceIndices(
    const VtIntArray &targetFaceIndices, 
    const UsdTimeCode &time/*=UsdTimeCode::Default()*/) const
{
    return _GetTargetFaceIndicesAttr(/*create*/ true).Set(
        targetFaceIndices, time);
}

bool 
UsdLightSplineAPI::GetTargetFaceIndices(
    VtIntArray *targetFaceIndices, 
    const UsdTimeCode &time /* =UsdTimeCode::Default() */) const
{
    UsdAttribute attr = _GetTargetFaceIndicesAttr();
    return attr.Get(targetFaceIndices, time);
}

bool 
UsdLightSplineAPI::AppendTarget(
    const SdfPath &target, 
    const VtIntArray &faceIndices /* =VtIntArray() */,
    const UsdTimeCode &time /* =UsdTimeCode::Default() */) const
{
    if (target.IsEmpty()) {
        TF_CODING_ERROR("Cannot add empty target to collection '%s' on "
            "prim <%s>.", _name.GetText(), GetPath().GetText());
        return false;
    }

    bool hasFaceCountsAtTime = true;
    if (time != UsdTimeCode::Default()) {
        UsdAttribute targetFaceCountsAttr = GetTargetFaceCountsAttr();
        double lower=0., upper=0.;
        bool hasTimeSamples=false;
        if (targetFaceCountsAttr.GetBracketingTimeSamples(time.GetValue(),
            &lower, &upper, &hasTimeSamples)) 
        {
            hasFaceCountsAtTime = (lower==upper && lower==time.GetValue());
        }
    }

    VtIntArray targetFaceCounts, targetFaceIndices;
    if (hasFaceCountsAtTime) {
        GetTargetFaceCounts(&targetFaceCounts, time);
        GetTargetFaceIndices(&targetFaceIndices, time);
    }

    SdfPathVector targets;
    GetTargets(&targets);

    // If there are no existing face restrictions and no face-restriction is 
    // specified on the current target, simly add the target and return.
    if (targetFaceCounts.empty() && targetFaceIndices.empty() &&
        faceIndices.empty()) 
    {
        // We can simply add the target here to the relationship here since 
        // there are no companion non-list-edited integer arrays.
        return CreateTargetsRel().AppendTarget(target);
    }

    if (targetFaceCounts.empty() && !targetFaceIndices.empty()) {
        TF_CODING_ERROR("targetFaceCounts is empty, but targetFaceIndices "
            "is not, for the collection '%s' belonging to prim <%s>.",
            _name.GetText(), GetPath().GetText());
        return false;
    }

    if (targetFaceCounts.empty() && !faceIndices.empty()) {
        for (size_t i = 0 ; i < targets.size(); i++)
            targetFaceCounts.push_back(0);
    }

    targetFaceCounts.push_back(faceIndices.size()); 
    targetFaceIndices.reserve(targetFaceIndices.size() + faceIndices.size());
    TF_FOR_ALL(it, faceIndices) {
        targetFaceIndices.push_back(*it);
    }
    targets.push_back(target);

    // We can't simply add the target here to the relationship since we 
    // have companion non-list-edited integer arrays. We must keep them in
    // sync irrespective of what may change in weaker layers.
    return SetTargets(targets) && SetTargetFaceCounts(targetFaceCounts, time) 
        && SetTargetFaceIndices(targetFaceIndices, time);
}

UsdAttribute 
UsdLightSplineAPI::GetTargetFaceCountsAttr() const
{
    return _GetTargetFaceCountsAttr();
}


UsdAttribute 
UsdLightSplineAPI::CreateTargetFaceCountsAttr(
    const VtValue &defaultValue /* =VtValue() */,
    bool writeSparsely /* =false */) const
{
    const TfToken &propName = _GetCollectionPropertyName(_tokens->targetFaceCounts);
    return UsdSchemaBase::_CreateAttr(propName,
                                      SdfValueTypeNames->IntArray,
                                      /* custom = */ false,
                                      SdfVariabilityVarying,
                                      defaultValue,
                                      writeSparsely);
}

UsdAttribute 
UsdLightSplineAPI::GetTargetFaceIndicesAttr() const
{
    return _GetTargetFaceIndicesAttr();
}

UsdAttribute 
UsdLightSplineAPI::CreateTargetFaceIndicesAttr(
    const VtValue &defaultValue /* =VtValue() */,
    bool writeSparsely /* =false */) const
{
    const TfToken &propName = _GetCollectionPropertyName(_tokens->targetFaceIndices);
    return UsdSchemaBase::_CreateAttr(propName,
                                      SdfValueTypeNames->IntArray,
                                      /* custom = */ false,
                                      SdfVariabilityVarying,
                                      defaultValue,
                                      writeSparsely);
}

UsdRelationship 
UsdLightSplineAPI::GetTargetsRel() const
{
    return _GetTargetsRel();
}

UsdRelationship 
UsdLightSplineAPI::CreateTargetsRel() const
{
    return _GetTargetsRel(/* create */ true);
}

/* static */
UsdLightSplineAPI 
UsdLightSplineAPI::Create(
    const UsdPrim &prim, 
    const TfToken &name,
    const SdfPathVector &targets /* =SdfPathVector() */,
    const VtIntArray &targetFaceCounts /* =VtIntArray() */,
    const VtIntArray &targetFaceIndices /* =VtIntArray() */)
{
    UsdLightSplineAPI collection(prim, name);

    // If the collection relationship does not exist or 
    // if the set of targets is not empty, then call SetTargets to create
    // the collection and set the specified targets.
    if (!collection.GetTargetsRel() || !targets.empty())
        collection.SetTargets(targets);

    if (!targetFaceCounts.empty() || !targetFaceIndices.empty()) {
        collection.SetTargetFaceCounts(targetFaceCounts);
        collection.SetTargetFaceIndices(targetFaceIndices);
    }
    return collection;
}

/* static */
UsdLightSplineAPI 
UsdLightSplineAPI::Create(
    const UsdSchemaBase &schemaObj, 
    const TfToken &name,
    const SdfPathVector &targets /* =SdfPathVector() */,
    const VtIntArray &targetFaceCounts /* =VtIntArray() */,
    const VtIntArray &targetFaceIndices /* =VtIntArray() */)
{
    return UsdLightSplineAPI::Create(schemaObj.GetPrim(), name, targets,
        targetFaceCounts, targetFaceIndices);
}

/* static */
std::vector<UsdLightSplineAPI> 
UsdLightSplineAPI::GetCollections(const UsdPrim &prim)
{
    vector<UsdLightSplineAPI> result;
    vector<UsdProperty> collectionProperties = prim.GetPropertiesInNamespace(
        UsdGeomTokens->collection);
    TF_FOR_ALL(propIt, collectionProperties) {
        const UsdProperty &prop = *propIt;

        if (!prop.Is<UsdRelationship>())
            continue;

        vector<string> nameTokens = prop.SplitName();
        if (nameTokens.size() == 2) {
            result.push_back(UsdLightSplineAPI(
                prim, TfToken(nameTokens[1])));
        }
    }
    return result;
}

/* static */
std::vector<UsdLightSplineAPI> 
UsdLightSplineAPI::GetCollections(const UsdSchemaBase &schemaObj)
{
    return UsdLightSplineAPI::GetCollections(schemaObj.GetPrim());
}

static 
string _Stringify(const UsdTimeCode &time) {
    return time.IsDefault() ? string("DEFAULT") : TfStringify(time.GetValue());
}

bool 
UsdLightSplineAPI::Validate(std::string *reason) const
{
    SdfPathVector targets;
    const bool hasTargets = GetTargets(&targets);

    if (!hasTargets) {
        *reason += "Could not get targets.\n";
        return false;
    }

    VtIntArray targetFaceCounts, targetFaceIndices;
    const bool hasTargetFaceCounts = 
        GetTargetFaceCounts(&targetFaceCounts, UsdTimeCode(0.0));
    const bool hasTargetFaceIndices = 
        GetTargetFaceIndices(&targetFaceCounts, UsdTimeCode(0.0));

    if (hasTargetFaceCounts ^ hasTargetFaceIndices) {
        *reason += "collection has only one of targetFaceCounts and "
            "targetFaceIndices authored. It should have both or neither.\n";
        return false;
    }
    
    bool isValid = true;
    if (hasTargets && targets.empty()) {
        // Make sure that targetFaceCounts and targetFaceIndices are empty too.
        if (!targetFaceCounts.empty() || !targetFaceIndices.empty()) {
            isValid = false;
            *reason += "Collection has empty targets, but non-empty "
                "targetFaceCounts or targetFaceIndices.\n";
        }
        return isValid;
    }

    const size_t numTargets = targets.size();

    const UsdAttribute targetFaceCountsAttr = GetTargetFaceCountsAttr();
    const UsdAttribute targetFaceIndicesAttr = GetTargetFaceIndicesAttr();

    if (!targetFaceCountsAttr && !targetFaceIndicesAttr) {
        return true;
    } 

    TF_VERIFY(!((bool)targetFaceCountsAttr ^ (bool)targetFaceIndicesAttr));

    // The list of all timeSamples at which the collection attributes are 
    // authored.
    vector<UsdTimeCode> allTimes;

    VtIntArray defaultTargetFaceCounts, defaultTargetFaceIndices;
    if (targetFaceCountsAttr.Get(&defaultTargetFaceCounts, UsdTimeCode::Default()) ||
        targetFaceIndicesAttr.Get(&defaultTargetFaceIndices, UsdTimeCode::Default())) 
    {
        allTimes.push_back(UsdTimeCode::Default());
    }

    vector<double> tfiTimes, tfcTimes;
    set<double> allTimeSamples;
    if (targetFaceIndicesAttr.GetTimeSamples(&tfiTimes))
        allTimeSamples.insert(tfiTimes.begin(), tfiTimes.end());

    if (targetFaceCountsAttr.GetTimeSamples(&tfcTimes))
        allTimeSamples.insert(tfcTimes.begin(), tfcTimes.end());

    allTimes.reserve(allTimes.size() + allTimeSamples.size());
    TF_FOR_ALL(timeSampleIt, allTimeSamples) {
        allTimes.push_back(UsdTimeCode(*timeSampleIt));
    }

    TF_FOR_ALL(timeIt, allTimes) {
        const UsdTimeCode &time = *timeIt;

        VtIntArray faceCounts, faceIndices;
        if (!GetTargetFaceCounts(&faceCounts, time) ||
            !GetTargetFaceIndices(&faceIndices, time)) {
            *reason += TfStringPrintf("Unable to get targetFaceCounts or "
                "targetFaceIndices at time %s.", _Stringify(time).c_str());
            isValid = false;
            continue;
        }

        if (faceCounts.size() != numTargets) {
            *reason += TfStringPrintf("Number of elements in 'targetFaceCounts'"
                " (%ld) does not match the number of targets (%ld) at frame %s.", 
                faceCounts.size(), numTargets, _Stringify(time).c_str());
            isValid = false;
        }
        
        size_t totalFaceCounts = 0;
        TF_FOR_ALL(faceCountsIter, faceCounts) 
            totalFaceCounts += *faceCountsIter;

        if (faceIndices.size() != totalFaceCounts) {
            *reason += TfStringPrintf("The sum of all 'targetFaceCounts'"
                " (%ld) does not match the size of 'targetFaceIndices' (%ld) "
                "at frame %s.", totalFaceCounts, faceIndices.size(), 
                _Stringify(time).c_str());
            isValid = false;
        }
    }
    
    return isValid;
}


PXR_NAMESPACE_CLOSE_SCOPE

