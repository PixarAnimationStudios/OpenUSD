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
#include "pxr/usd/ar/asset.h"
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include "types.h"
#include "desc.h"
#include "tokens.h"


PXR_NAMESPACE_OPEN_SCOPE

enum UsdAnimXReaderState {
    ANIMX_READ_PRIM,
    ANIMX_READ_OP,
    ANIMX_READ_CURVE
};

class UsdAnimXData;

/// \class UsdAnimxReader
///
/// An animx reader suitable for an SdfAbstractData.
///
class UsdAnimXReader {
public:
    UsdAnimXReader();
    ~UsdAnimXReader();

    /// Open a file
    //bool Open(const std::string& filePath);
    bool Open(std::shared_ptr<ArAsset> asset);

    /// Close the file.
    void Close();

protected:
    inline bool _HasSpec(const std::string& s, const TfToken& token);
    inline bool _IsPrim(const std::string &s, UsdAnimXPrimDesc *desc);
    inline bool _IsOp(const std::string &s, UsdAnimXOpDesc *desc);
    inline bool _IsCurve(const std::string &s, UsdAnimXCurveDesc *desc);
    inline bool _IsKeyframe(const std::string& s, UsdAnimXKeyframeDesc *desc);
    inline bool _HasOpeningBrace(const std::string &s, size_t *pos=NULL);
    inline bool _HasClosingBrace(const std::string &s, size_t *pos=NULL);
    inline bool _HasOpeningBracket(const std::string &s, size_t *pos=NULL);
    inline bool _HasClosingBracket(const std::string &s, size_t *pos=NULL);
    inline bool _HasOpeningParenthese(const std::string &s, size_t *pos=NULL);
    inline bool _HasClosingParenthese(const std::string &s, size_t *pos=NULL);
    inline bool _HasOpeningQuote(const std::string &s, size_t *pos=NULL);
    inline bool _HasClosingQuote(const std::string &s, size_t start, size_t *pos=NULL);
    inline bool _HasChar(const std::string &s, const char c, size_t *pos=NULL);
    inline TfToken _GetNameToken(const std::string &s);
    inline VtValue _GetValue(const std::string& s);
    inline std::string _Trim(const std::string& s);

    void _ReadPrim(const std::string& s);
    void _ReadOp(const std::string& s);
    void _ReadCurve(const std::string& s);

private:
    std::vector<UsdAnimXPrimDesc> _rootPrims;
    size_t                        _readState;
    size_t                        _primDepth;
    UsdAnimXData*                 _datas;

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
UsdAnimXReader::_HasClosingQuote(const std::string &s, size_t start, size_t *pos)
{
    return _HasChar(std::string(s.begin() + start, s.end()), '"', pos);
}

TfToken
UsdAnimXReader::_GetNameToken(const std::string &s)
{
    size_t start, end;
    if(_HasOpeningQuote(s, &start)) {
        if(_HasClosingQuote(s, start + 1, &end)) {
            return TfToken(std::string(s.begin()+start+1, s.begin()+start+1+end));
        }
    }
    return TfToken();
}

VtValue
UsdAnimXReader::_GetValue(const std::string &s)
{
    return VtValue();
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
UsdAnimXReader::_IsKeyframe(const std::string& s, UsdAnimXKeyframeDesc* desc)
{   
    if(_readState != ANIMX_READ_CURVE)return false;
    if(_HasOpeningBrace(s, NULL) || _HasClosingBrace(s, NULL))return false;
    if(_HasOpeningParenthese(s) && _HasClosingParenthese(s)) {
        /*
        VtValue value = _GetValue(s);
        if(value.IsHolding<VtArray<double>>()) {
            VtArray<double>& datas = value.UncheckedGet<VtArray<double>>();
            desc->time = datas[0];
            size_t n = datas.size() - 1;
            desc->data.resize(n);
            memcpy(&desc->data[0], &datas[1], n * sizeof(double));
        }
        */
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

/*
std::vector<std::string> 
UsdAnimXReader::_GetTokens(const std::string& s)
{
  return std::vector<std::string>();
}
*/

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PLUGIN_USD_ANIMX_READER_H
