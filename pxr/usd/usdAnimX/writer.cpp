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

#include <pxr/usd/sdf/data.h>
#include "tokens.h"
#include "writer.h"
#include "curve.h"
#include "keyframe.h"
#include "data.h"
#include "desc.h"
#include "fileFormat.h"

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
    if(_file.is_open()) {
        _WriteCookie(_file);
        std::vector<UsdAnimXPrimDesc> rootPrims = animXData->BuildDescription();
        for(auto& rootPrim: rootPrims) {
            _WritePrim(_file, rootPrim);
        }
        return true;
    }
    return false;
}

void
UsdAnimXWriter::Close()
{
    if(_file.is_open())
        _file.close();
}

void
UsdAnimXWriter::_WriteCookie(std::ostream &stream)
{
    stream  << "#" << UsdAnimXFileFormatTokens->Id
            << " " << UsdAnimXFileFormatTokens->Version 
            << "\n\n";
}

void 
UsdAnimXWriter::_WriteDepth(std::ostream &stream)
{
    for(size_t d=0;d<_currentDepth;++d)
        stream << "\t";
}

void 
UsdAnimXWriter::_OpenScope(std::ostream &stream)
{
    _WriteDepth(stream);
    stream << "{\n";
    _currentDepth++;
}

void 
UsdAnimXWriter::_CloseScope(std::ostream &stream)
{
    _currentDepth--;
    _WriteDepth(stream);
    stream << "}\n"; 
}

void 
UsdAnimXWriter::_WritePrim(std::ostream &stream, const UsdAnimXPrimDesc &desc)
{
    _WriteDepth(stream);
    stream << UsdAnimXTokens->prim.GetText() 
        << " \"" << desc.name.GetText() << "\"\n";
    _OpenScope(stream);
    if(desc.ops.size()) {
        for(const auto& op: desc.ops) {
            _WriteOp(stream, op);
        }
    }
    if(desc.children.size()) {
        for(const auto& child: desc.children) {
            _WritePrim(stream, child);
        }
    }
    _CloseScope(stream);
}

void 
UsdAnimXWriter::_WriteOp(std::ostream &stream, const UsdAnimXOpDesc &desc)
{
    _WriteDepth(stream);
    stream << UsdAnimXTokens->op.GetText()
        << " \"" << desc.name.GetText() << "\"\n";
    _OpenScope(stream);

    _WriteSpec(stream, UsdAnimXTokens->target, VtValue(desc.target));
    _WriteSpec(stream, UsdAnimXTokens->dataType, VtValue(desc.dataType));
    _WriteSpec(stream, UsdAnimXTokens->defaultValue, desc.defaultValue);

    for(const auto& curve: desc.curves) {
        _WriteCurve(stream, curve);
    }
    _CloseScope(stream);
}

void 
UsdAnimXWriter::_WriteCurve(std::ostream &stream, const UsdAnimXCurveDesc &desc)
{
    _WriteDepth(stream);
    stream << UsdAnimXTokens->curve.GetText()
        << " \"" << desc.name.GetText() << "\"\n";
    _OpenScope(stream);
    _WriteSpec(stream, UsdAnimXTokens->preInfinityType, 
        VtValue(desc.preInfinityType));
    _WriteSpec(stream, UsdAnimXTokens->postInfinityType, 
        VtValue(desc.postInfinityType));
    _WriteKeyframes(stream, desc.keyframes);
    _CloseScope(stream);
}

void 
UsdAnimXWriter::_WriteSpec(std::ostream &stream, const TfToken &token, 
    const VtValue &value)
{
    _WriteDepth(stream);
    if(token == UsdAnimXTokens->dataType) {
        stream << token << "  \"" << value.Get<TfToken>() << "\"\n";
    } else if(token == UsdAnimXTokens->defaultValue) {
        stream << token << " " << value << "\n";
    } else {
        stream << token << " \""<< value.Get<TfToken>() << "\"\n";
    }
}

void 
UsdAnimXWriter::_WriteKeyframes(std::ostream &stream, 
    const std::vector<UsdAnimXKeyframeDesc> &keyframes)
{
    _WriteDepth(stream);
    stream << UsdAnimXTokens->keyframes << " [";
    for(const auto& keyframe: keyframes) {
        stream << keyframe << ",";
    }
    stream << "]\n";
    
}

PXR_NAMESPACE_CLOSE_SCOPE

