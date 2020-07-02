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
#include "animx.h"
#include "reader.h"
#include "data.h"
#include <memory>
#include <iostream>
#include <istream>
#include <streambuf>

PXR_NAMESPACE_OPEN_SCOPE

struct UsdAnimXReadBuffer : std::streambuf
{
  UsdAnimXReadBuffer(char* begin, char* end) {
      setg(begin, begin, end);
  }
};

static void PrintReadState(size_t state){
    switch(state) {
        case ANIMX_READ_PRIM:
            std::cout << "READ STATE : PRIM" << std::endl;
            break;
        case ANIMX_READ_OP:
            std::cout << "READ STATE : OP" << std::endl;
            break;
        case ANIMX_READ_CURVE:
            std::cout << "READ STATE : CURVE" << std::endl;
            break;
        default:
            std::cout << "READ STATE : OUT OF BOUND!!!" << std::endl;
    }
}

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
    if(_IsPrim(s, &_primDesc))
    {
        _primDepth++;
        if(_currentPrim){
            _primDesc.parent = _currentPrim;
            _currentPrim->children.push_back(_primDesc);
            _currentPrim = &_currentPrim->children.back();
        }
        else {
            _primDesc.parent = NULL;
            _rootPrims.push_back(_primDesc);
            _currentPrim = &_rootPrims.back();
        }
        _readState = ANIMX_READ_PRIM;
    }
    else if(_IsOp(s, &_opDesc))
    {
        std::cout << "GET OP : " << _opDesc.name << std::endl;
        _currentPrim->ops.push_back(_opDesc);
        _currentOp = &_currentPrim->ops.back();
        _readState = ANIMX_READ_OP;
    }
}

void UsdAnimXReader::_ReadOp(const std::string& s)
{ 
    if(_HasOpeningBrace(s))return;
    if(_IsCurve(s, &_curveDesc))
    {
        std::cout << "GET CURVE : " << _curveDesc.name << std::endl;
        _currentOp->curves.push_back(_curveDesc);
        _currentCurve = &_currentOp->curves.back();
        _readState = ANIMX_READ_CURVE;
    }
    else if(_HasSpec(s, UsdAnimXTokens->target)) {
        std::cout << "TARGET : " << _GetNameToken(s) << std::endl;
        _currentOp->target = _GetNameToken(s);
    }
    else if(_HasSpec(s, UsdAnimXTokens->dataType)) {
        std::cout << "DATA TYPE : " << _GetNameToken(s) << std::endl;
        _currentOp->dataType = _GetNameToken(s);
        const SdfValueTypeName& typeName = 
            AnimXGetSdfValueTypeNameFromToken(_currentOp->dataType);

        //Sdf_ValueTypeRegistry::FindType(_currentOp->dataType.GetString());
        //SdfValueTypeName typeName = SdfValueTypeName::
        TfType type = typeName.GetType();
        std::cout << "TF TYPE IS KNOWN : " << type.IsUnknown() << std::endl;
        std::cout << "TF TYPE : " << type.GetTypeid().name() << std::endl;
    }
    else if(_HasSpec(s, UsdAnimXTokens->defaultValue)) {
        std::cout << "DEFAULT VALUE: " << _GetValue(s) << std::endl;
        _currentOp->defaultValue = _GetValue(s);
    }
    //else if(_HasSpec(s, UsdAnimXTokens->use))
}

void UsdAnimXReader::_ReadCurve(const std::string& s)
{
    if(_HasSpec(s, UsdAnimXTokens->preInfinityType)) {
        std::cout << "HAS PRE INFINITY SPEC!!!" << std::endl;
        _currentCurve->preInfinityType = _GetNameToken(s);
        std::cout << "PRE-INFINITY : " << _currentCurve->preInfinityType << std::endl;
    } else if(_HasSpec(s, UsdAnimXTokens->postInfinityType)) {
        std::cout << "HAS POST INFINITY SPEC!!!" << std::endl;
        _currentCurve->postInfinityType = _GetNameToken(s);
        std::cout << "POST-INFINITY : " << _currentCurve->postInfinityType << std::endl;
    }
    else if(_IsKeyframe(s, &_keyframeDesc))
    {
        std::cout << "GET KEYFRAME : " << s << std::endl;
    }
}

/// Open a file
bool  UsdAnimXReader::Open(std::shared_ptr<ArAsset> asset)
{
    size_t bufferSize = asset->GetSize();
    std::shared_ptr<const char> content = asset->GetBuffer();
    UsdAnimXReadBuffer buffer((char*)content.get(), (char*)content.get() + bufferSize);

    std::istream stream(&buffer);
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

/// Close the file.
void  UsdAnimXReader::Close()
{

}

PXR_NAMESPACE_CLOSE_SCOPE

