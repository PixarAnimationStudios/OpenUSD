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
#ifndef PXR_USD_PLUGIN_USD_ANIMX_READER_H
#define PXR_USD_PLUGIN_USD_ANIMX_READER_H

/// \file usdAnimX/rader.h

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/sdf/valueTypeRegistry.h"
#include "pxr/usd/sdf/abstractData.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/timeCode.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/ar/asset.h"
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <sstream>
#include "types.h"
#include "desc.h"
#include "tokens.h"
#include "data.h"

PXR_NAMESPACE_OPEN_SCOPE

enum UsdAnimXReaderState {
    ANIMX_READ_PRIM,
    ANIMX_READ_OP,
    ANIMX_READ_CURVE
};


/// \class UsdAnimxReader
///
/// An animx reader suitable for an SdfAbstractData.
///
class UsdAnimXReader {
public:
    UsdAnimXReader();
    ~UsdAnimXReader();

    /// Read a file
    bool Read(const std::string& filePath);
    void PopulateDatas(UsdAnimXDataRefPtr& datas);

protected:
    inline bool _HasSpec(const std::string& s, const TfToken& token);
    inline bool _IsPrim(const std::string &s, UsdAnimXPrimDesc *desc);
    inline bool _IsOp(const std::string &s, UsdAnimXOpDesc *desc);
    inline bool _IsCurve(const std::string &s, UsdAnimXCurveDesc *desc);
    inline bool _IsKeyframes(const std::string& s);
    inline bool _HasOpeningBrace(const std::string &s, size_t *pos=NULL);
    inline bool _HasClosingBrace(const std::string &s, size_t *pos=NULL);
    inline bool _HasOpeningBracket(const std::string &s, size_t *pos=NULL);
    inline bool _HasClosingBracket(const std::string &s, size_t *pos=NULL);
    inline bool _HasOpeningParenthese(const std::string &s, size_t *pos=NULL);
    inline bool _HasClosingParenthese(const std::string &s, size_t *pos=NULL);
    inline bool _HasOpeningQuote(const std::string &s, size_t *pos=NULL);
    inline bool _HasClosingQuote(const std::string &s, size_t start, 
        size_t *pos=NULL);
    inline bool _HasChar(const std::string &s, const char c, size_t *pos=NULL);
    inline bool _ReverseHasChar(const std::string &s, const char c, 
        size_t *pos=NULL);
    inline TfToken _GetNameToken(const std::string &s);
    inline std::string _Trim(const std::string& s);
    VtValue _GetValue(const std::string& s, const TfToken& type);

    void _ReadPrim(const std::string& s);
    void _ReadOp(const std::string& s);
    void _ReadCurve(const std::string& s);
    void _ReadKeyframes(const std::string& s);

private:
    std::vector<UsdAnimXPrimDesc> _rootPrims;
    size_t                        _readState;
    size_t                        _primDepth;

    UsdAnimXPrimDesc              _primDesc;
    UsdAnimXOpDesc                _opDesc;
    UsdAnimXCurveDesc             _curveDesc;
    UsdAnimXKeyframeDesc          _keyframeDesc;

    UsdAnimXPrimDesc*             _currentPrim;
    UsdAnimXOpDesc*               _currentOp;
    UsdAnimXCurveDesc*            _currentCurve;
};

bool 
UsdAnimXReader::_HasChar(const std::string &s, const char c, size_t *pos)
{
  size_t p = s.find(c);
    if(p != std::string::npos) {
        if(pos)*pos = p;
        return true;
    }
    return false;
}

bool 
UsdAnimXReader::_ReverseHasChar(const std::string &s, const char c, size_t *pos)
{
    size_t p = s.rfind(c);
    if(p != std::string::npos) {
        if(pos)*pos = p;
        return true;
    }
    return false;
}

bool
UsdAnimXReader::_HasOpeningBrace(const std::string &s, size_t *pos)
{
    return _HasChar(s, '{', pos);
}

bool
UsdAnimXReader::_HasClosingBrace(const std::string &s, size_t *pos)
{
    return _HasChar(s, '}', pos);
}

bool
UsdAnimXReader::_HasOpeningBracket(const std::string &s, size_t *pos)
{
    return _HasChar(s, '[', pos);
}

bool
UsdAnimXReader::_HasClosingBracket(const std::string &s, size_t *pos)
{
    return _HasChar(s, ']', pos);
}

bool
UsdAnimXReader::_HasOpeningParenthese(const std::string &s, size_t *pos)
{
    return _HasChar(s, '(', pos);
}

bool
UsdAnimXReader::_HasClosingParenthese(const std::string &s, size_t *pos)
{
    return _HasChar(s, ')', pos);
}

bool
UsdAnimXReader::_HasOpeningQuote(const std::string &s, size_t *pos)
{
    return _HasChar(s, '"', pos);
}

bool
UsdAnimXReader::_HasClosingQuote(const std::string &s, 
    size_t start, size_t *pos)
{
    return _HasChar(std::string(s.begin() + start, 
        s.end()), '"', pos);
}

