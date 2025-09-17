//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_USD_FILE_FORMAT_H
#define PXR_USD_SDF_USD_FILE_FORMAT_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/base/tf/staticTokens.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(SdfUsdFileFormat);

#define SDF_USD_FILE_FORMAT_TOKENS  \
    ((Id,           "usd"))         \
    ((Version,      "1.0"))         \
    ((Target,       "usd"))         \
    ((FormatArg,    "format"))

TF_DECLARE_PUBLIC_TOKENS(SdfUsdFileFormatTokens, SDF_API,
    SDF_USD_FILE_FORMAT_TOKENS);

/// \class SdfUsdFileFormat
///
/// File format for USD files.
///
/// When creating a file through the SdfLayer::CreateNew() interface, the
/// meaningful SdfFileFormat::FileFormatArguments are as follows:
/// \li SdfUsdFileFormatTokens->FormatArg , which must be a supported format's
///     'Id'.  The possible values are SdfUsdaFileFormatTokens->Id
///     or SdfUsdcFileFormatTokens->Id.
///
/// If no SdfUsdFileFormatTokens->FormatArg is supplied, the default is
/// SdfUsdcFileFormatTokens->Id.
///
class SdfUsdFileFormat : public SdfFileFormat
{
public:
    using SdfFileFormat::FileFormatArguments;

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
    virtual bool SaveToFile(
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

    /// Returns the value of the "format" argument to be used in the 
    /// FileFormatArguments when exporting or saving the given layer.
    /// 
    /// Returns an empty token if the given layer does not have this 
    /// file format.
    SDF_API
    static TfToken GetUnderlyingFormatForLayer(const SdfLayer& layer);

protected:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    SdfAbstractDataRefPtr _InitDetachedData(
        const FileFormatArguments& args) const override;

    bool _ReadDetached(
        SdfLayer* layer,
        const std::string& resolvedPath,
        bool metadataOnly) const override;

private:
    SdfUsdFileFormat();
    virtual ~SdfUsdFileFormat();
    
    static SdfFileFormatConstPtr 
    _GetUnderlyingFileFormatForLayer(const SdfLayer& layer);

    template <bool Detached>
    bool _ReadHelper(
        SdfLayer* layer,
        const std::string& resolvedPath,
        bool metadataOnly) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_USD_FILE_FORMAT_H
