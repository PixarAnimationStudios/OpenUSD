//
// Copyright 2019 Pixar
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
#include "data.h"
#include "pxr/usd/sdf/schema.h"
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

// Helper macro for many of our functions need to optionally set an output 
// VtValue when returning true. 
#define RETURN_TRUE_WITH_OPTIONAL_VALUE(val) \
    if (value) { *value = VtValue(val); } \
    return true;

// Helper function for getting the root prim path.
static const SdfPath &_GetRootPrimPath()
{
    static const SdfPath rootPrimPath("/Animation");
    return rootPrimPath;
}

TfTokenVector 
UsdAnimXData::_AnimXPrimData::GetAnimatedOpNames() const
{
  TfTokenVector names;
  for(auto& op: ops) {
    names.push_back(op.name);
  }
  std::cout << std::endl;
  return names;
}

bool 
UsdAnimXData::_AnimXPrimData::HasAnimatedOp(const TfToken& name) const
{
  for(const auto& op: ops) {
    if(op.name == name) return true;
  }
  return false;
}

const UsdAnimXData::_AnimXOpData* 
UsdAnimXData::_AnimXPrimData::GetAnimatedOp(const TfToken& name) const
{
  for(auto& op: ops) {
    if(op.name == name) return &op;
  }
  return NULL;
}

UsdAnimXData::_AnimXOpData* 
UsdAnimXData::_AnimXPrimData::GetMutableAnimatedOp(const TfToken& name)
{
  for(auto& op: ops) {
    if(op.name == name) return &op;
  }
  return NULL;
}

////////////////////////////////////////////////////////////////////////
// UsdAnimXData
/*static*/
UsdAnimXDataRefPtr 
UsdAnimXData::New()
{
    return TfCreateRefPtr(new UsdAnimXData());
}

UsdAnimXData::UsdAnimXData()
{
}

UsdAnimXData::~UsdAnimXData()
{
}

bool
UsdAnimXData::StreamsData() const
{
    return true;
}

bool 
UsdAnimXData::IsEmpty() const
{
    return _animXPrimDataMap.size() == 0;
}

bool
UsdAnimXData::HasSpec(const SdfPath& path) const
{
    std::cout << "ANIMX DATA HAS SPEC : " << path << std::endl;
    return GetSpecType(path) != SdfSpecTypeUnknown;
}

void
UsdAnimXData::EraseSpec(const SdfPath& path)
{
    /*
    _HashTable::iterator i = _data.find(path);
    if (!TF_VERIFY(i != _data.end(),
                   "No spec to erase at <%s>", path.GetText())) {
        return;
    }
    _data.erase(i);
    */
    
   TF_RUNTIME_ERROR("UsdAnimX file EraseSpec() not supported");
}

void
UsdAnimXData::MoveSpec(const SdfPath& oldPath, 
                                      const SdfPath& newPath)
{
    /*
    _HashTable::iterator old = _data.find(oldPath);
    if (!TF_VERIFY(old != _data.end(),
            "No spec to move at <%s>", oldPath.GetString().c_str())) {
        return;
    }
    bool inserted = _data.insert(std::make_pair(newPath, old->second)).second;
    if (!TF_VERIFY(inserted)) {
        return;
    }
    _data.erase(old);
    */
    TF_RUNTIME_ERROR("UsdAnimX file MoveSpec() not supported");
}

SdfSpecType
UsdAnimXData::GetSpecType(const SdfPath& path) const
{
    // All specs are generated.
    if (path.IsPropertyPath()) {

        if(_animXPrimDataMap.count(path.GetAbsoluteRootOrPrimPath())) {
            const _AnimXPrimData *data = 
                TfMapLookupPtr(_animXPrimDataMap, path.GetAbsoluteRootOrPrimPath());
            if(data->HasAnimatedOp(path.GetNameToken()))
                return SdfSpecTypeAttribute;
        }
    } else {
        // Special case for pseudoroot.
        if (path == SdfPath::AbsoluteRootPath()) {
            return SdfSpecTypePseudoRoot;
        }
        // All other valid prim spec paths are cached.
        if (_animXPrimDataMap.count(path)) {
            return SdfSpecTypePrim;
        }
    }
    
    return SdfSpecTypeUnknown;
}