TfToken
UsdAnimXReader::_GetNameToken(const std::string &s)
{
    size_t start, end;
    if(_HasOpeningQuote(s, &start)) {
        if(_HasClosingQuote(s, start + 1, &end)) {
            return TfToken(std::string(s.begin()+start+1, 
                s.begin()+start+1+end));
        }
    }
    return TfToken();
}

bool 
UsdAnimXReader::_HasSpec(const std::string& s, const TfToken& token)
{
    const char* spec = token.GetText();
    if(s.find(spec, 0, 
        token.size()) != std::string::npos) {
            return true;
        }
    return false;
}

bool 
UsdAnimXReader::_IsPrim(const std::string& s, UsdAnimXPrimDesc* desc)
{   
    if(_HasSpec(s, UsdAnimXTokens->prim)) {
        desc->name = _GetNameToken(s);
        desc->children.clear();
        desc->ops.clear();
        return true;
    } 
    return false;
}

bool 
UsdAnimXReader::_IsOp(const std::string& s, UsdAnimXOpDesc* desc)
{   
    if(_HasSpec(s, UsdAnimXTokens->op)) {
        desc->name = _GetNameToken(s);
        return true;
    }    
    return false;
}

bool 
UsdAnimXReader::_IsCurve(const std::string& s, UsdAnimXCurveDesc* desc)
{   
    if(_HasSpec(s, UsdAnimXTokens->curve)) {
        desc->name = _GetNameToken(s);
        return true;
    }
    return false;
}

bool 
UsdAnimXReader::_IsKeyframes(const std::string& s)
{   
    if(_HasSpec(s, UsdAnimXTokens->keyframes)) {
        return true;
    }
    return false;
}

std::string 
UsdAnimXReader::_Trim(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && isspace(*it))it++;

    std::string::const_reverse_iterator rit = s.rbegin();
    while (rit.base() != it && isspace(*rit))rit++;

    return std::string(it, rit.base());
}


/// Extract data 
///
namespace { // anonymous namespace
template<typename T>
static
T _ExtractSingleValueFromString(std::istringstream &stream, size_t *pos)
{
    T value;
    stream >> value;
    *pos = (size_t)stream.tellg();
    return value;
};

template<>
static
TfToken _ExtractSingleValueFromString<TfToken>(std::istringstream &stream, 
    size_t *pos)
{   
    char c;
    std::string value;
    stream >> c >> value >> c;  
    *pos = (size_t)stream.tellg();
    return TfToken(value);
}

template<typename T>
static
VtArray<T> _ExtractSingleValueArrayFromString(std::istringstream &stream, 
    size_t *pos)
{
    char c;
    VtArray<T> array;
    T value;
    stream >> c;
    while(true) {
        stream >> value;
        array.push_back(value);
        stream >> c;
        if(c == ']' || (size_t)stream.tellg() == std::string::npos)
            break;
    }
    *pos = (size_t)stream.tellg();
    return array;
};

template<>
static
VtArray<TfToken> _ExtractSingleValueArrayFromString<TfToken>(
    std::istringstream &stream, size_t *pos)
{
    VtArray<TfToken> array;
    std::string value;
    char c;
    bool reading = false;
    while(true) {
        stream >> c;
        if(c == '"') {
            reading = 1 - reading;
            if(!reading) {
                if(value.length()) {
                    array.push_back(TfToken(value));
                    value.clear();
                }
            }
        } else if( c == ']' || (size_t)stream.tellg() == std::string::npos) {
            break;
        } else if(reading) {
            value += c;
        }
    }
    *pos = (size_t)stream.tellg();
    return array;
};

template<typename T>
static
T _ExtractTupleValueFromString(std::istringstream &stream, size_t d, 
    size_t *pos)
{
    char c;
    T value;
    for(size_t i = 0; i < d; ++i) {
        stream >> c;
        stream >> value[i];
    }
    *pos = (size_t)stream.tellg() + 1;
    return value;
};

template<typename T>
static
VtArray<T> _ExtractTupleValueArrayFromString(std::istringstream &stream, 
    size_t d, size_t *pos)
{
    char c;
    VtArray<T> array;
    T value;
    stream >> c;
    while(true) {
        for(size_t i = 0; i < d; ++i) {
            stream >> c;
            stream >> value[i];
        }
        array.push_back(value);
        stream >> c >> c;
        if(c == ']' || (size_t)stream.tellg() == std::string::npos)
            break;
    }
    *pos = (size_t)stream.tellg() + 1;
    return array;
};

template<typename T>
static
T _ExtractArrayTupleValueFromString(std::istringstream &stream, 
    size_t d1, size_t d2, size_t *pos)
{
    char c;
    T value;
    for(size_t i = 0; i<d1; ++i) {
        for(size_t j = 0; j<d2; ++j) {
            stream >> c >> value[i][j];
        }
        stream >> c;
    }
    *pos = (size_t)stream.tellg() + 1;
    return value;
};
} // end anonymous namespace

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PLUGIN_USD_ANIMX_READER_H
