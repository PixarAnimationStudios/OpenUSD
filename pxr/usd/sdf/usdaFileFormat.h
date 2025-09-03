//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_USDA_FILE_FORMAT_H
#define PXR_USD_SDF_USDA_FILE_FORMAT_H
 
#include "pxr/pxr.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/declareHandles.h" 
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/fileVersion.h"

#include <iosfwd>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

SDF_API
extern TfEnvSetting<std::string> USD_WRITE_NEW_USDA_FILES_AS_VERSION;

#define SDF_USDA_FILE_FORMAT_TOKENS \
    ((Id,      "usda"))             \
    ((Version, "1.0"))

TF_DECLARE_PUBLIC_TOKENS(SdfUsdaFileFormatTokens, SDF_API, SDF_USDA_FILE_FORMAT_TOKENS);

TF_DECLARE_WEAK_AND_REF_PTRS(SdfUsdaFileFormat);

SDF_DECLARE_HANDLES(SdfSpec);

class ArAsset;

/// Environment variable indicating the number of MB at which warnings
/// will be emitted when reading a SdfUsdaFileFormat-derived text file.
///
/// \deprecated
/// The env variable functionality will remain, but its API exposure below
/// will be removed in a subsequent release.
SDF_API
extern TfEnvSetting<int> SDF_TEXTFILE_SIZE_WARNING_MB;

/// \class SdfUsdaFileFormat
///
/// File format used by textual USD files.
///
class SdfUsdaFileFormat : public SdfFileFormat
{
public:
    // SdfFileFormat overrides.
    SDF_API
    virtual SdfAbstractDataRefPtr InitData(
        const FileFormatArguments& args) const override;

    SDF_API
    virtual bool CanRead(const std::string &file) const override;

    SDF_API
    virtual bool Read(
        SdfLayer* layer,
        const std::string& resolvedPath,
        bool metadataOnly) const override;

    /// \brief \c WriteToFile writes the layer contents to the file starting
    /// with the default output version and upgrading as needed.
    SDF_API
    virtual bool WriteToFile(
        const SdfLayer& layer,
        const std::string& filePath,
        const std::string& comment = std::string(),
        const FileFormatArguments& args = FileFormatArguments()) const override;

    /// \brief \c SaveToFile writes the layer contents to the file starting
    /// with the loaded layer's file version and upgrading as needed.
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

public:
    /// These methods return version info for the current version of the
    /// \c SdfUsdaFileFormat. These versions will be kept in sync with the
    /// abilities of the parsing and writing code, but it's convenient to
    /// gather them all here.
    /// @{

    /// Return the minimum version that is is possible for the software to read.
    static SdfFileVersion GetMinInputVersion();

    /// Return the minimum version that it is possible for the software to write.
    static SdfFileVersion GetMinOutputVersion();

    /// Return the maximum version that is is possible for the software to read.
    static SdfFileVersion GetMaxInputVersion();

    /// Return the maximum version that it is possible for the software to write.
    static SdfFileVersion GetMaxOutputVersion();

    /// Return the default version for newly created files.
    static SdfFileVersion GetDefaultOutputVersion();

    /// @}

protected:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    /// Destructor.
    SDF_API
    virtual ~SdfUsdaFileFormat();

    /// Constructor.
    SDF_API
    SdfUsdaFileFormat();

    /// Constructor. This form of the constructor may be used by formats that
    /// use the .usda text format as their internal representation. 
    /// If a non-empty versionString and target are provided, they will be
    /// used as the file format version and target; otherwise the .usda format
    /// version and target will be implicitly used.
    SDF_API
    explicit SdfUsdaFileFormat(const TfToken& formatId,
                               const TfToken& versionString = TfToken(),
                               const TfToken& target = TfToken());

    /// Return true if layer can be read from \p asset at \p resolvedPath.
    SDF_API
    bool _CanReadFromAsset(
        const std::string& resolvedPath,
        const std::shared_ptr<ArAsset>& asset) const;

    /// Read layer from \p asset at \p resolvedPath into \p layer.
    SDF_API 
    bool _ReadFromAsset(
        SdfLayer* layer, 
        const std::string& resolvedPath,
        const std::shared_ptr<ArAsset>& asset,
        bool metadataOnly) const;

private:
    // Override to return false.  Reloading anonymous text layers clears their
    // content.
    SDF_API virtual bool _ShouldSkipAnonymousReload() const override;

    friend class SdfUsdFileFormat;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_USDA_FILE_FORMAT_H