void
UsdAnimXData::CreateSpec(const SdfPath& path,
                                        SdfSpecType specType)
{
  
    std::cout << "ANIMX CREATE SPEC : " << path << std::endl;
    if (!TF_VERIFY(specType != SdfSpecTypeUnknown)) {
        return;
    }
    if(specType == SdfSpecType::SdfSpecTypePrim)
      _primSpecPaths.insert(path);
    
    //TF_RUNTIME_ERROR("UsdAnimX file CreateSpec() not supported");
}

void
UsdAnimXData::_VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const
{
    // Visit the pseudoroot.
    if (!visitor->VisitSpec(*this, SdfPath::AbsoluteRootPath())) {
        return;
    }
    // Visit all the cached prim spec paths.
    for (const auto &path: _primSpecPaths) {
        if (!visitor->VisitSpec(*this, path)) {
            return;
        }
    }
    // Visit the property specs which exist only on animated prims.
    for (auto it : _animXPrimDataMap) {
        for (const TfToken &propertyName : it.second.GetAnimatedOpNames()) {
            if (!visitor->VisitSpec(
                    *this, it.first.AppendProperty(propertyName))) {
                return;
            }
        }
    }
}

void 
UsdAnimXData::SetRootPrimPath(const SdfPath& rootPrimPath)
{
  _rootPrimPath = rootPrimPath;
}

const SdfPath& 
UsdAnimXData::GetRootPrimPath() const
{
  return _rootPrimPath;
}


void 
UsdAnimXData::AddPrim(const SdfPath& path)
{
  _primSpecPaths.insert(path);
}

UsdAnimXData::_AnimXOpData* 
UsdAnimXData::AddOp(const SdfPath& primPath, const TfToken& name, 
    const TfToken& typeName, const size_t elementCount, const VtValue& defaultValue) 
{
  _AnimXPrimData& primData = _animXPrimDataMap[primPath];
  _AnimXOpData opData;
  opData.name = name;
  opData.typeName = typeName;
  opData.elementCount = elementCount;
  opData.dataType = defaultValue.GetType();
  opData.defaultValue = defaultValue;
  primData.ops.push_back(opData);
  return &primData.ops.back();
}

void 
UsdAnimXData::AddFCurve(const SdfPath& primPath, const TfToken& opName, 
    const UsdAnimXCurve& curve)
{
  _AnimXPrimData& primData = _animXPrimDataMap[primPath];
  std::cout << "ADD FCURVE : " << primPath << ": " << opName << std::endl;
  _AnimXOpData* opData = primData.GetMutableAnimatedOp(opName);
  if(opData) {
    opData->curves.push_back(curve);
  }
  else std::cout << opName << " NOT FOUND !!!" << std::endl;
}


void
UsdAnimXData::ComputeTimesSamples()
{
    for(int ts=1;ts<31;++ts)
      _animTimeSampleTimes.insert((double)ts);
}

bool 
UsdAnimXData::Has(const SdfPath& path, 
                  const TfToken &field,
                  SdfAbstractDataValue* value) const
{    
    if (value) {
        VtValue val;
        if (Has(path, field, &val)) {
            return value->StoreValue(val);
        }
        return false;
    } else {
        VtValue val;
        return (Has(path, field, &val));
    }
    return false;
    
}

