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
#ifndef USDABC_ALEMBIC_FILE_FORMAT_H
#define USDABC_ALEMBIC_FILE_FORMAT_H
 
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/base/tf/staticTokens.h"
#include <iosfwd>
#include <string>

#define USDABC_ALEMBIC_FILE_FORMAT_TOKENS  \
    ((Id,      "abc"))                  \
    ((Version, "1.0"))                  \
    ((Target,  "usd"))

TF_DECLARE_PUBLIC_TOKENS(UsdAbcAlembicFileFormatTokens, USDABC_ALEMBIC_FILE_FORMAT_TOKENS);

TF_DECLARE_WEAK_AND_REF_PTRS(UsdAbcAlembicFileFormat);
TF_DECLARE_WEAK_AND_REF_PTRS(SdfLayerBase);

/// \class UsdAbcAlembicFileFormat
///
class UsdAbcAlembicFileFormat : public SdfFileFormat {
public:
    // SdfFileFormat overrides
    virtual SdfAbstractDataRefPtr InitData(const FileFormatArguments&) const;
    virtual bool CanRead(const std::string &file) const;
    virtual bool Read(const SdfLayerBasePtr& layerBase,
                      const std::string& resolvedPath,
                      bool metadataOnly) const;
    virtual bool WriteToFile(const SdfLayerBase* layerBase,
                             const std::string& filePath,
                             const std::string& comment = std::string(),
                             const FileFormatArguments& args = 
                                 FileFormatArguments()) const;
    virtual bool ReadFromString(const SdfLayerBasePtr& layerBase,
                                const std::string& str) const;
    virtual bool WriteToString(const SdfLayerBase* layerBase,
                               std::string* str,
                               const std::string& comment=std::string()) const;
    virtual bool WriteToStream(const SdfSpecHandle &spec,
                               std::ostream& out,
                               size_t indent) const;

protected:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    virtual ~UsdAbcAlembicFileFormat();

    UsdAbcAlembicFileFormat();

private:
    virtual bool _IsStreamingLayer(const SdfLayerBase& layer) const;

private:
    SdfFileFormatConstPtr _usda;
};

#endif // USDABC_ALEMBIC_FILE_FORMAT_H
