//
// Copyright 2025 Apple
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

/// \file usdPmc/usdPmcFileFormat.hpp

#ifndef USD_PMC_FILE_FORMAT_H
#define USD_PMC_FILE_FORMAT_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/base/tf/staticTokens.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

#define USDPMC_FILE_FORMAT_TOKENS       \
    ((Id,       "pmc"))                 \
    ((Version,  "1.0"))                 \
    ((Target,   "usd"))

TF_DECLARE_PUBLIC_TOKENS(UsdPmcFileFormatTokens, USDPMC_FILE_FORMAT_TOKENS);

TF_DECLARE_WEAK_AND_REF_PTRS(UsdPmcFileFormat);

/// \class UsdPmcFileFormat
///
/// File format plugin for PMC compressed mesh data.
///
/// This file format plugin provides the ability to read PMC compressed
/// mesh files and decompress them into USD geometry. PMC files contain
/// compressed mesh data using the AOMedia Polygonal Mesh Coding compression
/// algorithm.
///
/// The plugin supports reading PMC files and converting them to USD
/// geometry, but does not support writing USD data back to PMC format.
/// Use the PMC compression tools to generate PMC files from USD data.
class UsdPmcFileFormat: public SdfFileFormat {
public:

    /// Determine if the given file can be read by this format.
    /// \param file Path to the file to check
    /// \return true if the file can be read, false otherwise
    virtual bool CanRead(const std::string &file) const override;

    /// Read PMC data from file into USD layer.
    /// \param layer The SdfLayer to populate with decoded data
    /// \param resolvedPath Path to the PMC file to read
    /// \param metadataOnly If true, only read metadata (not supported)
    /// \return true if reading was successful, false otherwise
    virtual bool Read(SdfLayer* layer,
                      const std::string& resolvedPath,
                      bool metadataOnly) const override;

    /// Read PMC data from string into USD layer.
    /// \param layer The SdfLayer to populate with decoded data
    /// \param str String containing PMC data
    /// \return true if reading was successful, false otherwise
    virtual bool ReadFromString(SdfLayer* layer,
                                const std::string& str) const override;

    /// Write USD data to string (delegates to USDA format).
    /// Writing USD data to PMC format is not supported. Use the PMC
    /// compression tools to generate PMC files from USD data.
    /// \param layer The SdfLayer to write
    /// \param str Output string to write to
    /// \param comment Optional comment to include
    /// \return true if writing was successful, false otherwise
    virtual bool WriteToString(const SdfLayer& layer, std::string* str,
                               const std::string& comment=std::string())
                               const override;

    /// Write USD data to stream (delegates to USDA format).
    /// Writing USD data to PMC format is not supported. Use the PMC
    /// compression tools to generate PMC files from USD data.
    /// \param spec The SdfSpec to write
    /// \param out Output stream to write to
    /// \param indent Indentation level
    /// \return true if writing was successful, false otherwise
    virtual bool WriteToStream(const SdfSpecHandle &spec, std::ostream& out,
                               size_t indent) const override;

protected:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    virtual ~UsdPmcFileFormat();
    UsdPmcFileFormat();

private:
    /// Internal method to read PMC data from a buffer into a USD layer
    bool _ReadFromBuffer(SdfLayer* layer, const char* buffer, size_t length,
                         bool metadataOnly, std::string *outErr) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_PMC_FILE_FORMAT_H