bool 
UsdAnimXData::Has(const SdfPath& path, 
                  const TfToken & field, 
                  VtValue *value) const
{
    // If property spec, check property field
    if (path.IsPropertyPath()) {
        if (field == SdfFieldKeys->TypeName) {
            return _HasPropertyTypeNameValue(path, value);
        } else if (field == SdfFieldKeys->Default) {
            return _HasPropertyDefaultValue(path, value);
        } else if (field == SdfFieldKeys->TimeSamples) {
            // Only animated properties have time samples.
            if (_IsAnimatedProperty(path)) {
                // Will need to generate the full SdfTimeSampleMap with a 
                // time sample value for each discrete animated frame if the 
                // value of the TimeSamples field is requested. Use a generator
                // function in case we don't need to output the value as this
                // can be expensive.
                
                auto _MakeTimeSampleMap = [this, &path]() {
                    SdfTimeSampleMap sampleMap;
                    for (auto time : _animTimeSampleTimes) {
                         QueryTimeSample(path, time, &sampleMap[time]);
                    }
                    return sampleMap;
                };
                
                RETURN_TRUE_WITH_OPTIONAL_VALUE(_MakeTimeSampleMap());
                
                return false;
            }
        } 
    } else if (path == SdfPath::AbsoluteRootPath()) {
        // Special case check for the pseudoroot prim spec.
        if (field == SdfChildrenKeys->PrimChildren) {
            // Pseudoroot only has the root prim as a child
            static TfTokenVector rootChildren(
                {_GetRootPrimPath().GetNameToken()});
            RETURN_TRUE_WITH_OPTIONAL_VALUE(rootChildren);
        }
        // Default prim is always the root prim.
        if (field == SdfFieldKeys->DefaultPrim) {
            if (path == SdfPath::AbsoluteRootPath() ) {
                RETURN_TRUE_WITH_OPTIONAL_VALUE(_GetRootPrimPath().GetNameToken());
            }
        }
        // Start time code is always 0
        if (field == SdfFieldKeys->StartTimeCode) {
            if (path == SdfPath::AbsoluteRootPath() ) {
                RETURN_TRUE_WITH_OPTIONAL_VALUE(0.0);
            }
        }
        // End time code is always num frames - 1
        if (field == SdfFieldKeys->EndTimeCode) {
            if (path == SdfPath::AbsoluteRootPath() ) {
                RETURN_TRUE_WITH_OPTIONAL_VALUE(double(200));
            }
        }

    } else {
        // Otherwise check prim spec fields.
        if (field == SdfFieldKeys->Specifier) {
            // All our prim specs use the "over" specifier.
            if (_primSpecPaths.count(path)) {
                RETURN_TRUE_WITH_OPTIONAL_VALUE(SdfSpecifierOver);
            }
        }
        
        /*
        if (field == SdfChildrenKeys->PrimChildren) {
          
            if(path == _GetRootPrimPath()) {
                std::vector<TfToken> primChildNames;
                primChildNames.push_back(TfToken("cube"));
                
                RETURN_TRUE_WITH_OPTIONAL_VALUE(primChildNames);
            }
        }
        
        if (field == SdfChildrenKeys->PropertyChildren) {
            if(_animXPrimDataMap.count(path)) {
                const _AnimXPrimData *data =
                    TfMapLookupPtr(_animXPrimDataMap, path);
                RETURN_TRUE_WITH_OPTIONAL_VALUE(data->GetAnimatedOpNames());
            }
        }
        */
    }

    return false;
}
/*
bool
UsdAnimXData::HasSpecAndField(
    const SdfPath &path, const TfToken &fieldName,
    SdfAbstractDataValue *value, SdfSpecType *specType) const
{
    if (VtValue const *v =
        _GetSpecTypeAndFieldValue(path, fieldName, specType)) {
        return !value || value->StoreValue(*v);
    }
    return false;
}

bool
UsdAnimXData::HasSpecAndField(
    const SdfPath &path, const TfToken &fieldName,
    VtValue *value, SdfSpecType *specType) const
{
    if (VtValue const *v =
        _GetSpecTypeAndFieldValue(path, fieldName, specType)) {
        if (value) {
            *value = *v;
        }
        return true;
    }
    return false;
}
*/
 
VtValue
UsdAnimXData::Get(const SdfPath& path, 
                                 const TfToken & field) const
{
  std::cout << "ANIMX DATA GET : " << path <<  ":" << field << std::endl;
  /*
    if (const VtValue *value = _GetFieldValue(path, field)) {
        return *value;
    }*/
    return VtValue();
    /*
    VtValue value;
    _impl->Has(path, field, &value);
    return value;
    */
}

