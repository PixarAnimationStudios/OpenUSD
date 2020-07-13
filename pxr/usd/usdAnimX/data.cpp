//
// Copyright 2020 benmalartre
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
#include "pxr/usd/usdAnimX/data.h"
#include "pxr/usd/usdAnimX/types.h"
#include "pxr/usd/usdAnimX/tokens.h"
#include "pxr/usd/usdAnimX/curve.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/spec.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

// Helper macro for many of our functions need to optionally set an output 
// VtValue when returning true. 
#define RETURN_TRUE_WITH_OPTIONAL_VALUE(val) \
    if (value) { *value = VtValue(val); } \
    return true;

TfTokenVector 
UsdAnimXPrimData::GetAnimatedOpNames() const
{
    TfTokenVector names;
    for(auto& op: ops) {
        names.push_back(op.target);
    }
    return names;
}

bool 
UsdAnimXPrimData::HasAnimatedOp(const TfToken& name) const
{
    for(const auto& op: ops) {
        if(op.target == name) return true;
    }
    return false;
}

std::set<double> 
UsdAnimXPrimData::ComputeTimeSamples() const
{
    std::set<double> samples;
    for(const auto& op: ops) {
        for(const auto& curve: op.curves) {
            std::set<double> curveSamples = curve.computeSamples();
            if(curveSamples.size())
              samples.insert(curveSamples.begin(), curveSamples.end());
        }
    }
    return samples;
}

const UsdAnimXOpData* 
UsdAnimXPrimData::GetAnimatedOp(const TfToken& name) const
{
    for(auto& op: ops) {
        if(op.target == name) return &op;
    }
    return NULL;
}

UsdAnimXOpData* 
UsdAnimXPrimData::GetMutableAnimatedOp(const TfToken& name)
{
    for(auto& op: ops) {
        if(op.target == name) return &op;
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
    return _animatedPrimDatas.size() == 0;
}

bool
UsdAnimXData::HasSpec(const SdfPath& path) const
{
    return GetSpecType(path) != SdfSpecTypeUnknown;
}

bool 
UsdAnimXData::HasSpecAndField(const SdfPath &path, const TfToken &fieldName,
    SdfAbstractDataValue *value, SdfSpecType *specType) const
{
    *specType = GetSpecType(path);
    return *specType != SdfSpecTypeUnknown && Has(path, fieldName, value);
}

bool 
UsdAnimXData::HasSpecAndField(const SdfPath &path, const TfToken &fieldName,
    VtValue *value, SdfSpecType *specType) const
{
    *specType = GetSpecType(path);
    return *specType != SdfSpecTypeUnknown && Has(path, fieldName, value);
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
        SdfPath primPath = path.GetAbsoluteRootOrPrimPath();
        if(_animatedPrimDatas.count(primPath)) {
            const UsdAnimXPrimData *data = 
                TfMapLookupPtr(_animatedPrimDatas, primPath);
            if(data->HasAnimatedOp(path.GetNameToken())) {
                return SdfSpecTypeAttribute;
            }
        }
    } else {
        // Special case for pseudoroot.
        if (path == SdfPath::AbsoluteRootPath()) {
            return SdfSpecTypePseudoRoot;
        }
        // All other valid prim spec paths are cached.
        if (_primPaths.count(path)) {
            return SdfSpecTypePrim;
        }
    }
    
    return SdfSpecTypeUnknown;
}

void
UsdAnimXData::CreateSpec(const SdfPath& path,
                                        SdfSpecType specType)
{
    if (!TF_VERIFY(specType != SdfSpecTypeUnknown)) {
        return;
    }
    if(specType == SdfSpecType::SdfSpecTypePrim)
        _primPaths.insert(path);
}

void
UsdAnimXData::_VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const
{
    // Visit the pseudoroot.
    if (!visitor->VisitSpec(*this, SdfPath::AbsoluteRootPath())) {
        return;
    }
    // Visit all the cached prim spec paths.
    for (const auto &path: _primPaths) {
        if (!visitor->VisitSpec(*this, path)) {
            return;
        }
    }
    // Visit the property specs which exist only on animated prims.
    for (auto it : _animatedPrimDatas) {
        for (const TfToken &propertyName : it.second.GetAnimatedOpNames()) {
            if (!visitor->VisitSpec(
                    *this, it.first.AppendProperty(propertyName))) {
                return;
            }
        }
    }
}

