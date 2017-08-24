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
#include "pxr/usd/usdGeom/faceSetAPI.h"

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
    (isPartition)
    (faceCounts)
    (faceIndices)
    (binding)
);

/* virtual */
UsdGeomFaceSetAPI::~UsdGeomFaceSetAPI()
{ }

/* virtual */
bool
UsdGeomFaceSetAPI::_IsCompatible(const UsdPrim &prim) const
{
    return GetPrim() && _GetIsPartitionAttr();
}

/* static */
UsdGeomFaceSetAPI 
UsdGeomFaceSetAPI::Create(const UsdPrim &prim, 
                          const TfToken &setName,
                          bool isPartition /* = true */)
{
    UsdGeomFaceSetAPI faceSet(prim, setName);
    faceSet.SetIsPartition(isPartition);
    return faceSet;
}

/* static */
UsdGeomFaceSetAPI 
UsdGeomFaceSetAPI::Create(const UsdSchemaBase &schemaObj, 
                          const TfToken &setName,
                          bool isPartition /* = true */)
{
    return UsdGeomFaceSetAPI::Create(schemaObj.GetPrim(), setName, isPartition);
}

/* static */
vector<UsdGeomFaceSetAPI> 
UsdGeomFaceSetAPI::GetFaceSets(const UsdPrim &prim)
{
    vector<UsdGeomFaceSetAPI> result;

    vector<UsdProperty> faceSetProperties = prim.GetPropertiesInNamespace(
        UsdGeomTokens->faceSet);
    TF_FOR_ALL(propIt, faceSetProperties) {
        const UsdProperty &prop = *propIt;
        
        if (prop.GetBaseName() != _tokens->isPartition) 
            continue;

        vector<string> nameTokens = prop.SplitName();
        if (nameTokens.size() == 3)
            result.push_back(UsdGeomFaceSetAPI(prim, TfToken(nameTokens[1])));
    }
    return result;
}

/* static */
vector<UsdGeomFaceSetAPI> 
UsdGeomFaceSetAPI::GetFaceSets(const UsdSchemaBase &schemaObj)
{
    return UsdGeomFaceSetAPI::GetFaceSets(schemaObj.GetPrim());
}

TfToken 
UsdGeomFaceSetAPI::_GetFaceSetPropertyName(const TfToken &baseName) const
{
    return TfToken(UsdGeomTokens->faceSet.GetString() + ":" + 
                   _setName.GetString()  + ":" + 
                   baseName.GetString());
}

UsdAttribute 
UsdGeomFaceSetAPI::_GetIsPartitionAttr(bool create) const
{
    const TfToken &propName = _GetFaceSetPropertyName(_tokens->isPartition);
    return create ? CreateIsPartitionAttr()
                  : GetPrim().GetAttribute(propName);
}

UsdAttribute 
UsdGeomFaceSetAPI::_GetFaceCountsAttr(bool create) const
{
    const TfToken &propName = _GetFaceSetPropertyName(_tokens->faceCounts);
    return create ? CreateFaceCountsAttr()
                  : GetPrim().GetAttribute(propName);
}

UsdAttribute 
UsdGeomFaceSetAPI::_GetFaceIndicesAttr(bool create) const
{
    const TfToken &propName = _GetFaceSetPropertyName(_tokens->faceIndices);
    return create ? CreateFaceIndicesAttr()
                  : GetPrim().GetAttribute(propName);
}

UsdRelationship 
UsdGeomFaceSetAPI::_GetBindingTargetsRel(bool create) const
{
    const TfToken &relName = _GetFaceSetPropertyName(_tokens->binding);
    return create ? GetPrim().CreateRelationship(relName, /* custom */ false) 
                  : GetPrim().GetRelationship(relName);
}

UsdAttribute 
UsdGeomFaceSetAPI::GetIsPartitionAttr() const
{
    return _GetIsPartitionAttr();
}

UsdAttribute 
UsdGeomFaceSetAPI::CreateIsPartitionAttr(const VtValue &defaultValue,
                                         bool writeSparsely) const
{
    const TfToken &propName = _GetFaceSetPropertyName(_tokens->isPartition);
    return UsdSchemaBase::_CreateAttr(propName,
                                      SdfValueTypeNames->Bool,
                                      /* custom = */ false,
                                      SdfVariabilityUniform,
                                      defaultValue,
                                      writeSparsely);
}

UsdAttribute 
UsdGeomFaceSetAPI::GetFaceCountsAttr() const
{
    return _GetFaceCountsAttr();
}

UsdAttribute 
UsdGeomFaceSetAPI::CreateFaceCountsAttr(const VtValue &defaultValue,
                                        bool writeSparsely) const
{
    const TfToken &propName = _GetFaceSetPropertyName(_tokens->faceCounts);
    return UsdSchemaBase::_CreateAttr(propName,
                                      SdfValueTypeNames->IntArray,
                                      /* custom = */ false,
                                      SdfVariabilityVarying,
                                      defaultValue,
                                      writeSparsely);
}