void 
UsdAnimXData::Set(const SdfPath& path, 
                                 const TfToken & field, const VtValue& value)
{
    std::cout << "ANIMX DATA SET : " << path <<  ":" << field << std::endl;
    /*
    TfAutoMallocTag2 tag("Sdf", "SdfData::Set");

    if (value.IsEmpty()) {
        Erase(path, field);
        return;
    }

    VtValue* newValue = _GetOrCreateFieldValue(path, field);
    if (newValue) {
        *newValue = value;
    }
    */
}

void 
UsdAnimXData::Set(const SdfPath& path, 
                                 const TfToken & field, 
                                 const SdfAbstractDataConstValue& value)
{
    /*
    TfAutoMallocTag2 tag("Sdf", "SdfData::Set");

    VtValue* newValue = _GetOrCreateFieldValue(path, field);
    if (newValue) {
        value.GetValue(newValue);
    }
    */
   std::cout << "ANIMX DATA ABSTRACT SET : " << path <<  ":" << field << std::endl;
}

void 
UsdAnimXData::Erase(const SdfPath& path, 
                                   const TfToken & field)
{
    std::cout << "ANIMX DATA ERASE : " << path <<  ":" << field << std::endl;
    /*
    _HashTable::iterator i = _data.find(path);
    if (i == _data.end()) {
        return;
    }
    
    _SpecData &spec = i->second;
    for (size_t j=0, jEnd = spec.fields.size(); j != jEnd; ++j) {
        if (spec.fields[j].first == field) {
            spec.fields.erase(spec.fields.begin()+j);
            return;
        }
    }
    */
}

std::vector<TfToken>
UsdAnimXData::List(const SdfPath& path) const
{
    if (path.IsPropertyPath()) {
        // For properties, check that it's a valid leaf prim property
        const _AnimXPrimData *data = 
          TfMapLookupPtr(_animXPrimDataMap, path.GetAbsoluteRootOrPrimPath());
        if (_animXPrimDataMap.count(path.GetAbsoluteRootOrPrimPath())) {
            // Include time sample field in the property is animated.
            static std::vector<TfToken> animPropFields(
                {SdfFieldKeys->TypeName,
                  SdfFieldKeys->Default,
                  SdfFieldKeys->TimeSamples});
            return animPropFields;
        }
    } else if (path == SdfPath::AbsoluteRootPath()) {
        // Pseudoroot fields.
        static std::vector<TfToken> pseudoRootFields(
            {SdfChildrenKeys->PrimChildren,
             SdfFieldKeys->DefaultPrim,
             SdfFieldKeys->StartTimeCode,
             SdfFieldKeys->EndTimeCode});
        return pseudoRootFields;
    } else if (_primSpecPaths.count(path)) {
        // Prim spec. Different fields for leaf and non-leaf prims.
        if (_animXPrimDataMap.count(path)) {
            static std::vector<TfToken> leafPrimFields(
                {SdfFieldKeys->Specifier,
                 SdfFieldKeys->TypeName,
                 SdfChildrenKeys->PropertyChildren});
            return leafPrimFields;
        } else {
            static std::vector<TfToken> nonLeafPrimFields(
                {SdfFieldKeys->Specifier,
                 SdfChildrenKeys->PrimChildren});
            return nonLeafPrimFields;
        }
    }

    static std::vector<TfToken> empty;
    return empty;
}

std::set<double>
UsdAnimXData::ListAllTimeSamples() const
{
    return _animTimeSampleTimes;
}

std::set<double>
UsdAnimXData::ListTimeSamplesForPath(const SdfPath& path) const
{
    if(_IsAnimatedProperty(path))
        return _animTimeSampleTimes;
    return std::set<double>();
}

