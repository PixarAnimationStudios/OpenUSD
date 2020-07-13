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
#include <iostream>
#include <typeinfo>
#include "pxr/usd/usdAnimX/api.h"
#include "pxr/usd/usdAnimX/types.h"
#include "pxr/usd/usdAnimX/desc.h"

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
    bool Open(const std::string &filePath);
    ANIMX_API
    bool Write(const SdfAbstractDataConstPtr &data);
    ANIMX_API
    void Close();

private:
    void _WriteCookie(std::ostream &stream);
    void _WriteDepth(std::ostream &stream);
    void _OpenScope(std::ostream &stream);
    void _CloseScope(std::ostream &stream);
    void _WritePrim(std::ostream &stream, const UsdAnimXPrimDesc &desc);
    void _WriteOp(std::ostream &stream, const UsdAnimXOpDesc &desc);
    void _WriteCurve(std::ostream &stream, const UsdAnimXCurveDesc &desc);
    void _WriteSpec(std::ostream &stream, const TfToken &token, 
        const VtValue &value);
    inline void _WriteValue(std::ostream &stream, const VtValue &value);
    inline void _WriteKeyframes(std::ostream &stream, 
        const std::vector<UsdAnimXKeyframeDesc> &keyframes);

    size_t              _currentDepth;
    std::ofstream       _file;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PLUGIN_USD_ANIMX_WRITER_H
