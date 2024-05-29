//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USDMTLX_FILE_FORMAT_H
#define PXR_USD_USDMTLX_FILE_FORMAT_H
 
#include "pxr/pxr.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define USDMTLX_FILE_FORMAT_TOKENS    \
    ((Id,      "mtlx"))                         \
    ((Version, "1.0"))                          \
    ((Target,  "usd"))

TF_DECLARE_PUBLIC_TOKENS(UsdMtlxFileFormatTokens, USDMTLX_FILE_FORMAT_TOKENS);

TF_DECLARE_WEAK_AND_REF_PTRS(UsdMtlxFileFormat);

/// \class UsdMtlxFileFormat
///
class UsdMtlxFileFormat : public SdfFileFormat {
public:
    // SdfFileFormat overrides
    SdfAbstractDataRefPtr InitData(const FileFormatArguments&) const override;
    bool CanRead(const std::string &file) const override;
    bool Read(SdfLayer* layer,
              const std::string& resolvedPath,
              bool metadataOnly) const override;
    bool WriteToFile(const SdfLayer& layer,
                     const std::string& filePath,
                     const std::string& comment = std::string(),
                     const FileFormatArguments& args = 
                         FileFormatArguments()) const override;
    bool ReadFromString(SdfLayer* layer,
                        const std::string& str) const override;
    bool WriteToString(const SdfLayer& layer,
                       std::string* str,
                       const std::string& comment=std::string()) const override;
    bool WriteToStream(const SdfSpecHandle &spec,
                       std::ostream& out,
                       size_t indent) const override;

protected:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    UsdMtlxFileFormat();
    ~UsdMtlxFileFormat() override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USDMTLX_FILE_FORMAT_H
