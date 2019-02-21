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
#ifndef USD_USDC_FILE_FORMAT_H
#define USD_USDC_FILE_FORMAT_H
 
#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/base/tf/staticTokens.h"
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

#define USD_USDC_FILE_FORMAT_TOKENS   \
    ((Id,      "usdc"))

TF_DECLARE_PUBLIC_TOKENS(UsdUsdcFileFormatTokens, USD_API, USD_USDC_FILE_FORMAT_TOKENS);

TF_DECLARE_WEAK_AND_REF_PTRS(UsdUsdcFileFormat);

/// \class UsdUsdcFileFormat
///
/// File format for binary Usd files.
///
class UsdUsdcFileFormat : public SdfFileFormat
{
public:
    using SdfFileFormat::FileFormatArguments;
    using string = std::string;

    virtual SdfAbstractDataRefPtr InitData(
        const FileFormatArguments& args) const override;

    virtual bool CanRead(const string &file) const override;

    virtual bool Read(
        SdfLayer* layer,
        const string& resolvedPath,
        bool metadataOnly) const override;

    virtual bool WriteToFile(
        const SdfLayer& layer,
        const string& filePath,
        const string& comment = string(),
        const FileFormatArguments& args = FileFormatArguments()) const override;

    virtual bool ReadFromString(SdfLayer* layer,
                                const string& str) const override;

    virtual bool WriteToString(const SdfLayer& layer,
                               string* str,
                               const string& comment = string()) const override;

    virtual bool WriteToStream(const SdfSpecHandle &spec,
                               std::ostream& out,
                               size_t indent) const override;

protected:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    UsdUsdcFileFormat();
    virtual ~UsdUsdcFileFormat();

private:
    virtual bool _IsStreamingLayer(const SdfLayer& layer) const override;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_USDC_FILE_FORMAT_H
