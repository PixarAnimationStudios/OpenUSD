//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_USDC_FILE_FORMAT_H
#define PXR_USD_USD_USDC_FILE_FORMAT_H
 
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

class ArAsset;

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

    virtual bool SaveToFile(
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
    friend class UsdUsdFileFormat;

    SdfAbstractDataRefPtr _InitDetachedData(
        const FileFormatArguments& args) const override;

    bool _ReadDetached(
        SdfLayer* layer,
        const std::string& resolvedPath,
        bool metadataOnly) const override;

    bool _CanReadFromAsset(
        const std::string& resolvedPath,
        const std::shared_ptr<ArAsset>& asset) const;

    bool _ReadFromAsset(
        SdfLayer* layer, 
        const std::string& resolvedPath,
        const std::shared_ptr<ArAsset>& asset,
        bool metadataOnly,
        bool detached) const;

    template <class ...Args>
    bool _ReadHelper(
        SdfLayer* layer, 
        const std::string& resolvedPath,
        bool metadataOnly,
        Args&&... args) const;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_USDC_FILE_FORMAT_H
