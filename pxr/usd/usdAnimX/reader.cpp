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
/// \file usdAnimX/reader.cpp

#include "pxr/pxr.h"
#include "pxr/usd/usdAnimX/animx.h"
#include "pxr/usd/usdAnimX/reader.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdAnimXReader::UsdAnimXReader()
  : _currentPrim(NULL)
  , _currentOp(NULL)
  , _currentCurve(NULL)
{
}

UsdAnimXReader::~UsdAnimXReader()
{
}

void UsdAnimXReader::_ReadPrim(const std::string& s)
{
    if(_IsPrim(s, &_primDesc)) {
        _primDepth++;
        if(_currentPrim) {
            _primDesc.parent = _currentPrim;
            _currentPrim->children.push_back(_primDesc);
            _currentPrim = &_currentPrim->children.back();
        } else {
            _primDesc.parent = NULL;
            _rootPrims.push_back(_primDesc);
            _currentPrim = &_rootPrims.back();
        }
        _readState = ANIMX_READ_PRIM;
    } else if(_IsOp(s, &_opDesc)) {
        _currentPrim->ops.push_back(_opDesc);
        _currentOp = &_currentPrim->ops.back();
        _readState = ANIMX_READ_OP;
    }
}

void UsdAnimXReader::_ReadOp(const std::string& s)
{ 
    if(_HasOpeningBrace(s))return;
    if(_IsCurve(s, &_curveDesc)) {
        _curveDesc.preInfinityType = UsdAnimXTokens->constant;
        _curveDesc.postInfinityType = UsdAnimXTokens->constant;
        _currentOp->curves.push_back(_curveDesc);
        _currentCurve = &_currentOp->curves.back();
        _readState = ANIMX_READ_CURVE;
    } else if(_HasSpec(s, UsdAnimXTokens->target)) {
        _currentOp->target = _GetNameToken(s);
    } else if(_HasSpec(s, UsdAnimXTokens->dataType)) {
        _currentOp->dataType = _GetNameToken(s);
    } else if(_HasSpec(s, UsdAnimXTokens->defaultValue)) {
        _currentOp->defaultValue = _GetValue(s, _currentOp->dataType);
    }
}

void UsdAnimXReader::_ReadCurve(const std::string& s)
{
    if(_HasSpec(s, UsdAnimXTokens->preInfinityType)) {
        _currentCurve->preInfinityType = _GetNameToken(s);
    } else if(_HasSpec(s, UsdAnimXTokens->postInfinityType)) {
        _currentCurve->postInfinityType = _GetNameToken(s);
    } else if(_IsKeyframes(s))
    {
        _ReadKeyframes(s);
    }
}

void UsdAnimXReader::_ReadKeyframes(const std::string& s)
{
    std::istringstream stream(s);
    size_t state = 0;
    bool readKey = false;
    char c;
    while(true) {
        if(readKey) {
            if(!state) {
                stream >> _keyframeDesc.time;
            } else {
                stream >> _keyframeDesc.data[state-1];
            }
            state++;
        }
        stream >> c;
        if(c == ')') {
            readKey = false;
            state = 0;
            _currentCurve->keyframes.push_back(_keyframeDesc);
        } else if(c == '(') {
            readKey = true;
        } else if(c == ']') break;
    }
}

