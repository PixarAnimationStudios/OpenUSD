//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_USDZ_FILE_FORMAT_H
#define PXR_USD_SDF_USDZ_FILE_FORMAT_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/base/tf/staticTokens.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(SdfUsdzFileFormat);

#define SDF_USDZ_FILE_FORMAT_TOKENS  \
    ((Id,      "usdz"))              \
    ((Version, "1.0"))               \
    ((Target,  "usd"))

TF_DECLARE_PUBLIC_TOKENS(
    SdfUsdzFileFormatTokens, SDF_API, SDF_USDZ_FILE_FORMAT_TOKENS);

/// \class SdfUsdzFileFormat
///
/// File format for package .usdz files.
class SdfUsdzFileFormat : public SdfFileFormat
{
public:
    using SdfFileFormat::FileFormatArguments;

    SDF_API
    virtual bool IsPackage() const override;

    SDF_API
    virtual std::string GetPackageRootLayerPath(
        const std::string& resolvedPath) const override;

    SDF_API
    virtual SdfAbstractDataRefPtr
    InitData(const FileFormatArguments& args) const override;

    SDF_API
    virtual bool CanRead(const std::string &file) const override;

    SDF_API
    virtual bool Read(
        SdfLayer* layer,
        const std::string& resolvedPath,
        bool metadataOnly) const override;

    SDF_API
    virtual bool WriteToFile(
        const SdfLayer& layer,
        const std::string& filePath,
        const std::string& comment = std::string(),
        const FileFormatArguments& args = FileFormatArguments()) const override;

    SDF_API
    virtual bool ReadFromString(
        SdfLayer* layer,
        const std::string& str) const override;

    SDF_API
    virtual bool WriteToString(
        const SdfLayer& layer,
        std::string* str,
        const std::string& comment = std::string()) const override;

    SDF_API
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
    SdfUsdzFileFormat();
    virtual ~SdfUsdzFileFormat();

    template <bool Detached>
    bool _ReadHelper(
        SdfLayer* layer,
        const std::string& resolvedPath,
        bool metadataOnly) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_USDZ_FILE_FORMAT_H
