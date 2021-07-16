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
#ifndef PXR_USD_USD_ANIMX_FILE_FORMAT_H
#define PXR_USD_USD_ANIMX_FILE_FORMAT_H
 
#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/sdf/fileFormat.h"
//#include "pxr/usd/pcp/dynamicFileFormatInterface.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/vt/value.h"
#include <vector>
#include <string>
#include <iostream>
#include "pxr/usd/usdAnimX/api.h"
#include "pxr/usd/usdAnimX/desc.h"

PXR_NAMESPACE_OPEN_SCOPE

#define USD_ANIMX_FILE_FORMAT_TOKENS \
    ((Id,      "animx"))             \
    ((Version, "1.0"))               \
    ((Target,  "usd"))               \
    ((Extension, "animx"))

TF_DECLARE_PUBLIC_TOKENS(UsdAnimXFileFormatTokens, USD_API, USD_ANIMX_FILE_FORMAT_TOKENS);

TF_DECLARE_WEAK_AND_REF_PTRS(UsdAnimXFileFormat);

/// \class UsdAnimXFileFormat
///
/// File format used by textual animx files.
///
class UsdAnimXFileFormat : public SdfFileFormat
/*, public PcpDynamicFileFormatInterface */
{
public:
    /// Override this function from SdfFileFormat to provide our own procedural
    /// SdfAbstractData class.
    SdfAbstractDataRefPtr InitData(
        const FileFormatArguments &args) const override;

    ANIMX_API
    bool Read(SdfLayer* layer,
              const std::string& resolvedPath,
              bool metadataOnly) const override;

    ANIMX_API
    bool CanRead(const std::string& filePath) const override;

    ANIMX_API
    SdfAbstractDataConstPtr GetData(SdfLayer* layer) const;

    ANIMX_API
    bool WriteToFile(   const SdfLayer& layer,
                        const std::string& filePath,
                        const std::string& comment = std::string(),
                        const FileFormatArguments& args = 
                            FileFormatArguments()) const override;
    ANIMX_API
    bool ReadFromString(    SdfLayer* layer,
                            const std::string& str) const override;
    ANIMX_API
    bool WriteToString( const SdfLayer& layer,
                        std::string* str,
                        const std::string& comment=std::string()) 
                        const override;
    ANIMX_API
    bool WriteToStream( const SdfSpecHandle &spec,
                        std::ostream& out,
                        size_t indent) const override;

    /*
     // Required PcpDynamicFileFormatInterface overrides
    void ComposeFieldsForFileFormatArguments(
        const std::string& assetPath, 
        const PcpDynamicFileFormatContext& context,
        FileFormatArguments* args,
        VtValue *dependencyContextData) const override;
    bool CanFieldChangeAffectFileFormatArguments(
        const TfToken& field,
        const VtValue& oldValue,
        const VtValue& newValue,
        const VtValue &dependencyContextData) const override;
    */
    

private:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    UsdAnimXFileFormat();
    virtual ~UsdAnimXFileFormat();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_ANIMX_FILE_FORMAT_H