static VtValue
_ExtractValueFromString(const std::string &s, const std::type_info &typeInfo, 
    size_t *pos)
{
    std::istringstream stream;
    stream.str(s);
    if(typeInfo == typeid(bool)) {
        return VtValue(
            _ExtractSingleValueFromString<bool>(stream, pos));
    } else if(typeInfo == typeid(unsigned char)) {
        return VtValue(
            _ExtractSingleValueFromString<unsigned char>(stream, pos));
    } else if(typeInfo == typeid(int)) {
        return VtValue(
            _ExtractSingleValueFromString<int>(stream, pos));
    } else if(typeInfo == typeid(float)) {
        return VtValue(
            _ExtractSingleValueFromString<float>(stream, pos));
    } else if(typeInfo == typeid(double)) {
        return VtValue(
            _ExtractSingleValueFromString<double>(stream, pos));
    } else if(typeInfo == typeid(TfToken)) {
        return VtValue(TfToken(s));
    } else if(typeInfo == typeid(GfVec2i)) {
        return VtValue(
            _ExtractTupleValueFromString<GfVec2i>(stream, 2, pos));
    } else if(typeInfo == typeid(GfVec2h)) {
        return VtValue(
            _ExtractTupleValueFromString<GfVec2h>(stream, 2, pos));
    } else if(typeInfo == typeid(GfVec2f)) {
        return VtValue(
            _ExtractTupleValueFromString<GfVec2f>(stream, 2, pos));
    } else if(typeInfo == typeid(GfVec2d)) {
       return VtValue(
          _ExtractTupleValueFromString<GfVec2d>(stream, 2, pos));
    } else if(typeInfo == typeid(GfVec3i)) {
        return VtValue(
            _ExtractTupleValueFromString<GfVec3i>(stream, 3, pos));
    } else if(typeInfo == typeid(GfVec3h)) {
       return VtValue(
          _ExtractTupleValueFromString<GfVec3h>(stream, 3, pos));
    } else if(typeInfo == typeid(GfVec3f)) {
        return VtValue(
            _ExtractTupleValueFromString<GfVec3f>(stream, 3, pos));
    } else if(typeInfo == typeid(GfVec3d)) {
        return VtValue(
            _ExtractTupleValueFromString<GfVec3d>(stream, 3, pos));
    } else if(typeInfo == typeid(GfVec4i)) {
        return VtValue(
            _ExtractTupleValueFromString<GfVec4i>(stream, 4, pos));
    } else if(typeInfo == typeid(GfVec4h)) {
        return VtValue(
            _ExtractTupleValueFromString<GfVec4h>(stream, 4, pos));
    } else if(typeInfo == typeid(GfVec4f)) {
        return VtValue(
            _ExtractTupleValueFromString<GfVec4f>(stream, 4, pos));
    } else if(typeInfo == typeid(GfVec4d)) {
        return VtValue(
            _ExtractTupleValueFromString<GfVec4d>(stream, 4, pos));
    } else if(typeInfo == typeid(GfMatrix4d)) {
        return VtValue(
            _ExtractArrayTupleValueFromString<GfMatrix4d>(stream, 4, 4, pos));
    } /*else if(typeInfo == typeid(GfQuath)) {
        return VtValue(_ExtractTupleValueFromString<GfQuath>(stream, 4, pos));
    } else if(typeInfo == typeid(GfQuatf)) {
        return VtValue(_ExtractTupleValueFromString<GfQuatf>(stream, 4, pos));
    } else if(typeInfo == typeid(GfQuatd)) {
        return VtValue(_ExtractTupleValueFromString<GfQuatd>(stream, 4, pos));
    } */else {
        return VtValue();
    }
}

static VtValue
_ExtractArrayValueFromString(const std::string &s, 
    const std::type_info &typeInfo, size_t *pos)
{
    std::istringstream stream(s);
    if(typeInfo == typeid(bool)) {
        return VtValue(
            _ExtractSingleValueArrayFromString<bool>(stream, pos));
    } else if(typeInfo == typeid(unsigned char)) {
        return VtValue(
            _ExtractSingleValueArrayFromString<unsigned char>(stream, pos));
    } else if(typeInfo == typeid(int)) {
        return VtValue(
            _ExtractSingleValueArrayFromString<int>(stream, pos));
    } else if(typeInfo == typeid(float)) {
        return VtValue(
            _ExtractSingleValueArrayFromString<float>(stream, pos));
    } else if(typeInfo == typeid(double)) {
        return VtValue(
            _ExtractSingleValueArrayFromString<double>(stream, pos));
    } else if(typeInfo == typeid(TfToken)) {
        return VtValue(
            _ExtractSingleValueArrayFromString<TfToken>(stream, pos));
    } else if(typeInfo == typeid(GfVec2i)) {
        return VtValue(
            _ExtractTupleValueArrayFromString<GfVec2i>(stream, 2, pos));
    } else if(typeInfo == typeid(GfVec2h)) {
        return VtValue(
            _ExtractTupleValueArrayFromString<GfVec2h>(stream, 2, pos));
    } else if(typeInfo == typeid(GfVec2f)) {
        return VtValue(
            _ExtractTupleValueArrayFromString<GfVec2f>(stream, 2, pos));
    } else if(typeInfo == typeid(GfVec2d)) {
       return VtValue(
          _ExtractTupleValueArrayFromString<GfVec2d>(stream, 2, pos));
    } else if(typeInfo == typeid(GfVec3i)) {
        return VtValue(
            _ExtractTupleValueArrayFromString<GfVec3i>(stream, 3, pos));
    } else if(typeInfo == typeid(GfVec3h)) {
       return VtValue(
          _ExtractTupleValueArrayFromString<GfVec3h>(stream, 3, pos));
    } else if(typeInfo == typeid(GfVec3f)) {
        return VtValue(
            _ExtractTupleValueArrayFromString<GfVec3f>(stream, 3, pos));
    } else if(typeInfo == typeid(GfVec3d)) {
        return VtValue(
            _ExtractTupleValueArrayFromString<GfVec3d>(stream, 3, pos));
    } else if(typeInfo == typeid(GfVec4i)) {
        return VtValue(
            _ExtractTupleValueArrayFromString<GfVec4i>(stream, 4, pos));
    } else if(typeInfo == typeid(GfVec4h)) {
        return VtValue(
            _ExtractTupleValueArrayFromString<GfVec4h>(stream, 4, pos));
    } else if(typeInfo == typeid(GfVec4f)) {
        return VtValue(
            _ExtractTupleValueArrayFromString<GfVec4f>(stream, 4, pos));
    } else if(typeInfo == typeid(GfVec4d)) {
        return VtValue(
            _ExtractTupleValueArrayFromString<GfVec4d>(stream, 4, pos));
    } /*else if(typeInfo == typeid(GfMatrix4d)) {
        return VtValue(_ExtractArrayValuesFromString<GfMatrix4d>(stream, pos));
    } else if(typeInfo == typeid(GfQuath)) {
        return VtValue(_ExtractArrayValuesFromString<GfQuath>(stream, 4, pos));
    } else if(typeInfo == typeid(GfQuatf)) {
        return VtValue(_ExtractArrayValuesFromString<GfQuatf>(stream, 4, pos));
    } else if(typeInfo == typeid(GfQuatd)) {
        return VtValue(_ExtractArrayValuesFromString<GfQuatd>(stream, 4, pos));
    } */else {
        return VtValue();
    }
    /*
    if(typeInfo == typeid(TfToken)) {
        return VtValue(_ExtractArrayValuesFromString<TfToken>(stream, &c));
    }
    return VtValue();
    */
}

