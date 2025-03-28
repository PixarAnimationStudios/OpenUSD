//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_USDZ_FILE_FORMAT_H
#define PXR_USD_USD_USDZ_FILE_FORMAT_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/base/tf/staticTokens.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(UsdUsdzFileFormat);

#define USD_USDZ_FILE_FORMAT_TOKENS  \
    ((Id,      "usdz"))              \
    ((Version, "1.0"))               \
    ((Target,  "usd"))

TF_DECLARE_PUBLIC_TOKENS(
    UsdUsdzFileFormatTokens, USD_API, USD_USDZ_FILE_FORMAT_TOKENS);

/// \class UsdUsdzFileFormat
///
/// File format for package .usdz files.
class UsdUsdzFileFormat : public SdfFileFormat
{
public:
    using SdfFileFormat::FileFormatArguments;

    USD_API
    virtual bool IsPackage() const override;

    USD_API
    virtual std::string GetPackageRootLayerPath(
        const std::string& resolvedPath) const override;

    USD_API
    virtual SdfAbstractDataRefPtr
    InitData(const FileFormatArguments& args) const override;

    USD_API
    virtual bool CanRead(const std::string &file) const override;

    USD_API
    virtual bool Read(
        SdfLayer* layer,
        const std::string& resolvedPath,
        bool metadataOnly) const override;

    USD_API
    virtual bool WriteToFile(
        const SdfLayer& layer,
        const std::string& filePath,
        const std::string& comment = std::string(),
        const FileFormatArguments& args = FileFormatArguments()) const override;

    USD_API
    virtual bool ReadFromString(
        SdfLayer* layer,
        const std::string& str) const override;

    USD_API
    virtual bool WriteToString(
        const SdfLayer& layer,
        std::string* str,
        const std::string& comment = std::string()) const override;

    USD_API
    virtual bool WriteToStream(
        const SdfSpecHandle &spec,
        std::ostream& out,
        size_t indent) const override;

protected:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    bool _ReadDetached(
        SdfLayer* layer,
        const std::string& resolvedPath,
        bool metadataOnly) const override;

private:
    UsdUsdzFileFormat();
    virtual ~UsdUsdzFileFormat();

    template <bool Detached>
    bool _ReadHelper(
        SdfLayer* layer,
        const std::string& resolvedPath,
        bool metadataOnly) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_USDZ_FILE_FORMAT_H