void 
UsdAnimXData::SetRootPrimPaths(const SdfPathVector& rootPrimPaths)
{
  _rootPrimPaths = rootPrimPaths;
}

const SdfPathVector& 
UsdAnimXData::GetRootPrimPaths() const
{
  return _rootPrimPaths;
}


void 
UsdAnimXData::AddPrim(const SdfPath& primPath)
{
    if(_primPaths.find(primPath) == _primPaths.end())
        _primPaths.insert(primPath);
}

void
UsdAnimXData::AddOp(const SdfPath& primPath, const UsdAnimXOpDesc &op) 
{
    UsdAnimXPrimData& primData = _animatedPrimDatas[primPath];
    for(const auto &opData: primData.ops) {
        if(opData.target == op.target) return;
    }
    UsdAnimXOpData opData;
    opData.target = op.target;
    if(op.dataType == UsdAnimXValueTypeTokens->bool_) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateBool;
    } else if(op.dataType == UsdAnimXValueTypeTokens->int_) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateInt;
    } else if(op.dataType == UsdAnimXValueTypeTokens->half_) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateHalf;
    } else if(op.dataType == UsdAnimXValueTypeTokens->float_) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateFloat;
    } else if(op.dataType == UsdAnimXValueTypeTokens->double_) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateDouble;
    } else if(op.dataType == UsdAnimXValueTypeTokens->half2_) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateVector2h;
    } else if(op.dataType == UsdAnimXValueTypeTokens->float2_) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateVector2f;
    } else if(op.dataType == UsdAnimXValueTypeTokens->double2_) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateVector2d;
    } else if(op.dataType == UsdAnimXValueTypeTokens->half3_) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateVector3h;
    } else if(op.dataType == UsdAnimXValueTypeTokens->float3_) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateVector3f;
    } else if(op.dataType == UsdAnimXValueTypeTokens->double3_) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateVector3d;
    } else if(op.dataType == UsdAnimXValueTypeTokens->half4_) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateVector4h;
    } else if(op.dataType == UsdAnimXValueTypeTokens->float4_) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateVector4f;
    } else if(op.dataType == UsdAnimXValueTypeTokens->double4_) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateVector4d;
    } else if(op.dataType == UsdAnimXValueTypeTokens->quath_) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateQuath;
    } else if(op.dataType == UsdAnimXValueTypeTokens->quatf_) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateQuatf;
    } else if(op.dataType == UsdAnimXValueTypeTokens->quatd_) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateQuatd;
    } else if(op.dataType == UsdAnimXValueTypeTokens->half_Array) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateHalfArray;
    } else if(op.dataType == UsdAnimXValueTypeTokens->float_Array) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateFloatArray;
    } else if(op.dataType == UsdAnimXValueTypeTokens->double_Array) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateDoubleArray;
    } else if(op.dataType == UsdAnimXValueTypeTokens->half2_Array) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateVector2hArray;
    } else if(op.dataType == UsdAnimXValueTypeTokens->float2_Array) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateVector2fArray;
    } else if(op.dataType == UsdAnimXValueTypeTokens->double2_Array) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateVector2dArray;
    } else if(op.dataType == UsdAnimXValueTypeTokens->half3_Array) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateVector3hArray;
    } else if(op.dataType == UsdAnimXValueTypeTokens->float3_Array) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateVector3fArray;
    } else if(op.dataType == UsdAnimXValueTypeTokens->double3_Array) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateVector3dArray;
    } else if(op.dataType == UsdAnimXValueTypeTokens->half4_Array) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateVector4hArray;
    } else if(op.dataType == UsdAnimXValueTypeTokens->float4_Array) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateVector4fArray;
    } else if(op.dataType == UsdAnimXValueTypeTokens->double4_Array) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateVector4dArray;
    } else if(op.dataType == UsdAnimXValueTypeTokens->quath_Array) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateQuathArray;
    } else if(op.dataType == UsdAnimXValueTypeTokens->quatf_Array) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateQuatfArray;
    } else if(op.dataType == UsdAnimXValueTypeTokens->quatd_Array) {
        opData.func = (InterpolateFunc)UsdAnimXInterpolateQuatdArray;
    } else {
        opData.func = NULL;
    }
    
    opData.defaultValue = op.defaultValue;
    primData.ops.push_back(opData);
}