bool
UsdAnimXData::GetBracketingTimeSamples(
    double time, double* tLower, double* tUpper) const
{
    // A time sample time will exist at each discrete integer frame for the 
    // duration of the generated animation and will already be cached.
    if (_animTimeSampleTimes.empty()) {
        return false;
    }

    // First time sample is always zero.
    if (time <= 0) {
        *tLower = *tUpper = 0;
        return true;
    }
    // Last time sample will alway be size - 1.
    if (time >= _animTimeSampleTimes.size() - 1) {
        *tLower = *tUpper = _animTimeSampleTimes.size() - 1;
        return true;
    }
    // Lower bound is the integer time. Upper bound will be the same unless the
    // time itself is non-integer, in which case it'll be the next integer time.
    *tLower = *tUpper = int(time);
    if (time > *tUpper) {
        *tUpper += 1.0;
    }
    return true;
}

size_t
UsdAnimXData::GetNumTimeSamplesForPath(
    const SdfPath& path) const
{
    if(_IsAnimatedProperty(path)) {
        return _animTimeSampleTimes.size();
    }
    return 0;
}

bool
UsdAnimXData::GetBracketingTimeSamplesForPath(
    const SdfPath& path, double time,
    double* tLower, double* tUpper) const
{
     // All animated properties use the same set of time samples.
    if (_IsAnimatedProperty(path)) {
        return GetBracketingTimeSamples(time, tLower, tUpper);
    }

    return false;
}

bool
UsdAnimXData::QueryTimeSample(const SdfPath& path, 
                                             double time, VtValue *value) const
{
    // Only leaf prim properties have time samples
    const _AnimXPrimData *primData =
        TfMapLookupPtr(_animXPrimDataMap, path.GetAbsoluteRootOrPrimPath());
    if (!primData) return false;

    if(path.GetNameToken() == TfToken("xformOpOrder"))return false;

    const _AnimXOpData* opData = primData->GetAnimatedOp(path.GetNameToken());
    if(!opData) return false;

    //double computedSize(opData->curves[0].evaluate(time));

    //RETURN_TRUE_WITH_OPTIONAL_VALUE(computedSize);

    std::cout << "OP TYPE NAME : " << opData->typeName << std::endl;
    std::cout << "OP DATA TYPE : " << opData->dataType.GetTypeName() << std::endl;
    std::cout << "OP NUM ELEMENTS : " << opData->elementCount << std::endl;

    /*
    if(opData->typeName == TfToken("double")) {
        switch((int)opData->elementCount) {
            case 1:
                return _Interpolate<double, 1>(&opData->curves[0], value, time);
            case 2:
                return _InterpolateVec2<GfVec2d>(&opData->curves[0], value, time);
            case 3:
                return _InterpolateVec3<GfVec3d>(&opData->curves[0], value, time);
            case 4:
                return _InterpolateVec4<GfVec4d>(&opData->curves[0], value, time);
            case 9:
                return _Interpolate<int, 9>(&opData->curves[0], value, time);
            case 16:
                return _Interpolate<int, 16>(&opData->curves[0], value, time);
            default:
              return false;
        }
    } else if(opData->typeName == TfToken("float")) {
        switch((int)opData->elementCount)
        {
            case 1:
                return _Interpolate<float, 1>(&opData->curves[0], value, time);
            case 2:
                return _InterpolateVec2<GfVec2f>(&opData->curves[0], value, time);
            case 3:
                return _InterpolateVec3<GfVec3f>(&opData->curves[0], value, time);
            case 4:
                return _InterpolateVec4<GfVec4f>(&opData->curves[0], value, time);
            case 9:
                return _Interpolate<double, 9>(&opData->curves[0], value, time);
            case 16:
                return _Interpolate<double, 16>(&opData->curves[0], value, time);
            default:
              return false;
        }
    } else if(opData->typeName == TfToken("int")) {
        switch((int)opData->elementCount)
        {
            case 1:
                return _Interpolate<int, 1>(&opData->curves[0], value, time);
            case 2:
                return _InterpolateVec2<GfVec2i>(&opData->curves[0], value, time);
            case 3:
                return _InterpolateVec3<GfVec3i>(&opData->curves[0], value, time);
            case 4:
                return _InterpolateVec4<GfVec4i>(&opData->curves[0], value, time);
            case 9:
                return _Interpolate<int, 9>(&opData->curves[0], value, time);
            case 16:
                return _Interpolate<int, 16>(&opData->curves[0], value, time);
            default:
              return false;
        }
    }
    */
    return false;
}

