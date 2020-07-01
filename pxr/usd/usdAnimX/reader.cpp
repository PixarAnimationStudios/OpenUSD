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
        case ANIMX_READ_NONE:
            std::cout << "READ STATE : NONE" << std::endl;
            break;
        case ANIMX_READ_PRIM_ENTER:
            std::cout << "READ STATE : PRIM_ENTER" << std::endl;
            break;
        case ANIMX_READ_PRIM:
            std::cout << "READ STATE : PRIM" << std::endl;
            break;
        case ANIMX_READ_OP_ENTER:
            std::cout << "READ STATE : OP_ENTER" << std::endl;
            break;
        case ANIMX_READ_OP:
            std::cout << "READ STATE : OP" << std::endl;
            break;
        case ANIMX_READ_CURVE_ENTER:
            std::cout << "READ STATE : CURVE_ENTER" << std::endl;
            break;
        case ANIMX_READ_CURVE:
            std::cout << "READ STATE : CURVE" << std::endl;
            break;
        default:
            std::cout << "READ STATE : OUT OF BOUND!!!" << std::endl;
    }
}

UsdAnimXReader::UsdAnimXReader()
{
}

UsdAnimXReader::~UsdAnimXReader()
{
}

/// Open a file
bool  UsdAnimXReader::Open(std::shared_ptr<ArAsset> asset)
{
    size_t bufferSize = asset->GetSize();
    std::shared_ptr<const char> content = asset->GetBuffer();
    UsdAnimXReadBuffer buffer((char*)content.get(), (char*)content.get() + bufferSize);

    std::istream stream(&buffer);
    std::string line;

    UsdAnimXPrimDesc primDesc;
    UsdAnimXOpDesc opDesc;
    UsdAnimXCurveDesc curveDesc;
    UsdAnimXKeyframeDesc keyframeDesc;

    UsdAnimXPrimDesc* currentPrim = NULL;
    UsdAnimXOpDesc* currentOp = NULL;
    UsdAnimXCurveDesc* currentCurve = NULL;

    _readState = ANIMX_READ_NONE;
    _primDepth = 0;
    size_t start_p, end_p;
    while (std::getline(stream, line)) {
        if(line.empty())continue;
        PrintReadState(_readState);
        line = _Trim(line);
        if(_IsPrim(line, &primDesc))
        {
            _primDepth++;
            if(currentPrim){
                primDesc.parent = currentPrim;
                currentPrim->children.push_back(primDesc);
                currentPrim = &currentPrim->children.back();
            }
            else {
                primDesc.parent = NULL;
                _rootPrims.push_back(primDesc);
                currentPrim = &_rootPrims.back();
            }
            _readState = ANIMX_READ_PRIM_ENTER;
        }
        else if(_IsOp(line, &opDesc))
        {
          std::cout << "GET OP : " << opDesc.name << std::endl;
          _readState = ANIMX_READ_OP_ENTER;
        }
        else if(_IsCurve(line, &curveDesc))
        {
          std::cout << "GET CURVE : " << curveDesc.name << std::endl;
          _readState = ANIMX_READ_CURVE_ENTER;
        }
        else if(_IsKeyframe(line, &keyframeDesc))
        {
            std::cout << "GET KEYFRAME : " << line << std::endl;
        }

        if(_HasOpeningBrace(line, &start_p)) {
            _readState++;
        }
        else if(_HasClosingBrace(line, &end_p)) {
            switch(_readState) {
                case ANIMX_READ_CURVE:
                    _readState = ANIMX_READ_OP;
                    break;
                case ANIMX_READ_OP:
                    _readState = ANIMX_READ_PRIM;
                    break;
                case ANIMX_READ_PRIM:
                    _primDepth--;
                    if(_primDepth == 0)_readState = ANIMX_READ_NONE;
                    break;
            }
        }
    }
    return true;
}

/// Close the file.
void  UsdAnimXReader::Close()
{

}

PXR_NAMESPACE_CLOSE_SCOPE

