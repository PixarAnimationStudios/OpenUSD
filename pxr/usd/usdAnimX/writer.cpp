//
// Copyright 2020 benmalarre
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
/// \file usdAnimX/writer.cpp

#include "pxr/usd/sdf/data.h"
#include "tokens.h"
#include "writer.h"
#include "curve.h"
#include "keyframe.h"
#include "data.h"
#include "desc.h"

PXR_NAMESPACE_OPEN_SCOPE

//
// UsdAnimXWriter
//
UsdAnimXWriter::UsdAnimXWriter()
    : _currentDepth(0)
{
    // Do nothing
}

UsdAnimXWriter::~UsdAnimXWriter()
{
    Close();
}

bool
UsdAnimXWriter::Open(const std::string& filePath)
{
    _file.open(filePath);
    if(_file.is_open())return true;
    return false;
}

bool
UsdAnimXWriter::Write(const SdfAbstractDataConstPtr& data)
{
    UsdAnimXDataConstPtr animXData = 
        TfStatic_cast<const UsdAnimXDataConstPtr>(data);
    //for(const auto& prim: animXData)
    std::cout << "WRITE TO FUCKIN FILE !!!" << std::endl;
    if(_file.is_open()) {
        std::vector<UsdAnimXPrimDesc> rootPrims = animXData->BuildDescription();
        
        for(auto& rootPrim: rootPrims) {
            _WritePrim(_file, rootPrim.name);
        }
        //data->GetDesc
        //data->WriteToStream(_file);
    }
    
    return true;
}

void
UsdAnimXWriter::Close()
{
    if(_file.is_open())
        _file.close();
}

PXR_NAMESPACE_CLOSE_SCOPE

