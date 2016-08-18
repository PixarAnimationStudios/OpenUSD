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
#include "pxr/usd/usdGeom/collectionAPI.h"

#include "pxr/base/tf/staticTokens.h"

#include <set>
#include <string>
#include <vector>

using std::set;
using std::string;
using std::vector;

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (targetFaceCounts)
    (targetFaceIndices)
);

/* virtual */
UsdGeomCollectionAPI::~UsdGeomCollectionAPI()
{ }

/* virtual */
bool
UsdGeomCollectionAPI::_IsCompatible(const UsdPrim &prim) const
{
    return GetPrim() and _GetTargetsRel();
}

UsdRelationship 
UsdGeomCollectionAPI::_GetTargetsRel(bool create /* =false */) const
{
    const TfToken &relName = _GetCollectionPropertyName();
    return create ? GetPrim().CreateRelationship(relName, /* custom */ false) :
                    GetPrim().GetRelationship(relName);
}

UsdAttribute 
UsdGeomCollectionAPI::_GetTargetFaceCountsAttr(
    bool create /* =false */) const
{
    const TfToken &propName = _GetCollectionPropertyName(
        _tokens->targetFaceCounts);
    return create ? CreateTargetFaceCountsAttr() :
                    GetPrim().GetAttribute(propName);    
}

UsdAttribute 
UsdGeomCollectionAPI::_GetTargetFaceIndicesAttr(
    bool create /* =false */) const
{
    const TfToken &propName = _GetCollectionPropertyName(
        _tokens->targetFaceIndices);
    return create ? CreateTargetFaceIndicesAttr() :
                    GetPrim().GetAttribute(propName);
}

TfToken 
UsdGeomCollectionAPI::_GetCollectionPropertyName(
    const TfToken &baseName /* =TfToken() */) const
{
    return TfToken(UsdGeomTokens->collection.GetString() + ":" + 
                   _name.GetString() + 
                   (baseName.IsEmpty() ? "" : (":" + baseName.GetString())));
}

bool 
UsdGeomCollectionAPI::IsEmpty() const
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
UsdGeomCollectionAPI::SetTargets(const SdfPathVector &targets) const
{
    return _GetTargetsRel(/* create */ true).SetTargets(targets);
}

bool 
UsdGeomCollectionAPI::GetTargets(SdfPathVector *targets) const
{
    UsdRelationship rel = _GetTargetsRel();
    return rel and rel.GetForwardedTargets(targets);
}

bool 
UsdGeomCollectionAPI::SetTargetFaceCounts(
    const VtIntArray &targetFaceCounts, 
    const UsdTimeCode &time /* =UsdTimeCode::Default() */) const
{
    return _GetTargetFaceCountsAttr(/*create*/ true).Set(
        targetFaceCounts, time);
}

bool 
UsdGeomCollectionAPI::GetTargetFaceCounts(
    VtIntArray *targetFaceCounts, 
    const UsdTimeCode &time /* =UsdTimeCode::Default() */) const
{
    UsdAttribute attr = _GetTargetFaceCountsAttr();
    return attr.Get(targetFaceCounts, time);
}

bool 
UsdGeomCollectionAPI::SetTargetFaceIndices(
    const VtIntArray &targetFaceIndices, 
    const UsdTimeCode &time/*=UsdTimeCode::Default()*/) const
{
    return _GetTargetFaceIndicesAttr(/*create*/ true).Set(
        targetFaceIndices, time);
}

bool 
UsdGeomCollectionAPI::GetTargetFaceIndices(
    VtIntArray *targetFaceIndices, 
    const UsdTimeCode &time /* =UsdTimeCode::Default() */) const
{
    UsdAttribute attr = _GetTargetFaceIndicesAttr();
    return attr.Get(targetFaceIndices, time);
}

bool 
UsdGeomCollectionAPI::AppendTarget(
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
            hasFaceCountsAtTime = (lower==upper and lower==time.GetValue());
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
    if (targetFaceCounts.empty() and targetFaceIndices.empty() and 
        faceIndices.empty()) 
    {
        // We can simply add the target here to the relationship here since 
        // there are no companion non-list-edited integer arrays.
        return CreateTargetsRel().AddTarget(target);
    }

    if (targetFaceCounts.empty() and not targetFaceIndices.empty()) {
        TF_CODING_ERROR("targetFaceCounts is empty, but targetFaceIndices "
            "is not, for the collection '%s' belonging to prim <%s>.",
            _name.GetText(), GetPath().GetText());
        return false;
    }

    if (targetFaceCounts.empty() and not faceIndices.empty()) {
        for (size_t i = 0 ; i < targets.size(); i++)
            targetFaceCounts.push_back(0);
    }

    targetFaceCounts.push_back(static_cast<int>(faceIndices.size())); 
    targetFaceIndices.reserve(targetFaceIndices.size() + faceIndices.size());
    TF_FOR_ALL(it, faceIndices) {
        targetFaceIndices.push_back(*it);
    }
    targets.push_back(target);

    // We can't simply add the target here to the relationship since we 
    // have companion non-list-edited integer arrays. We must keep them in
    // sync irrespective of what may change in weaker layers.
    return SetTargets(targets) and SetTargetFaceCounts(targetFaceCounts, time) 
        and SetTargetFaceIndices(targetFaceIndices, time);
}

UsdAttribute 
UsdGeomCollectionAPI::GetTargetFaceCountsAttr() const
{
    return _GetTargetFaceCountsAttr();
}


