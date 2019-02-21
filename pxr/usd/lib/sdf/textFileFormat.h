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
#ifndef SD_MENVA_FILE_FORMAT_H
#define SD_MENVA_FILE_FORMAT_H

/// \file sdf/textFileFormat.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/declareHandles.h" 
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/base/tf/staticTokens.h"

#include <iosfwd>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

#define SDF_TEXT_FILE_FORMAT_TOKENS \
    ((Id,      "sdf"))              \
    ((Version, "1.4.32"))           \
    ((Target,  "sdf"))

TF_DECLARE_PUBLIC_TOKENS(SdfTextFileFormatTokens,
                         SDF_API, SDF_TEXT_FILE_FORMAT_TOKENS);

TF_DECLARE_WEAK_AND_REF_PTRS(SdfTextFileFormat);
TF_DECLARE_WEAK_AND_REF_PTRS(SdfLayerBase);

SDF_DECLARE_HANDLES(SdfSpec);

/// \class SdfTextFileFormat
///
/// Sdf text file format
///
class SdfTextFileFormat : public SdfFileFormat
{
public:
    /// Writes the content of the layer \p layer to the stream \p ostr. If
    /// \p comment is non-empty, the supplied text is written into the stream
    /// instead of any existing layer comment, without changing the existing
    /// comment. Returns true if the content is successfully written to
    /// stream. Otherwise, false is returned and errors are posted.
    SDF_API
    bool Write(
        const SdfLayer& layer,
        std::ostream& ostr,
        const std::string& comment = std::string()) const;

    /// Writes the content in \p layer into the stream \p ostr. If the
    /// content is successfully written, this method returns true. Otherwise,
    /// false is returned and errors are posted. 
    SDF_API
    bool WriteToStream(
        const SdfLayer& layer,
        std::ostream& ostr) const;

    // SdfFileFormat overrides.
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

    /// Destructor.
    SDF_API
    virtual ~SdfTextFileFormat();

    /// Constructor.
    SDF_API
    SdfTextFileFormat();

    /// Constructor. This form of the constructor may be used by formats that
    /// use menva as their internal representation.  If a non-empty
    /// versionString is provided, it will be used as the file format version;
    /// otherwise the menva format version will be implicitly used.
    SDF_API
    explicit SdfTextFileFormat(const TfToken& formatId,
                               const TfToken& versionString = TfToken(),
                               const TfToken& target = TfToken());

private:

    // Override to return false.  Reloading anonymous menva files clears their
    // content.
    SDF_API virtual bool _ShouldSkipAnonymousReload() const override;

    SDF_API 
    virtual bool _IsStreamingLayer(const SdfLayer& layer) const override;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_TEXT_FILE_FORMAT_H