UsdAttribute 
UsdGeomFaceSetAPI::GetFaceIndicesAttr() const
{
    return _GetFaceIndicesAttr();
}

UsdAttribute 
UsdGeomFaceSetAPI::CreateFaceIndicesAttr(const VtValue &defaultValue,
                                        bool writeSparsely) const
{
    const TfToken &propName = _GetFaceSetPropertyName(_tokens->faceIndices);
    return UsdSchemaBase::_CreateAttr(propName,
                                      SdfValueTypeNames->IntArray,
                                      /* custom = */ false,
                                      SdfVariabilityVarying,
                                      defaultValue,
                                      writeSparsely);
}

UsdRelationship 
UsdGeomFaceSetAPI::GetBindingTargetsRel() const
{
    return _GetBindingTargetsRel();
}

UsdRelationship
UsdGeomFaceSetAPI::CreateBindingTargetsRel() const
{
    return _GetBindingTargetsRel(/* create */ true);
}

static
bool _ContainsDuplicates(VtIntArray array)
{
    std::sort(array.begin(), array.end());
    return std::unique(array.begin(), array.end()) != array.end();
}

static string
_Stringify(const UsdTimeCode &time) {
    return time.IsDefault() ? string("DEFAULT") : TfStringify(time.GetValue());
}

bool 
UsdGeomFaceSetAPI::Validate(string *reason) const
{
    bool isPartition = GetIsPartition();

    SdfPathVector bindings;
    bool hasBinding = GetBindingTargets(&bindings);

    UsdAttribute faceIndicesAttr = _GetFaceIndicesAttr();
    if (!faceIndicesAttr) {
        *reason += "Could not get the faceIndices attribute.\n";
        return false;
    }

    UsdAttribute faceCountsAttr = _GetFaceCountsAttr();
    if (!faceCountsAttr) {
        *reason += "Could not get the faceCounts attribute.\n";
        return true;
    }

    // The list of all timeSamples at which the faceSet attributes are 
    // authored.
    vector<UsdTimeCode> allTimes;

    VtIntArray defaultFaceIndices, defaultFaceCounts;    
    // Check if faceIndices or faceCounts is authored at time=default.
    if (faceIndicesAttr.Get(&defaultFaceIndices) ||
        faceCountsAttr.Get(&defaultFaceCounts)) {
        allTimes.push_back(UsdTimeCode::Default());
    }


    bool isValid = true;
    vector<double> fiTimes, fcTimes;
    set<double> allTimeSamples;
    if (faceIndicesAttr.GetTimeSamples(&fiTimes))
        allTimeSamples.insert(fiTimes.begin(), fiTimes.end());

    if (faceCountsAttr.GetTimeSamples(&fcTimes))
        allTimeSamples.insert(fcTimes.begin(), fcTimes.end());

    allTimes.reserve(allTimes.size() + allTimeSamples.size());
    TF_FOR_ALL(it, allTimeSamples) {
        allTimes.push_back(UsdTimeCode(*it));
    }

    size_t prevNumFaceCounts = std::numeric_limits<size_t>::max();
    TF_FOR_ALL(timeIt, allTimes) {
        const UsdTimeCode &time = *timeIt;

        VtIntArray faceIndices;
        if (GetFaceIndices(&faceIndices, time)) {
            if (isPartition && _ContainsDuplicates(faceIndices)) {
                isValid = false;
                if (reason) {
                    *reason += TfStringPrintf("isPartition is true, "
                        "but faceIndices contains duplicates at "
                        "time=%s.\n", _Stringify(time).c_str());
                }
            }
        }

        VtIntArray faceCounts;
        if (!GetFaceCounts(&faceCounts, time)) {           
            isValid = false;
            if (reason) {
                *reason += TfStringPrintf("Could not get faceCounts at "
                    "time %s.\n", 
                    _Stringify(time).c_str());
            }
        } else {

            if (prevNumFaceCounts != std::numeric_limits<size_t>::max() && 
                faceCounts.size() != prevNumFaceCounts) 
            {
                isValid = false;
                if (reason) {
                    *reason += "Number of elements in faceCounts is not "
                    "constant over all timeSamples.\n";
                }
            }
            prevNumFaceCounts = faceCounts.size();

            size_t sum = 0;
            TF_FOR_ALL(faceCountsIt, faceCounts) {
                sum += *faceCountsIt;
            }
            if (faceIndices.size() != sum) {
                isValid = false;
                if (reason) {
                    *reason += TfStringPrintf("The sum of all faceCounts "
                        "(%ld) does not match the length of the "
                        "faceIndices array (%ld) at time %s.\n", sum, 
                        faceIndices.size(), _Stringify(time).c_str());
                }
            }

            if (hasBinding) {
                if (!bindings.empty() && 
                    faceCounts.size() != bindings.size()) 
                {
                    isValid = false;
                    if (reason) {
                        *reason += TfStringPrintf("Length of faceCounts array "
                            "(%ld) does not match the number of bindings (%ld) "
                            "at frame %s.\n", faceCounts.size(), 
                            bindings.size(), _Stringify(time).c_str());
                    }
                }
            }
        }
    }

    return isValid;
}