VtValue
UsdAnimXReader::_GetValue(const std::string &s, const TfToken& type)
{
    const SdfValueTypeName& typeName = 
        AnimXGetSdfValueTypeNameFromToken(type);
    VtValue value = typeName.GetDefaultValue();
    
    size_t b, e, c;
    if(value.IsArrayValued()) {
        if(_HasChar(s, ' ', &b)) {
            std::string datas = s.substr(b, s.length()-b+1);
                return _ExtractArrayValueFromString(datas, 
                    typeName.GetScalarType().GetType().GetTypeid(), &c);
        }
    } else {
        if(_HasChar(s, ' ', &b)) {
            std::string datas = s.substr(b+1, s.length()-b);
                return _ExtractValueFromString(datas, 
                        typeName.GetType().GetTypeid(), &c);
        }
    }
    return value;
}

/// Open a file
bool  UsdAnimXReader::Read(const std::string& resolvedPath)
{
    std::ifstream stream(resolvedPath.c_str());
    std::string line;
    _readState = ANIMX_READ_PRIM;
    _primDepth = 0;
    size_t pos;
    while (std::getline(stream, line)) {
        if(line.empty())continue;
        line = _Trim(line);
        if(_HasClosingBrace(line, &pos)) {
            switch(_readState) {
                case ANIMX_READ_CURVE:
                    _readState = ANIMX_READ_OP;
                    break;
                case ANIMX_READ_OP:
                    _readState = ANIMX_READ_PRIM;
                    break;
                case ANIMX_READ_PRIM:
                    if(_currentPrim) {
                        _currentPrim = _currentPrim->parent;
                    }
                    break;
            }
        } else {
            switch(_readState) {
                case ANIMX_READ_CURVE:
                    _ReadCurve(line);
                    break;
                case ANIMX_READ_OP:
                    _ReadOp(line);
                    break;
                case ANIMX_READ_PRIM:
                    _ReadPrim(line);
                    break;
            }
        }
    };
    return true;
}

namespace { // anonymous namespace

void _PopulatePrim(UsdAnimXDataRefPtr& datas, const UsdAnimXPrimDesc& prim, 
    const SdfPath& parentPath)
{
    SdfPath primPath = parentPath.AppendChild(prim.name);
    datas->AddPrim(primPath);
    if(prim.ops.size()) {
        for(const auto& op: prim.ops) {
            datas->AddOp(primPath, op);
            
            if(op.curves.size()) {
                for(const auto& curve: op.curves) {
                    datas->AddFCurve(primPath, op.target, curve);
                }
            }
        }
    }
    
    for(const auto& child: prim.children) {
        _PopulatePrim(datas, child, primPath);
    }
}

} // end anonymous namespace

/// Populate datas
void UsdAnimXReader::PopulateDatas(UsdAnimXDataRefPtr& datas)
{
    SdfPath rootPath("/");
    SdfPathVector rootPaths;
    for(const auto& prim: _rootPrims) {
        rootPaths.push_back(rootPath.AppendChild(prim.name));
        _PopulatePrim(datas, prim, rootPath);
    }
    datas->SetRootPrimPaths(rootPaths);
}

PXR_NAMESPACE_CLOSE_SCOPE

