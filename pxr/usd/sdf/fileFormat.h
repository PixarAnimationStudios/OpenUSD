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
#ifndef PXR_USD_SDF_FILE_FORMAT_H
#define PXR_USD_SDF_FILE_FORMAT_H

/// \file sdf/fileFormat.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/weakBase.h"

#include <map>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class ArAssetInfo;
class SdfSchemaBase;
class SdfLayerHints;

SDF_DECLARE_HANDLES(SdfLayer);
SDF_DECLARE_HANDLES(SdfSpec);
TF_DECLARE_WEAK_AND_REF_PTRS(SdfAbstractData);
TF_DECLARE_WEAK_AND_REF_PTRS(SdfFileFormat);

#define SDF_FILE_FORMAT_TOKENS   \
    ((TargetArg, "target"))

TF_DECLARE_PUBLIC_TOKENS(SdfFileFormatTokens, SDF_API, SDF_FILE_FORMAT_TOKENS);

/// \class SdfFileFormat
///
/// Base class for file format implementations.
///
class SdfFileFormat
    : public TfRefBase
    , public TfWeakBase
{
public:
    SdfFileFormat(const SdfFileFormat&) = delete;
    SdfFileFormat& operator=(const SdfFileFormat&) = delete;

    /// Returns the schema for this format.
    SDF_API const SdfSchemaBase& GetSchema() const;

    /// Returns the format identifier.
    SDF_API const TfToken& GetFormatId() const;

    /// Returns the target for this file format.
    SDF_API const TfToken& GetTarget() const;

    /// Returns the cookie to be used when writing files with this format.
    SDF_API const std::string& GetFileCookie() const;

    /// Returns the current version of this file format.
    SDF_API const TfToken& GetVersionString() const;

    /// Returns true if this file format is the primary format for the 
    /// extensions it handles.
    SDF_API bool IsPrimaryFormatForExtensions() const;

    /// Returns a list of extensions that this format supports.
    SDF_API const std::vector<std::string>& GetFileExtensions() const;

    /// Returns the primary file extension for this format. This is the
    /// extension that is reported for layers using this file format.
    SDF_API const std::string& GetPrimaryFileExtension() const;

    /// Returns true if \p extension matches one of the extensions returned by
    /// GetFileExtensions.
    SDF_API bool IsSupportedExtension(const std::string& extension) const;

    /// Returns true if this file format is a package containing other
    /// assets.
    SDF_API 
    virtual bool IsPackage() const;

    /// Returns the path of the "root" layer contained in the package
    /// layer at \p resolvedPath produced by this file format. If this 
    /// file format is not a package, returns the empty string.
    ///
    /// The package root layer is the layer in the package layer that 
    /// is used when that package is opened via SdfLayer.
    SDF_API
    virtual std::string GetPackageRootLayerPath(
        const std::string& resolvedPath) const;

    /// Type for specifying additional file format-specific arguments
    /// to the various API below.
    typedef std::map<std::string, std::string> FileFormatArguments;

    /// Returns the FileFormatArguments that correspond to the default behavior
    /// of this file format when no FileFormatArguments are passed to NewLayer
    /// or InitData.
    SDF_API
    virtual FileFormatArguments GetDefaultFileFormatArguments() const;

    /// This method allows the file format to bind to whatever data container is
    /// appropriate. 
    ///
    /// Returns a shared pointer to an SdfAbstractData implementation.
    SDF_API
    virtual SdfAbstractDataRefPtr
    InitData(const FileFormatArguments& args) const;

    /// Instantiate a layer.
    SDF_API 
    SdfLayerRefPtr NewLayer(const SdfFileFormatConstPtr &fileFormat,
                            const std::string &identifier,
                            const std::string &realPath,
                            const ArAssetInfo& assetInfo,
                            const FileFormatArguments &args) const;

    /// Return true if this file format prefers to skip reloading anonymous
    /// layers.
    SDF_API bool ShouldSkipAnonymousReload() const;

    /// Return true if layers produced by this file format are based
    /// on physical files on disk. If so, this file format requires
    /// layers to be serialized to and read from files on disk.
    ///
    /// For file formats where this function returns true, when
    /// opening a layer Sdf will fetch layers to the filesystem 
    /// via calls to ArResolver::FetchToLocalResolvedPath prior 
    /// to calling ReadFromFile.
    ///
    /// This allows asset systems that do not store layers as individual
    /// files to operate with file formats that require these files.
    ///
    /// \sa ArResolver::Resolve
    /// \sa ArResolver::FetchToLocalResolvedPath
    SDF_API bool LayersAreFileBased() const;

    /// Returns true if \p file can be read by this format.
    SDF_API
    virtual bool CanRead(
        const std::string& file) const = 0;

    /// Reads scene description from the asset specified by \p resolvedPath
    /// into the layer \p layer.
    ///
    /// \p metadataOnly is a flag that asks for only the layer metadata
    /// to be read in, which can be much faster if that is all that is
    /// required.  Note that this is just a hint: some FileFormat readers
    /// may disregard this flag and still fully populate the layer contents.
    ///
    /// Returns true if the asset is successfully read into \p layer,
    /// false otherwise.
    SDF_API
    virtual bool Read(
        SdfLayer* layer,
        const std::string& resolvedPath,
        bool metadataOnly) const = 0;

    /// Writes the content in \p layer into the file at \p filePath. If the
    /// content is successfully written, this method returns true. Otherwise,
    /// false is returned and errors are posted. The default implementation
    /// returns false.
    SDF_API
    virtual bool WriteToFile(
        const SdfLayer& layer,
        const std::string& filePath,
        const std::string& comment = std::string(),
        const FileFormatArguments& args = FileFormatArguments()) const;

    /// Reads data in the string \p str into the layer \p layer. If
    /// the file is successfully read, this method returns true. Otherwise,
    /// false is returned and errors are posted.
    SDF_API
    virtual bool ReadFromString(
        SdfLayer* layer,
        const std::string& str) const;

    /// Write the provided \p spec to \p out indented \p indent levels.
    SDF_API
    virtual bool WriteToStream(
        const SdfSpecHandle &spec,
        std::ostream& out,
        size_t indent) const;

    /// Writes the content in \p layer to the string \p str. This function
    /// should write a textual representation of \p layer to the stream
    /// that can be read back in via ReadFromString.
    SDF_API
    virtual bool WriteToString(
        const SdfLayer& layer,
        std::string* str,
        const std::string& comment = std::string()) const;

    /// Returns the set of resolved paths to external asset file dependencies 
    /// for the given \p layer. These are additional dependencies, specific to 
    /// the file format, that are needed when generating the layer's contents
    /// and would not otherwise be discoverable through composition dependencies
    /// (i.e. sublayers, references, and payloads). 
    ///
    /// The default implementation returns an empty set. Derived file formats 
    /// that depend on external assets to read and generate layer content 
    /// should implement this function to return the external asset paths.
    ///
    /// \sa SdfLayer::GetExternalAssetDependencies
    /// \sa SdfLayer::Reload
    SDF_API
    virtual std::set<std::string> GetExternalAssetDependencies(
        const SdfLayer& layer) const;

    /// Returns the file extension for path or file name \p s, without the
    /// leading dot character.
    SDF_API static std::string GetFileExtension(const std::string& s);

    /// Returns a set containing the extension(s) corresponding to 
    /// all registered file formats.
    SDF_API static std::set<std::string> FindAllFileFormatExtensions();

    /// Returns the file format instance with the specified \p formatId
    /// identifier. If a format with a matching identifier is not found, this
    /// returns a null file format pointer.
    SDF_API 
    static SdfFileFormatConstPtr FindById(
        const TfToken& formatId);

    /// Returns the file format instance that supports the extension for
    /// \p path.  If a format with a matching extension is not found, this
    /// returns a null file format pointer.
    ///
    /// An extension may be handled by multiple file formats, but each
    /// with a different target. In such cases, if no \p target is specified, 
    /// the file format that is registered as the primary plugin will be
    /// returned. Otherwise, the file format whose target matches \p target
    /// will be returned.
    SDF_API
    static SdfFileFormatConstPtr FindByExtension(
        const std::string& path,
        const std::string& target = std::string());

    /// Returns a file format instance that supports the extension for \p
    /// path and whose target matches one of those specified by the given
    /// \p args. If the \p args specify no target, then the file format that is
    /// registered as the primary plugin will be returned. If a format with a
    /// matching extension is not found, this returns a null file format
    /// pointer.
    SDF_API
    static SdfFileFormatConstPtr FindByExtension(
        const std::string& path,
        const FileFormatArguments& args);

protected:
    /// Constructor.
    SDF_API SdfFileFormat(
        const TfToken& formatId,
        const TfToken& versionString,
        const TfToken& target,
        const std::string& extension);

    /// Constructor.
    /// \p schema must remain valid for the lifetime of this file format.
    SDF_API SdfFileFormat(
        const TfToken& formatId,
        const TfToken& versionString,
        const TfToken& target,
        const std::string& extension,
        const SdfSchemaBase& schema);

    /// Disallow temporary SdfSchemaBase objects being passed to the c'tor.
    SdfFileFormat(
        const TfToken& formatId,
        const TfToken& versionString,
        const TfToken& target,
        const std::string& extension,
        const SdfSchemaBase&& schema) = delete;

    /// Constructor.
    SDF_API SdfFileFormat(
        const TfToken& formatId,
        const TfToken& versionString,
        const TfToken& target,
        const std::vector<std::string> &extensions);

    /// Constructor.
    /// \p schema must remain valid for the lifetime of this file format.
    SDF_API SdfFileFormat(
        const TfToken& formatId,
        const TfToken& versionString,
        const TfToken& target,
        const std::vector<std::string> &extensions,
        const SdfSchemaBase& schema);

    /// Disallow temporary SdfSchemaBase objects being passed to the c'tor.
    SdfFileFormat(
        const TfToken& formatId,
        const TfToken& versionString,
        const TfToken& target,
        const std::vector<std::string> &extensions,
        const SdfSchemaBase&& schema) = delete;

    /// Destructor.
    SDF_API virtual ~SdfFileFormat();

    //
    // Minimally break layer encapsulation with the following methods.  These
    // methods are also intended to limit the need for SdfLayer friendship with 
    // SdfFileFormat child classes.
    //

    /// Set the internal data for \p layer to \p data, possibly transferring
    /// ownership of \p data.
    /// 
    /// Existing layer hints are reset to the default hints.
    SDF_API
    static void _SetLayerData(
        SdfLayer* layer, SdfAbstractDataRefPtr& data);

    /// Set the internal data for \p layer to \p data, possibly transferring
    /// ownership of \p data.
    ///
    /// Existing layer hints are replaced with \p hints.
    SDF_API
    static void _SetLayerData(
        SdfLayer* layer, SdfAbstractDataRefPtr& data,
        SdfLayerHints hints);

    /// Get the internal data for \p layer.
    SDF_API
    static SdfAbstractDataConstPtr _GetLayerData(const SdfLayer& layer);

private:
    SDF_API
    virtual SdfLayer *_InstantiateNewLayer(
        const SdfFileFormatConstPtr &fileFormat,
        const std::string &identifier,
        const std::string &realPath,
        const ArAssetInfo& assetInfo,
        const FileFormatArguments &args) const;

    // File format subclasses may override this if they prefer not to skip
    // reloading anonymous layers.  Default implementation returns true.
    SDF_API
    virtual bool _ShouldSkipAnonymousReload() const;

    /// File format subclasses may override this to specify whether
    /// their layers are backed by physical files on disk.
    /// Default implementation returns true.
    SDF_API
    virtual bool _LayersAreFileBased() const;

    const SdfSchemaBase& _schema;
    const TfToken _formatId;
    const TfToken _target;
    const std::string _cookie;
    const TfToken _versionString;
    const std::vector<std::string> _extensions;
    const bool _isPrimaryFormat;
};