bool 
UsdGeomFaceSetAPI::GetIsPartition() const
{
    if (UsdAttribute attr = GetIsPartitionAttr()) {
        bool isPartition = false;
        attr.Get(&isPartition);
        return isPartition;
    }
    return false;
}

bool 
UsdGeomFaceSetAPI::SetIsPartition(bool isPartition) const
{
    return _GetIsPartitionAttr(/*create*/ true).Set(isPartition);
}

bool 
UsdGeomFaceSetAPI::GetFaceCounts(VtIntArray *faceCounts, 
                                 const UsdTimeCode &time) const
{
    UsdAttribute attr = _GetFaceCountsAttr();
    return attr && attr.Get(faceCounts, time);
}

bool 
UsdGeomFaceSetAPI::SetFaceCounts(const VtIntArray &faceCounts, 
                                 const UsdTimeCode &time) const
{
    return _GetFaceCountsAttr(/*create*/ true).Set(faceCounts, time);
}

bool 
UsdGeomFaceSetAPI::GetFaceIndices(VtIntArray *faceIndices, 
                                  const UsdTimeCode &time) const
{
    UsdAttribute attr = _GetFaceIndicesAttr();
    return attr.Get(faceIndices, time);
}

bool 
UsdGeomFaceSetAPI::SetFaceIndices(const VtIntArray &faceIndices,
                                  const UsdTimeCode &time) const
{
    return _GetFaceIndicesAttr(/*create*/ true).Set(faceIndices, time);
}


bool 
UsdGeomFaceSetAPI::GetBindingTargets(SdfPathVector *bindings) const
{
    UsdRelationship rel = _GetBindingTargetsRel();
    return rel && rel.GetForwardedTargets(bindings);
}

bool 
UsdGeomFaceSetAPI::SetBindingTargets(const SdfPathVector &bindings) const
{
    return _GetBindingTargetsRel(/*create*/ true).SetTargets(bindings);
}

bool 
UsdGeomFaceSetAPI::AppendFaceGroup(const VtIntArray &indices,
                                   const SdfPath &bindingTarget,
                                   const UsdTimeCode &time) const
{
    bool hasFaceGroupsAtTime = true;

    // Check if there are faceCounts authored at the given time ordinate. 
    // If yes, then we append the face group to the existing set of face groups.
    // If not, then we create a new list of face groups at the given gime 
    // ordinate and add this one.
    // 
    if (time != UsdTimeCode::Default()) {
        UsdAttribute faceCountsAttr = GetFaceCountsAttr();
        double lower=0., upper=0.;
        bool hasTimeSamples=false;
        if (faceCountsAttr.GetBracketingTimeSamples(time.GetValue(), 
            &lower, &upper, &hasTimeSamples)) 
        {
            hasFaceGroupsAtTime = (lower==upper && lower==time.GetValue());
        }
    }

    VtIntArray faceCounts, faceIndices;
    if (hasFaceGroupsAtTime) {
        GetFaceCounts(&faceCounts, time);
        GetFaceIndices(&faceIndices, time);
    }

    faceCounts.push_back(indices.size());
    faceIndices.reserve(faceIndices.size() + indices.size());
    TF_FOR_ALL(indicesIt, indices) {
        faceIndices.push_back(*indicesIt);
    }

    bool success = true;
    SdfPathVector bindingTargets;
    GetBindingTargets(&bindingTargets);
    if (bindingTarget.IsEmpty() && !bindingTargets.empty()) {
        TF_CODING_ERROR("No binding target was provided for a face group being "
                        "added to a face set ('%s') containing existing binding"
                        " targets.", _setName.GetText());
        success = false;
    } else if (!bindingTarget.IsEmpty() && 
        bindingTargets.empty() && 
        faceCounts.size() > 1) 
    {
        TF_CODING_ERROR("Non-empty binding target was provided for a face group"
                        " being added to a non-empty face set ('%s') containing"
                        " no binding targets.", _setName.GetText());
        success = false;
    } else if (!bindingTarget.IsEmpty()) {
        bindingTargets.push_back(bindingTarget);
        success &= SetBindingTargets(bindingTargets);
    }

    return success && SetFaceCounts(faceCounts, time) && 
           SetFaceIndices(faceIndices, time);
}

PXR_NAMESPACE_CLOSE_SCOPE

