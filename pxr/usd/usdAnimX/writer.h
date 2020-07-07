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
#ifndef PXR_USD_PLUGIN_USD_ANIMX_WRITER_H
#define PXR_USD_PLUGIN_USD_ANIMX_WRITER_H

/// \file usdAnimX/writer.h

#include "pxr/pxr.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"
#include <set>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <typeinfo>
#include "api.h"
#include "types.h"

PXR_NAMESPACE_OPEN_SCOPE

struct UsdAnimXKeyframe;

TF_DECLARE_WEAK_AND_REF_PTRS(SdfAbstractData);


enum UsdAnimXWriterState {
    ANIMX_WRITE_PRIM,
    ANIMX_WRITE_OP,
    ANIMX_WRITE_CURVE
};

/// \class UsdAnimXWriter
///
/// An animx writer suitable for an SdfAbstractData.
///
class UsdAnimXWriter {
public:
    UsdAnimXWriter();
    ~UsdAnimXWriter();

    ANIMX_API
    bool Open(const std::string& filePath);
    ANIMX_API
    bool Write(const SdfAbstractDataConstPtr& data);
    ANIMX_API
    void Close();

private:
    inline void _WriteDepth(std::ostream& stream);
    inline void _OpenScope(std::ostream& stream);
    inline void _CloseScope(std::ostream& stream);
    inline void _WritePrim(std::ostream& stream, const TfToken& name);
    inline void _WriteOp(std::ostream& stream, const TfToken& name);
    inline void _WriteSpec(std::ostream& stream, const TfToken& token, 
        const VtValue& value);
    inline void _WriteValue(std::ostream& stream, const VtValue& value);
    inline void _WriteKeyframes(std::ostream& stream, 
        const std::vector<UsdAnimXKeyframe>& keyframes);

    size_t              _currentDepth;
    std::ofstream       _file;
    //std::ostream        _stream;
};

void 
UsdAnimXWriter::_WriteDepth(std::ostream& stream)
{
    for(size_t d=0;d<_currentDepth;++d)
        stream << "\t";
}

void 
UsdAnimXWriter::_OpenScope(std::ostream& stream)
{
    _WriteDepth(stream);
    stream << "{\n";
    _currentDepth++;
}

void 
UsdAnimXWriter::_CloseScope(std::ostream& stream)
{
    _currentDepth--;
    _WriteDepth(stream);
    stream << "}\n"; 
}

void 
UsdAnimXWriter::_WritePrim(std::ostream& stream, const TfToken& name)
{
    _WriteDepth(stream);
    stream << UsdAnimXTokens->prim.GetText() 
        << " \"" << name.GetText() << "\"\n";
    _OpenScope(stream);
}

void 
UsdAnimXWriter::_WriteOp(std::ostream& stream, const TfToken& name)
{
    _WriteDepth(stream);
    stream << UsdAnimXTokens->op.GetText()
        << " \"" << name.GetText() << "\"\n";
    _OpenScope(stream);
}