// Base file format factory.
class Sdf_FileFormatFactoryBase : public TfType::FactoryBase {
public:
    virtual SdfFileFormatRefPtr New() const = 0;
};

// Default file format factory.
template <typename T>
class Sdf_FileFormatFactory : public Sdf_FileFormatFactoryBase {
public:
    virtual SdfFileFormatRefPtr New() const
    {
        return TfCreateRefPtr(new T);
    }
};

/// \def SDF_DEFINE_FILE_FORMAT
///
/// Performs registrations needed for the specified file format class to be
/// discovered by Sdf. This typically would be invoked in a TF_REGISTRY_FUNCTION
/// in the source file defining the file format. 
///
/// The first argument is the name of the file format class being registered. 
/// Subsequent arguments list the base classes of the file format. Since all 
/// file formats must ultimately derive from SdfFileFormat, there should be
/// at least one base class specified.
///
/// For example:
///
/// \code
/// // in MyFileFormat.cpp
/// TF_REGISTRY_FUNCTION(TfType)
/// {
///     SDF_DEFINE_FILE_FORMAT(MyFileFormat, SdfFileFormat);
/// }
/// \endcode
///
#ifdef doxygen
#define SDF_DEFINE_FILE_FORMAT(FileFormatClass, BaseClass1, ...)
#else
#define SDF_DEFINE_FILE_FORMAT(...) SdfDefineFileFormat<__VA_ARGS__>()

