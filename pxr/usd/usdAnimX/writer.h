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
#include <set>
#include <string>
#include "api.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(SdfAbstractData);

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
    bool Close();
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PLUGIN_USD_ANIMX_WRITER_H