void 
UsdAnimXWriter::_WriteSpec(std::ostream& stream, const TfToken& token, 
    const VtValue& value)
{
    std::cout << "WRITE SPEC : " << token << std::endl;
    _WriteDepth(stream);
    if(token == UsdAnimXTokens->dataType) {
        stream << token << " "
            << AnimXGetSerializationTypeNameFromToken(value.Get<TfToken>()) 
            << "\n";
    } else if(token == UsdAnimXTokens->defaultValue) {
        /*
        if(value.IsArrayValued()) {
            const std::type_info &t = value.GetTypeid();
            
            if(t == typeid(VtArray<bool>) ) {
                _stream << value.Get<VtArray<bool>>();
            } else if(t == typeid(VtArray<unsigned char>)) {
                _stream << value.Get<VtArray<unsigned char>>();
            } else if(t == typeid(VtArray<int>)) {
                _stream << value.Get<VtArray<int>>();
            } else if(t == typeid(VtArray<float>)) {
                _stream << value.Get<VtArray<float>>();
            } else if(t == typeid(VtArray<double>)) {
                _stream << value.Get<VtArray<double>>();
            } else if(t == typeid(VtArray<GfHalf>)) {
                _stream << value.Get<VtArray<GfHalf>>();
            } else if(t == typeid(VtArray<GfVec2i>)) {
                _stream << value.Get<VtArray<GfVec2i>>();
            } else if(t == typeid(VtArray<GfVec2h>)) {
                _stream << value.Get<VtArray<GfVec2h>>();
            } else if(t == typeid(VtArray<GfVec2f>)) {
                _stream << value.Get<VtArray<GfVec2f>>();
            } else if(t == typeid(VtArray<GfVec2d>)) {
                _stream << value.Get<VtArray<GfVec2d>>();
            } else if(t == typeid(VtArray<GfVec3i>)) {
                _stream << value.Get<VtArray<GfVec3i>>();
            } else if(t == typeid(VtArray<GfVec3h>)) {
                _stream << value.Get<VtArray<GfVec3h>>();
            } else if(t == typeid(VtArray<GfVec3f>)) {
                _stream << value.Get<VtArray<GfVec3f>>();
            } else if(t == typeid(VtArray<GfVec3d>)) {
                _stream << value.Get<VtArray<GfVec3d>>();
            } else if(t == typeid(VtArray<GfVec4i>)) {
                _stream << value.Get<VtArray<GfVec4i>>();
            } else if(t == typeid(VtArray<GfVec4h>)) {
                _stream << value.Get<VtArray<GfVec4h>>();
            } else if(t == typeid(VtArray<GfVec4f>)) {
                _stream << value.Get<VtArray<GfVec4f>>();
            } else if(t == typeid(VtArray<GfVec4d>)) {
                _stream << value.Get<VtArray<GfVec4d>>();
            } else if(t == typeid(VtArray<GfQuath>)) {
                _stream << value.Get<VtArray<GfQuath>>();
            } else if(t == typeid(VtArray<GfQuatf>)) {
                _stream << value.Get<VtArray<GfQuatf>>();
            } else if(t == typeid(VtArray<GfQuatd>)) {
                _stream << value.Get<VtArray<GfQuatd>>();
            } else if(t == typeid(VtArray<GfMatrix2d>)) {
                _stream << value.Get<VtArray<GfMatrix2d>>();
            } else if(t == typeid(VtArray<GfMatrix3d>)) {
                _stream << value.Get<VtArray<GfMatrix3d>>();
            } else if(t == typeid(VtArray<GfMatrix4d>)) {
                _stream << value.Get<VtArray<GfMatrix4d>>();
            }
        }
        else {
            const std::type_info &t = value.GetTypeid();
            if(t == typeid(bool) ) {
                _stream << value.Get<bool>();
            } else if(t == typeid(unsigned char)) {
                _stream << value.Get<unsigned char>();
            } else if(t == typeid(int)) {
                _stream << value.Get<int>();
            } else if(t == typeid(float)) {
                _stream << value.Get<float>();
            } else if(t == typeid(double)) {
                _stream << value.Get<double>();
            } else if(t == typeid(GfHalf)) {
                _stream << value.Get<GfHalf>();
            } else if(t == typeid(GfVec2i)) {
                _stream << value.Get<GfVec2i>();
            } else if(t == typeid(GfVec2h)) {
                _stream << value.Get<GfVec2h>();
            } else if(t == typeid(GfVec2f)) {
                _stream << value.Get<GfVec2f>();
            } else if(t == typeid(GfVec2d)) {
                _stream << value.Get<GfVec2d>();
            } else if(t == typeid(GfVec3i)) {
                _stream << value.Get<GfVec3i>();
            } else if(t == typeid(GfVec3h)) {
                _stream << value.Get<GfVec3h>();
            } else if(t == typeid(GfVec3f)) {
                _stream << value.Get<GfVec3f>();
            } else if(t == typeid(GfVec3d)) {
                _stream << value.Get<GfVec3d>();
            } else if(t == typeid(GfVec4i)) {
                _stream << value.Get<GfVec4i>();
            } else if(t == typeid(GfVec4h)) {
                _stream << value.Get<GfVec4h>();
            } else if(t == typeid(GfVec4f)) {
                _stream << value.Get<GfVec4f>();
            } else if(t == typeid(GfVec4d)) {
                _stream << value.Get<GfVec4d>();
            } else if(t == typeid(GfQuath)) {
                _stream << value.Get<GfQuath>();
            } else if(t == typeid(GfQuatf)) {
                _stream << value.Get<GfQuatf>();
            } else if(t == typeid(GfQuatd)) {
                _stream << value.Get<GfQuatd>();
            } else if(t == typeid(GfMatrix2d)) {
                _stream << value.Get<GfMatrix2d>();
            } else if(t == typeid(GfMatrix3d)) {
                _stream << value.Get<GfMatrix3d>();
            } else if(t == typeid(GfMatrix4d)) {
                _stream << value.Get<GfMatrix4d>();
            }
        }*/
        stream << value << "\n";
    } else {
        stream << token << " \""<< value.Get<TfToken>() << "\"\n";
    }
}

void 
UsdAnimXWriter::_WriteKeyframes(std::ostream& stream, 
    const std::vector<UsdAnimXKeyframe>& keyframes)
{

}


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PLUGIN_USD_ANIMX_WRITER_H