template <class FileFormat, class ...BaseFormats>
void SdfDefineFileFormat()
{
    TfType::Define<FileFormat, TfType::Bases<BaseFormats...>>()
        .template SetFactory<Sdf_FileFormatFactory<FileFormat>>();
}
#endif // doxygen

/// \def SDF_DEFINE_ABSTRACT_FILE_FORMAT
///
/// Performs registrations needed for the specified abstract file format
/// class. This is used to register types that serve as base classes
/// for other concrete file format classes used by Sdf.
///
/// The first argument is the name of the file format class being registered.
/// Subsequent arguments list the base classes of the file format. Since all 
/// file formats must ultimately derive from SdfFileFormat, there should be
/// at least one base class specified.
///
/// For example:
///
/// \code
/// // in MyFileFormat.cpp
/// TF_REGISTRY_FUNCTION(TfType)
/// {
///     SDF_DEFINE_ABSTRACT_FILE_FORMAT(MyFileFormat, SdfFileFormat);
/// }
/// \endcode
///
#ifdef doxygen
#define SDF_DEFINE_ABSTRACT_FILE_FORMAT(FileFormatClass, BaseClass1, ...)
#else
#define SDF_DEFINE_ABSTRACT_FILE_FORMAT(...) \
    SdfDefineAbstractFileFormat<__VA_ARGS__>()

template <class FileFormat, class ...BaseFormats>
void SdfDefineAbstractFileFormat()
{
    TfType::Define<FileFormat, TfType::Bases<BaseFormats...>>();
}
#endif //doxygen

/// \def SDF_FILE_FORMAT_FACTORY_ACCESS
///
/// Provides access to allow file format classes to be instantiated
/// from Sdf. This should be specified in the class definition for
/// concrete file format classes.
///
/// For example:
/// 
/// \code
/// // in MyFileFormat.h
/// class MyFileFormat : public SdfFileFormat
/// {
///     SDF_FILE_FORMAT_FACTORY_ACCESS;
///     // ...
/// };
/// \endcode
///
#ifdef doxygen
#define SDF_FILE_FORMAT_FACTORY_ACCESS
#else
#define SDF_FILE_FORMAT_FACTORY_ACCESS \
    template<typename T> friend class Sdf_FileFormatFactory
#endif //doxygen

PXR_NAMESPACE_CLOSE_SCOPE

#endif