void 
UsdAnimXData::AddFCurve(const SdfPath& primPath, const TfToken& opName, 
    const UsdAnimXCurveDesc& desc)
{
    UsdAnimXPrimData &primData = _animatedPrimDatas[primPath];
    UsdAnimXOpData *opData = primData.GetMutableAnimatedOp(opName);
    if(opData) {
        opData->curves.push_back(UsdAnimXCurve(desc));
    }
}

void
UsdAnimXData::GetAnimatedPrims(std::vector<SdfPath>& paths)
{
  for(auto& primData: _animatedPrimDatas) {
    paths.push_back(primData.first);
  }
}

void
UsdAnimXData::GetCurves(const SdfPath& primPath, 
    TfHashMap<SdfPath, std::vector<UsdAnimXCurve*>, SdfPath::Hash>& io)
{
    UsdAnimXPrimData *primData = TfMapLookupPtr(_animatedPrimDatas, primPath);
    if(primData) {
        for(auto& opData: primData->ops) {
            std::vector<UsdAnimXCurve*>& curves = 
                io[primPath.AppendProperty(opData.target)];
            for(auto& curve: opData.curves) {
                curves.push_back(&curve);
            }
        }
    }
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
        if (field == SdfFieldKeys->Default) {
            return _HasPropertyDefaultValue(path, value);
        } else if (field == SdfFieldKeys->TypeName) {
            return _HasPropertyTypeNameValue(path, value);      
        }/*else if (field == SdfFieldKeys->TimeSamples) {
            // Only animated properties have time samples.
            if (_HasAnimatedProperty(path)) {
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
        } */
    } else if (path == SdfPath::AbsoluteRootPath()) {
        // Special case check for the pseudoroot prim spec.
        if (field == SdfChildrenKeys->PrimChildren) {
            // Pseudoroot only has the root prim as a child
            TfTokenVector rootChildren;
            for(const auto& rootPath: _rootPrimPaths) {
                rootChildren.push_back(rootPath.GetNameToken());
            }
            /*(
                {_rootPrimPath.GetNameToken()});*/
            RETURN_TRUE_WITH_OPTIONAL_VALUE(rootChildren);
        }
        // Default prim is always the first root prim.
        if (field == SdfFieldKeys->DefaultPrim) {
            if (path == SdfPath::AbsoluteRootPath() && 
                _rootPrimPaths.size()) {
                RETURN_TRUE_WITH_OPTIONAL_VALUE(_rootPrimPaths[0].GetNameToken());
            } else {
                return false;
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
            if (_primPaths.count(path)) {
                RETURN_TRUE_WITH_OPTIONAL_VALUE(SdfSpecifierOver);
            }
        }
        
        if (field == SdfChildrenKeys->PrimChildren) {
          /*
            if(path == _rootPrimPath) {
                std::vector<TfToken> primChildNames;
                primChildNames.push_back(TfToken("cube"));
                RETURN_TRUE_WITH_OPTIONAL_VALUE(primChildNames);
            }
          */
        }
        
        if (field == SdfChildrenKeys->PropertyChildren) {
            if(_animatedPrimDatas.count(path.GetPrimPath())) {
                const UsdAnimXPrimData *data =
                    TfMapLookupPtr(_animatedPrimDatas, path);
                RETURN_TRUE_WITH_OPTIONAL_VALUE(data->GetAnimatedOpNames());
            }
        }
    }

    return false;
}
 
VtValue
UsdAnimXData::Get(const SdfPath& path, 
                                 const TfToken & field) const
{
    VtValue value;
    Has(path, field, &value);
    return value;
}

void 
UsdAnimXData::Set(const SdfPath& path, 
                                 const TfToken & field, const VtValue& value)
{
    SdfPath primPath = path.GetPrimPath();
    UsdAnimXPrimData *primData = 
          TfMapLookupPtr(_animatedPrimDatas, primPath);
    if(!primData) {
        AddPrim(primPath);
        primData = &_animatedPrimDatas[primPath];
    }
    
    
    if(!_HasAnimatedProperty(primPath.AppendProperty(field))) {
        UsdAnimXOpDesc opDesc;
        opDesc.name = _GetOpName(field);
        opDesc.target = field;
        opDesc.defaultValue = value;
        opDesc.dataType = AnimXGetTokenFromSdfValueTypeName(value.GetType());
        AddOp(primPath, opDesc);
        
    }
    UsdAnimXOpData *opData = primData->GetMutableAnimatedOp(field);
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
   std::cout << "ANIMX DATA ABSTRACT SET : " << path <<  "." << field << std::endl;
}

