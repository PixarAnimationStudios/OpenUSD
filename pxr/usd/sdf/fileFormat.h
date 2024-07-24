//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_FILE_FORMAT_H
#define PXR_USD_SDF_FILE_FORMAT_H

/// \file sdf/fileFormat.h

#include "pxr/pxr.h"
#include "pxr/usd/ar/ar.h"
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

    /// Returns a new SdfAbstractData providing access to the layer's data.
    /// This data object is detached from any underlying storage.
    SDF_API
    SdfAbstractDataRefPtr InitDetachedData(
        const FileFormatArguments& args) const;

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

    /// Returns true if anonymous layer identifiers should be passed to Read 
    /// when a layer is opened or reloaded.
    /// 
    /// Anonymous layers will not have an asset backing and thus for most
    /// file formats there is nothing that can be read for an anonymous layer. 
    /// However, there are file formats that use Read to generate dynamic layer 
    /// content without reading any data from the resolved asset associated with
    /// the layer's identifier. 
    /// 
    /// For these types of file formats it is useful to be able to open 
    /// anonymous layers and allow Read to populate them to avoid requiring a
    /// placeholder asset to exist just so Read can populate the layer.
    SDF_API bool ShouldReadAnonymousLayers() const;

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

    /// Reads scene description from the asset specified by \p resolvedPath
    /// into the detached layer \p layer. After reading is completed,
    /// \p layer must be detached from any underlying storage.
    ///
    /// \p metadataOnly is a flag that asks for only the layer metadata
    /// to be read in, which can be much faster if that is all that is
    /// required.  Note that this is just a hint: some FileFormat readers
    /// may disregard this flag and still fully populate the layer contents.
    ///
    /// Returns true if the asset is successfully read into \p layer,
    /// false if the the asset could not be read or if the resulting
    /// layer is not detached.
    SDF_API
    bool ReadDetached(
        SdfLayer* layer,
        const std::string& resolvedPath,
        bool metadataOnly) const;

    /// Writes the content in \p layer into the file at \p filePath. If the
    /// content is successfully written, this method returns true. Otherwise,
    /// false is returned and errors are posted. The default implementation
    /// returns false.
    ///
    /// This member function makes no distinction between a "Save" operation
    /// that updates the backing store for the \p layer itself and an "Export"
    /// operation that writes the \p layer data to a distinct asset.  For file
    /// formats that retain all data in memory this is typically fine.  But for
    /// file formats that handle data requests by reading from the backing
    /// store, this distinction can be important.  In that case, additionally
    /// override the member function SaveToFile() to take different action.
    SDF_API
    virtual bool WriteToFile(
        const SdfLayer& layer,
        const std::string& filePath,
        const std::string& comment = std::string(),
        const FileFormatArguments& args = FileFormatArguments()) const;

    /// Write the content in \p layer to the file at \p filePath, which is the
    /// backing store for \p layer itself.  If the content is successfully
    /// written, this method returns true. Otherwise, false is returned and
    /// errors are posted. The default implementation just calls WriteToFile()
    /// passing all the same arguments.
    ///
    /// The purpose of this member function is to provide a distinction between
    /// a "Save" operation that updates the backing store for the \p layer
    /// itself and an "Export" operation that writes the \p layer data to a
    /// distinct asset.  File formats that retain all data in memory can
    /// typically override only WriteToFile(), but formats that do not may need
    /// to take different action on "Save" vs "Export".
    SDF_API
    virtual bool SaveToFile(
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


    /// Returns true if this file format supports reading.
    /// This is a convenience method for invoking \ref FormatSupportsReading
    /// with this format's extension and target
    SDF_API bool SupportsReading() const;

    // Returns true if this file format supports writing.
    /// This is a convenience method for invoking \ref FormatSupportsWriting
    /// with this format's extension and target
    SDF_API bool SupportsWriting() const;

    // Returns true if this file format supports editing.
    /// This is a convenience method for invoking \ref FormatSupportsEditing
    /// with this format's extension and target
    SDF_API bool SupportsEditing() const;

    /// Returns the file extension for path or file name \p s, without the
    /// leading dot character.
    SDF_API static std::string GetFileExtension(const std::string& s);

    /// Returns a set containing the extension(s) corresponding to 
    /// all registered file formats.
    SDF_API static std::set<std::string> FindAllFileFormatExtensions();

    /// Returns a set containing the extension(s) corresponding to
    /// all registered file formats that derive from \p baseType.
    ///
    /// \p baseType must derive from SdfFileFormat.
    SDF_API static std::set<std::string> FindAllDerivedFileFormatExtensions(
        const TfType& baseType);

    /// Returns true if the file format for the supplied \p extension and
    /// \p target pair supports reading.
    /// This method will not load the plugin that provides the specified 
    /// file format.
    /// If the extension and target pair is invalid, this method will
    /// return false.
    /// \sa FormatSupportsWriting \sa FormatSupportsEditing
    SDF_API
    static bool FormatSupportsReading(
        const std::string& extension,
        const std::string& target = std::string());

    /// Returns true if the file format for the supplied \p extension and 
    /// \p target pair supports writing.
    /// This method will not load the plugin that provides the specified 
    /// file format.
    /// If the extension and target pair is invalid, this method will return
    /// false.
    /// \sa FormatSupportsReading \sa FormatSupportsEditing
    SDF_API
    static bool FormatSupportsWriting(
        const std::string& extension,
        const std::string& target = std::string());

    /// Returns true if the file format for the supplied \p extension and 
    /// \p target pair supports editing.
    /// This method will not load the plugin that provides the specified 
    /// file format.
    /// If the extension and target pair is invalid, this method will return
    /// false.
    /// \sa FormatSupportsReading \sa FormatSupportsWriting
    SDF_API
    static bool FormatSupportsEditing(
        const std::string& extension,
        const std::string& target = std::string());

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

    /// Helper function for _ReadDetached.
    ///
    /// Calls Read with the given parameters. If successful and \p layer is
    /// not detached (i.e., SdfLayer::IsDetached returns false) copies the layer
    /// data into an SdfData object and set that into \p layer. If this copy
    /// occurs and \p didCopyData is given, it will be set to true.
    ///
    /// Note that the copying process is a simple spec-by-spec, field-by-field
    /// value copy. This process may not produce detached layers if the data
    /// object used by \p layer after the initial call to Read returns VtValues
    /// that are not detached. One example is a VtValue holding a VtArray backed
    /// by a foreign data source attached to a memory mapping.
    ///
    /// Returns true if Read was successful, false otherwise.
    SDF_API
    bool _ReadAndCopyLayerDataToMemory(
        SdfLayer* layer,
        const std::string& resolvedPath,
        bool metadataOnly,
        bool* didCopyData = nullptr) const;

protected:
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
    /// Read should be called when creating, opening, or reloading an anonymous
    /// layer of this format.
    /// Default implementation returns false.
    SDF_API 
    virtual bool _ShouldReadAnonymousLayers() const;

    /// \see InitDetachedData
    ///
    /// This function must return a new SdfAbstractData object that is
    /// detached, i.e. SdfAbstractData::IsDetached returns false.
    ///
    /// The default implementation returns an SdfData object.
    SDF_API
    virtual SdfAbstractDataRefPtr _InitDetachedData(
        const FileFormatArguments& args) const;

    /// \see ReadDetached
    ///
    /// Upon completion, \p layer must have an SdfAbstractData object set that
    /// is detached, i.e. SdfAbstractData::IsDetached returns false.
    ///
    /// The default implementation calls _ReadAndCopyLayerDataToMemory to read
    /// the specified layer and copy its data into an SdfData object if it is
    /// not detached. If data is copied, a warning will be issued since
    /// this may be an expensive operation. If the above behavior is desired,
    /// subclasses can just call _ReadAndCopyLayerDataToMemory to do the same
    /// thing but without the warning.
    SDF_API
    virtual bool _ReadDetached(
        SdfLayer* layer,
        const std::string& resolvedPath,
        bool metadataOnly) const;

private:
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
    SDF_API virtual ~Sdf_FileFormatFactoryBase();
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