bool 
UsdAnimXData::QueryTimeSample(const SdfPath& path, 
                                             double time, 
                                             SdfAbstractDataValue* value) const
{ 
    std::cout << "ANIMX DATA ABSTRACT QUERY TIME SAMPLE " << std::endl;
    
    if (value) {
        VtValue val;
        if (QueryTimeSample(path, time, &val)) {
            return value->StoreValue(val);
        }
        return false;
    }
    return false;
}

void
UsdAnimXData::SetTimeSample(const SdfPath& path, 
                                           double time, const VtValue& value)
{
    /*
    if (value.IsEmpty()) {
        EraseTimeSample(path, time);
        return;
    }

    SdfTimeSampleMap newSamples;

    // Attempt to get a pointer to an existing timeSamples field.
    VtValue *fieldValue =
        _GetMutableFieldValue(path, SdfDataTokens->TimeSamples);

    // If we have one, swap it out so we can modify it.
    if (fieldValue && fieldValue->IsHolding<SdfTimeSampleMap>()) {
        fieldValue->UncheckedSwap(newSamples);
    }
    
    // Insert or overwrite into newSamples.
    newSamples[time] = value;

    // Set back into the field.
    if (fieldValue) {
        fieldValue->Swap(newSamples);
    } else {
        Set(path, SdfDataTokens->TimeSamples, VtValue::Take(newSamples));
    }*/
    TF_RUNTIME_ERROR("UsdAnimX file SetTimeSample() not supported");
}

void
UsdAnimXData::EraseTimeSample(const SdfPath& path, double time)
{
    TF_RUNTIME_ERROR("UsdAnimX file EraseTimeSample() not supported");
}

bool
UsdAnimXData::Write(
    const SdfAbstractDataConstPtr& data,
    const std::string& filePath,
    const std::string& comment)
{
  
   return true;
}

bool 
UsdAnimXData::_IsAnimatedProperty(
    const SdfPath &path) const
{
    // Check that it is a property id.
    if (!path.IsPropertyPath()) {
        return false;
    }
    /*
    // Check that its one of our animated property names.
    const _LeafPrimPropertyInfo *propInfo = 
        TfMapLookupPtr(*_LeafPrimProperties, path.GetNameToken());
    if (!(propInfo && propInfo->isAnimated)) {
        return false;
    }
    
    // Check that it belongs to a leaf prim.
    return TfMapLookupPtr(_animXPrimDataMap, path.GetAbsoluteRootOrPrimPath());
    */
   
    auto data = TfMapLookupPtr(_animXPrimDataMap, path.GetAbsoluteRootOrPrimPath());
    if(data) {
        for(auto& op: data->ops) {
            if(op.name == path.GetNameToken()) {
                return true;
            }
        }
    }
    return false;
}

bool 
UsdAnimXData::_HasPropertyDefaultValue(
    const SdfPath &path, VtValue *value) const
{
    // Check that it is a property id.
    if (!path.IsPropertyPath()) {
        return false;
    }

    // Check that it belongs to an animated prim before getting the default value
    const _AnimXPrimData *data = 
        TfMapLookupPtr(_animXPrimDataMap, path.GetAbsoluteRootOrPrimPath());

    if (data) {
        if (value) {
            *value = data->GetAnimatedOp(TfToken("xformOpOrder"))->defaultValue;
        }
        return true;
    }

    return false;
}

bool 
UsdAnimXData::_HasPropertyTypeNameValue(
    const SdfPath &path, VtValue *value) const
{
    // Check that it is a property id.
    if (!path.IsPropertyPath()) {
        return false;
    }

    // Check that it belongs to a animated prim before getting the type name value
    const _AnimXPrimData *val = 
        TfMapLookupPtr(_animXPrimDataMap, path.GetAbsoluteRootOrPrimPath());
    if (val) {
        if (value) {
            //*value = VtValue(propInfo->typeName);
        }
        return true;
    }

    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