void 
UsdAnimXData::Erase(const SdfPath& path, 
                                   const TfToken & field)
{
    std::cout << "ANIMX DATA ERASE : " << path <<  "." << field << std::endl;
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
    std::cout << "### LIST " << path << std::endl;
    if (path.IsPropertyPath()) {
        // For properties, check that it's a valid animated prim property
        const UsdAnimXPrimData *data = 
          TfMapLookupPtr(_animatedPrimDatas, path.GetAbsoluteRootOrPrimPath());
        if (_animatedPrimDatas.count(path.GetAbsoluteRootOrPrimPath())) {
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
    } else if (_primPaths.count(path)) {
        // Prim spec. Different fields for leaf and non-leaf prims.
        if (_animatedPrimDatas.count(path)) {
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
    std::vector<double> perFrameSamples(2);
    perFrameSamples[0] = -0.1;
    perFrameSamples[1] = 0.2;
    VtValue value;
    Has( SdfPath::AbsoluteRootPath(), 
      SdfFieldKeys->StartTimeCode, &value);
    double startTimeCode = value.Get<double>();
    std::cout << "START TIME CODE : " << value << std::endl;
    Has( SdfPath::AbsoluteRootPath(), 
      SdfFieldKeys->EndTimeCode, &value);
    double endTimeCode = value.Get<double>();
    std::cout << "END TIME CODE : " << value << std::endl;
    /*
    Has( SdfPath::AbsoluteRootPath(), 
      SdfFieldKeys->TimeSamples, &value);
    std::cout << "TimeSamples : " << value << std::endl;
    */
    //return _animTimeSampleTimes;
    std::set<double> samples;
    /*
    for(const auto& prim: _animatedPrimDatas) {
        //std::set<double> primSamples = prim.second.ComputeTimeSamples();
        //samples.insert(primSamples.begin(), primSamples.end());
    }
    */
   for(size_t s=0;s<endTimeCode - startTimeCode; ++s) {
     samples.insert(startTimeCode + s + perFrameSamples[0]);
     samples.insert(startTimeCode + s + perFrameSamples[1]);
   }
    return samples;
}

std::set<double>
UsdAnimXData::ListTimeSamplesForPath(const SdfPath& path) const
{
    std::vector<double> perFrameSamples(1);
    perFrameSamples[0] = -0;
    std::cout << "LIST TIME SAMPELS FOR PATh :" << path << std::endl;
    VtValue value;
    Has( SdfPath::AbsoluteRootPath(), 
      SdfFieldKeys->StartTimeCode, &value);
    double startTimeCode = value.Get<double>();
    std::cout << "START TIME CODE : " << value << std::endl;
    Has( SdfPath::AbsoluteRootPath(), 
      SdfFieldKeys->EndTimeCode, &value);
    double endTimeCode = value.Get<double>();
    std::cout << "END TIME CODE : " << value << std::endl;
    if(_HasAnimatedProperty(path)) {
        std::set<double> samples;
   
        for(size_t s=0;s<endTimeCode - startTimeCode; ++s) {
          for(size_t t=0;t<perFrameSamples.size();++t) {
            samples.insert(startTimeCode + s + perFrameSamples[t]);
          }
        }
        return samples;
    }
    return std::set<double>();
}

bool
UsdAnimXData::GetBracketingTimeSamples(
    double time, double* tLower, double* tUpper) const
{
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
    if(_HasAnimatedProperty(path)) {
        return 65535;
    }
    return 0;
}

bool
UsdAnimXData::GetBracketingTimeSamplesForPath(
    const SdfPath& path, double time,
    double* tLower, double* tUpper) const
{
     // All animated properties have infinite time samples
    if (_HasAnimatedProperty(path)) {
        return GetBracketingTimeSamples(time, tLower, tUpper);
    }
    return false;
}

bool
UsdAnimXData::QueryTimeSample(const SdfPath& path, 
                                             double time, VtValue *value) const
{
    // Only animated prim properties have time samples
    const UsdAnimXPrimData *primData =
        TfMapLookupPtr(_animatedPrimDatas, path.GetPrimPath());
    if (!primData) return false;
    const UsdAnimXOpData *opData = 
        primData->GetAnimatedOp(path.GetNameToken());
    if(opData && opData->func) {
        return opData->func(opData->curves, value, time, 1);
    }
    return false;
}

bool 
UsdAnimXData::QueryTimeSample(const SdfPath& path, 
                                             double time, 
                                             SdfAbstractDataValue* value) const
{     
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
    std::cout << "ANIMX SET TIME SAMPLE : " << time << ": " << value << std::endl; 
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

std::vector<UsdAnimXPrimDesc> 
UsdAnimXData::BuildDescription() const
{
    SdfPathVector sortedPaths(_primPaths.begin(), _primPaths.end());
    std::sort(sortedPaths.begin(), sortedPaths.end());

    std::vector<UsdAnimXPrimDesc> rootPrims;
    TfHashMap<SdfPath, UsdAnimXPrimDesc*, SdfPath::Hash> insertedPrimDatas;

    for(auto& sortedPath: sortedPaths) {
        UsdAnimXPrimDesc primDesc;
        primDesc.name = sortedPath.GetNameToken();
        const UsdAnimXPrimData *primData =
            TfMapLookupPtr(_animatedPrimDatas, sortedPath);
        if (primData) {
            for(const auto& op: primData->ops) {
                UsdAnimXOpDesc opDesc;
                opDesc.name = _GetOpName(op.target);
                opDesc.target = op.target;
                opDesc.dataType = 
                    AnimXGetSerializationTypeNameFromSdfValueTypeName(
                        op.defaultValue.GetType());
                opDesc.defaultValue = op.defaultValue;
                primDesc.ops.push_back(opDesc);
                UsdAnimXOpDesc *currentOp = &primDesc.ops.back();
                for(auto& curve: op.curves) {
                    UsdAnimXCurveDesc curveDesc;
                    curveDesc.name = TfToken(curve.getName());
                    UsdAnimXKeyframe keyframe;
                    for(size_t k=0;k<curve.keyframeCount();++k) {
                        curve.keyframeAtIndex(k, keyframe);
                        UsdAnimXKeyframeDesc keyframeDesc = keyframe.GetDesc();
                        curveDesc.keyframes.push_back(keyframeDesc);
                    }
                    curveDesc.preInfinityType = 
                        _ResolveInfinityType(curve.preInfinityType());
                    curveDesc.postInfinityType = 
                        _ResolveInfinityType(curve.postInfinityType());
                    currentOp->curves.push_back(curveDesc);
                }
            }
        }
        if(sortedPath.GetParentPath().IsAbsoluteRootPath()) {
            primDesc.parent = NULL;
            rootPrims.push_back(primDesc);
            insertedPrimDatas.insert(std::make_pair(sortedPath,
                &rootPrims.back()));
        } else {
            UsdAnimXPrimDesc* parentDesc = 
                insertedPrimDatas[sortedPath.GetParentPath()];
            primDesc.parent = parentDesc;
            parentDesc->children.push_back(primDesc);
            insertedPrimDatas.insert(std::make_pair(sortedPath,
                &parentDesc->children.back()));
        }
    }
    return rootPrims;
}

bool
UsdAnimXData::_HasAnimatedProperty(
    const SdfPath &path) const
{
    // Check that it is a property id.
    if (!path.IsPropertyPath()) {
        return false;
    }
    auto data = TfMapLookupPtr(_animatedPrimDatas, path.GetPrimPath());
    if(data) {
        for(auto& op: data->ops) {
            if(op.target == path.GetNameToken()) {
                if(op.curves.size()) {
                    return true;
                }
                else return false;
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
    const UsdAnimXPrimData *data = 
        TfMapLookupPtr(_animatedPrimDatas, path.GetAbsoluteRootOrPrimPath());

    if (data) {
        if (value) {
            const UsdAnimXOpData *op = data->GetAnimatedOp(path.GetNameToken());
            if(op) {
                *value = op->defaultValue;
                return true;
            }
        }
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
    const UsdAnimXPrimData *data = 
        TfMapLookupPtr(_animatedPrimDatas, path.GetAbsoluteRootOrPrimPath());
    if (data) {
        if (value) {
            const UsdAnimXOpData *op = data->GetAnimatedOp(path.GetNameToken());
            if(op) {
                /*
                *value = 
                    VtValue(SdfSchema::GetInstance().FindType(
                        op->defaultValue).GetAsToken());
                */
                //VtArray<double> keyframe;
                UsdAnimXKeyframe keyframe;
                *value = VtValue(SdfSchema::GetInstance().FindType(
                    VtValue(keyframe)).GetAsToken());
                
                return true;
            }
        }
    }
    return false;
}


PXR_NAMESPACE_CLOSE_SCOPE
