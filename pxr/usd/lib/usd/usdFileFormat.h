//
// Copyright 2016 Pixar
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
#ifndef USD_USD_FILE_FORMAT_H
#define USD_USD_FILE_FORMAT_H

#include "pxr/usd/usd/api.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/base/tf/staticTokens.h"

#include <string>

TF_DECLARE_WEAK_AND_REF_PTRS(UsdUsdFileFormat);
TF_DECLARE_WEAK_PTRS(SdfLayerBase);

#define USD_USD_FILE_FORMAT_TOKENS  \
    ((Id,           "usd"))         \
    ((Version,      "1.0"))         \
    ((Target,       "usd"))         \
    ((FormatArg,    "format"))

TF_DECLARE_PUBLIC_TOKENS(UsdUsdFileFormatTokens, USD_API, USD_USD_FILE_FORMAT_TOKENS);

/// \class UsdUsdFileFormat
///
/// File format for USD files.
///
/// When creating a file through the SdfLayer::CreateNew() interface, the
/// meaningful SdfFileFormat::FileFormatArguments are as follows:
/// \li UsdUsdFileFormatTokens->FormatArg , which must be a supported format's
///     'Id'.  The possible values are UsdUsdaFileFormatTokens->Id,
///     UsdUsdbFileFormatTokens->Id, or UsdUsdcFileFormatTokens->Id.
///
/// If no UsdUsdFileFormatTokens->FormatArg is supplied, the default is
/// UsdUsdcFileFormatTokens->Id.
///
class UsdUsdFileFormat : public SdfFileFormat
{
public:
    using SdfFileFormat::FileFormatArguments;

    USD_API
    virtual SdfAbstractDataRefPtr
    InitData(const FileFormatArguments& args) const;

    USD_API
    virtual bool CanRead(const std::string &file) const;

    USD_API
    virtual bool ReadFromFile(
        const SdfLayerBasePtr& layerBase,
        const std::string& filePath,
        bool metadataOnly) const;

    USD_API
    virtual bool WriteToFile(
        const SdfLayerBase* layerBase,
        const std::string& filePath,
        const std::string& comment = std::string(),
        const FileFormatArguments& args = FileFormatArguments()) const;

    USD_API
    virtual bool ReadFromString(
        const SdfLayerBasePtr& layerBase,
        const std::string& str) const;

    USD_API
    virtual bool WriteToString(
        const SdfLayerBase* layerBase,
        std::string* str,
        const std::string& comment = std::string()) const;

    USD_API
    virtual bool WriteToStream(
        const SdfSpecHandle &spec,
        std::ostream& out,
        size_t indent) const;

protected:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

private:
    UsdUsdFileFormat();
    virtual ~UsdUsdFileFormat();
    
    USD_API static SdfFileFormatConstPtr 
    _GetUnderlyingFileFormatForLayer(const SdfLayerBase* layer);

    virtual bool _IsStreamingLayer(const SdfLayerBase& layer) const;
};

#endif // USD_USD_FILE_FORMAT_H