UsdAttribute 
UsdGeomCollectionAPI::CreateTargetFaceCountsAttr(
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
UsdGeomCollectionAPI::GetTargetFaceIndicesAttr() const
{
    return _GetTargetFaceIndicesAttr();
}

UsdAttribute 
UsdGeomCollectionAPI::CreateTargetFaceIndicesAttr(
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
UsdGeomCollectionAPI::GetTargetsRel() const
{
    return _GetTargetsRel();
}

UsdRelationship 
UsdGeomCollectionAPI::CreateTargetsRel() const
{
    return _GetTargetsRel(/* create */ true);
}

/* static */
UsdGeomCollectionAPI 
UsdGeomCollectionAPI::Create(
    const UsdPrim &prim, 
    const TfToken &name,
    const SdfPathVector &targets /* =SdfPathVector() */,
    const VtIntArray &targetFaceCounts /* =VtIntArray() */,
    const VtIntArray &targetFaceIndices /* =VtIntArray() */)
{
    UsdGeomCollectionAPI collection(prim, name);

    // If the collection relationship does not exist or 
    // if the set of targets is not empty, then call SetTargets to create
    // the collection and set the specified targets.
    if (not collection.GetTargetsRel() or not targets.empty())
        collection.SetTargets(targets);

    if (not targetFaceCounts.empty() or not targetFaceIndices.empty()) {
        collection.SetTargetFaceCounts(targetFaceCounts);
        collection.SetTargetFaceIndices(targetFaceIndices);
    }
    return collection;
}

/* static */
UsdGeomCollectionAPI 
UsdGeomCollectionAPI::Create(
    const UsdSchemaBase &schemaObj, 
    const TfToken &name,
    const SdfPathVector &targets /* =SdfPathVector() */,
    const VtIntArray &targetFaceCounts /* =VtIntArray() */,
    const VtIntArray &targetFaceIndices /* =VtIntArray() */)
{
    return UsdGeomCollectionAPI::Create(schemaObj.GetPrim(), name, targets,
        targetFaceCounts, targetFaceIndices);
}

/* static */
std::vector<UsdGeomCollectionAPI> 
UsdGeomCollectionAPI::GetCollections(const UsdPrim &prim)
{
    vector<UsdGeomCollectionAPI> result;
    vector<UsdProperty> collectionProperties = prim.GetPropertiesInNamespace(
        UsdGeomTokens->collection);
    TF_FOR_ALL(propIt, collectionProperties) {
        const UsdProperty &prop = *propIt;

        if (not prop.Is<UsdRelationship>())
            continue;

        vector<string> nameTokens = prop.SplitName();
        if (nameTokens.size() == 2) {
            result.push_back(UsdGeomCollectionAPI(
                prim, TfToken(nameTokens[1])));
        }
    }
    return result;
}

/* static */
std::vector<UsdGeomCollectionAPI> 
UsdGeomCollectionAPI::GetCollections(const UsdSchemaBase &schemaObj)
{
    return UsdGeomCollectionAPI::GetCollections(schemaObj.GetPrim());
}

static 
string _Stringify(const UsdTimeCode &time) {
    return time.IsDefault() ? string("DEFAULT") : TfStringify(time.GetValue());
}

bool 
UsdGeomCollectionAPI::Validate(std::string *reason) const
{
    SdfPathVector targets;
    const bool hasTargets = GetTargets(&targets);

    if (not hasTargets) {
        *reason += "Could not get targets.\n";
        return false;
    }

    VtIntArray targetFaceCounts, targetFaceIndices;
    const bool hasTargetFaceCounts = 
        GetTargetFaceCounts(&targetFaceCounts, UsdTimeCode(0.0));
    const bool hasTargetFaceIndices = 
        GetTargetFaceIndices(&targetFaceCounts, UsdTimeCode(0.0));

    if (hasTargetFaceCounts xor hasTargetFaceIndices) {
        *reason += "collection has only one of targetFaceCounts and "
            "targetFaceIndices authored. It should have both or neither.\n";
        return false;
    }
    
    bool isValid = true;
    if (hasTargets and targets.empty()) {
        // Make sure that targetFaceCounts and targetFaceIndices are empty too.
        if (not targetFaceCounts.empty() or not targetFaceIndices.empty()) {
            isValid = false;
            *reason += "Collection has empty targets, but non-empty "
                "targetFaceCounts or targetFaceIndices.\n";
        }
        return isValid;
    }

    const size_t numTargets = targets.size();

    const UsdAttribute targetFaceCountsAttr = GetTargetFaceCountsAttr();
    const UsdAttribute targetFaceIndicesAttr = GetTargetFaceIndicesAttr();

    if (not targetFaceCountsAttr and not targetFaceIndicesAttr) {
        return true;
    } 

    TF_VERIFY(not (((bool)targetFaceCountsAttr) xor ((bool)targetFaceIndicesAttr)));

    // The list of all timeSamples at which the collection attributes are 
    // authored.
    vector<UsdTimeCode> allTimes;

    VtIntArray defaultTargetFaceCounts, defaultTargetFaceIndices;
    if (targetFaceCountsAttr.Get(&defaultTargetFaceCounts, UsdTimeCode::Default()) or 
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
        if (not GetTargetFaceCounts(&faceCounts, time) or 
            not GetTargetFaceIndices(&faceIndices, time)) {
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

