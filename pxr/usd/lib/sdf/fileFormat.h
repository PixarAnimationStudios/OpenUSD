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
///
/// \file sdf/fileFormat.h
///
///

#ifndef SDF_FILE_FORMAT_H
#define SDF_FILE_FORMAT_H

#include "pxr/usd/sdf/declareHandles.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/weakBase.h"

#include <boost/noncopyable.hpp>
#include <map>
#include <string>
#include <vector>

class ArAssetInfo;

SDF_DECLARE_HANDLES(SdfLayer);
SDF_DECLARE_HANDLES(SdfSpec);
TF_DECLARE_WEAK_AND_REF_PTRS(SdfAbstractData);
TF_DECLARE_WEAK_AND_REF_PTRS(SdfLayerBase);
TF_DECLARE_WEAK_AND_REF_PTRS(SdfFileFormat);

#define SDF_FILE_FORMAT_TOKENS   \
    ((TargetArg, "target"))

TF_DECLARE_PUBLIC_TOKENS(SdfFileFormatTokens, SDF_FILE_FORMAT_TOKENS);

/// \class SdfFileFormat
///
/// Base class for file format implementations.
///
class SdfFileFormat : public TfRefBase, public TfWeakBase, boost::noncopyable
{
public:
    /// Returns the format identifier.
    const TfToken& GetFormatId() const;

    /// Returns the target for this file format.
    const TfToken& GetTarget() const;

    /// Returns the cookie to be used when writing files with this format.
    const std::string& GetFileCookie() const;

    /// Returns the current version of this file format.
    const TfToken& GetVersionString() const;

    /// Returns true if this file format is the primary format for the 
    /// extensions it handles.
    bool IsPrimaryFormatForExtensions() const;

    /// Returns a list of extensions that this format supports.
    const std::vector<std::string>& GetFileExtensions() const;

    /// Returns the primary file extension for this format. This is the
    /// extension that is reported for layers using this file format.
    const std::string& GetPrimaryFileExtension() const;

    /// Returns true if \p extension matches one of the extensions returned by
    /// GetFileExtensions.
    bool IsSupportedExtension(const std::string& extension) const;

    /// Type for specifying additional file format-specific arguments
    /// to the various API below.
    typedef std::map<std::string, std::string> FileFormatArguments;

    /// Returns the FileFormatArguments that correspond to the default behavior
    /// of this file format when no FileFormatArguments are passed to NewLayer
    /// or InitData.
    virtual FileFormatArguments GetDefaultFileFormatArguments() const;

    /// This method allows the file format to bind to whatever data container is
    /// appropriate. 
    ///
    /// Returns a shared pointer to an SdfAbstractData implementation.
    virtual SdfAbstractDataRefPtr InitData(const FileFormatArguments& args) const;

    /// Instantiate a layer.
    SdfLayerBaseRefPtr NewLayer(const SdfFileFormatConstPtr &fileFormat,
                                const std::string &identifier,
                                const std::string &realPath,
                                const ArAssetInfo& assetInfo,
                                const FileFormatArguments &args) const;

    /// Instantiate a layer of the SdfLayerBase subclass T.  Issue an error and
    /// return null if the created layer does not have dynamic type T.
    template <class T>
    TfRefPtr<T> NewLayer(const SdfFileFormatConstPtr &fileFormat,
                         const std::string &identifier,
                         const std::string &realPath,
                         const ArAssetInfo &assetInfo,
                         const FileFormatArguments &args) const {
        SdfLayerBaseRefPtr l = 
            NewLayer(fileFormat, identifier, realPath, assetInfo, args);
        if (TfRefPtr<T> ret = TfDynamic_cast<TfRefPtr<T> >(l))
            return ret;
        _IssueNewLayerFailError(l, typeid(T), identifier, realPath);
        return TfRefPtr<T>();
    }

    /// Return true if this file format prefers to skip reloading anonymous
    /// layers.
    bool ShouldSkipAnonymousReload() const;

    /// Return true if the the \p layer produced by this file format
    /// streams its data to and from its serialized data store on demand.
    ///
    /// Sdf will treat streaming layers differently to avoid pulling
    /// in data unnecessarily. For example, reloading a streaming layer 
    /// will not perform fine-grained change notification, since doing 
    /// so would require the full contents of the layer to be loaded.
    ///
    /// Edits to a streaming layer are assumed to immediately affect
    /// the serialized data without an explicit call to SdfLayer::Save.
    ///
    /// It is a coding error to call this function with a layer that was
    /// not created with this file format.
    bool IsStreamingLayer(const SdfLayerBase& layer) const;

    /// Returns true if \p file can be read by this format.
    virtual bool CanRead(
        const std::string& file) const = 0;

    /// Reads data in the file at \p filePath into the layer \p layerBase. If
    /// the file is successfully read, this method returns true. Otherwise,
    /// false is returned and errors are posted.
    ///
    /// \p metadataOnly is a flag that asks for only the layer metadata
    //// to be read in, which can be much faster if that is all that is
    /// required.  Note that this is just a hint: some FileFormat readers
    /// may disregard this flag and still fully populate the layer contents.
    virtual bool ReadFromFile(
        const SdfLayerBasePtr& layerBase,
        const std::string& filePath,
        bool metadataOnly) const = 0;

    /// Writes the content in \p layerBase into the file at \p filePath. If the
    /// content is successfully written, this method returns true. Otherwise,
    /// false is returned and errors are posted. The default implementation
    /// returns false.
    virtual bool WriteToFile(
        const SdfLayerBase* layerBase,
        const std::string& filePath,
        const std::string& comment = std::string(),
        const FileFormatArguments& args = FileFormatArguments()) const;

    /// Reads data in the string \p str into the layer \p layerBase. If
    /// the file is successfully read, this method returns true. Otherwise,
    /// false is returned and errors are posted.
    virtual bool ReadFromString(
        const SdfLayerBasePtr& layerBase,
        const std::string& str) const;

    /// Write the provided \p spec to \p out indented \p indent levels.
    virtual bool WriteToStream(
        const SdfSpecHandle &spec,
        std::ostream& out,
        size_t indent) const;

    /// Writes the content in \p layerBase to the string \p str. This function
    /// should write a textual representation of \p layerBase to the stream
    /// that can be read back in via ReadFromString.
    virtual bool WriteToString(
        const SdfLayerBase* layerBase,
        std::string* str,
        const std::string& comment = std::string()) const;

    /// Returns the file extension for path or file name \p s, without the
    /// leading dot character.
    static std::string GetFileExtension(const std::string& s);

    /// Returns the file format instance with the specified \p formatId
    /// identifier. If a format with a matching identifier is not found, this
    /// returns a null file format pointer.
    static SdfFileFormatConstPtr FindById(
        const TfToken& formatId);

    /// Returns the file format instance that supports the specified file \p
    /// extension. If a format with a matching extension is not found, this
    /// returns a null file format pointer.
    ///
    /// An extension may be handled by multiple file formats, but each
    /// with a different target. In such cases, if no \p target is specified, 
    /// the file format that is registered as the primary plugin will be
    /// returned. Otherwise, the file format whose target matches \p target
    /// will be returned.
    static SdfFileFormatConstPtr FindByExtension(
        const std::string& extension,
        const std::string& target = std::string());

protected:
    /// Constructor.
    SdfFileFormat(
        const TfToken& formatId,
        const TfToken& versionString,
        const TfToken& target,
        const std::string& extensions);

    /// Constructor.
    SdfFileFormat(
        const TfToken& formatId,
        const TfToken& versionString,
        const TfToken& target,
        const std::vector<std::string> &extensions);

    /// Destructor.
    virtual ~SdfFileFormat();

    //
    // Minimally break layer encapsulation with the following methods.  These
    // methods are also intended to limit the need for SdLayer friendship with 
    // SdFileFormat child classes.
    //

    /// Swap the layers internal _data member with \p data.
    static void _SwapLayerData(const SdfLayerHandle& layer,
                               SdfAbstractDataRefPtr& data);

    /// Set the layers internal _data member to \p data.
    static void _SetLayerData(const SdfLayerHandle& layer,
                              const SdfAbstractDataPtr& data);

    /// Get the layers internal _data member.
    static SdfAbstractDataConstPtr _GetLayerData(const SdfLayerHandle& layer);

    /// Returns true if the layer is loading as a new layer.
    static bool _LayerIsLoadingAsNew(const SdfLayerHandle& layer);

private:
    virtual SdfLayerBase *_InstantiateNewLayer(
        const SdfFileFormatConstPtr &fileFormat,
        const std::string &identifier,
        const std::string &realPath,
        const ArAssetInfo& assetInfo,
        const FileFormatArguments &args) const;

    // File format subclasses may override this if they prefer not to skip
    // reloading anonymous layers.  Default implementation returns true.
    virtual bool _ShouldSkipAnonymousReload() const;

    // File format subclasses must override this to determine whether the
    // given layer is streaming or not. The file format of \p layer is 
    // guaranteed to be an instance of this class.
    virtual bool _IsStreamingLayer(const SdfLayerBase& layer) const = 0;

    // Helper to issue an error in case the method template NewLayer fails.
    void _IssueNewLayerFailError(SdfLayerBaseRefPtr const &l,
                                 std::type_info const &type,
                                 std::string const &identifier,
                                 std::string const &realPath) const;


    const TfToken _formatId;
    const TfToken _target;
    const std::string _cookie;
    const TfToken _versionString;
    const std::vector<std::string> _extensions;
    const bool _isPrimaryFormat;
};

/// Base file format factory.
class Sdf_FileFormatFactoryBase : public TfType::FactoryBase {
public:
    virtual SdfFileFormatRefPtr New() const = 0;
};

/// Default file format factory.
template <typename T>
class Sdf_FileFormatFactory : public Sdf_FileFormatFactoryBase {
public:
    virtual SdfFileFormatRefPtr New() const
    {
        return TfCreateRefPtr(new T);
    }
};

/// Defines a file format and factory. This macro is intended for use in a
/// TfType registry function block. It defines a type for the first argument,
/// with optional bases as additional arguments, and adds a factory.
#define SDF_DEFINE_FILE_FORMAT(c, ...) \
    TfType::Define<c BOOST_PP_COMMA_IF(TF_NUM_ARGS(__VA_ARGS__)) \
        BOOST_PP_IF(TF_NUM_ARGS(__VA_ARGS__), \
            TfType::Bases<__VA_ARGS__>, BOOST_PP_EMPTY) >() \
        .SetFactory<Sdf_FileFormatFactory<c> >()

/// Defines a file format without a factory. This macro is intended for use in
/// a TfType registry function block. It defines a type for the first
/// argument, with optional bases as additional arguments.
#define SDF_DEFINE_ABSTRACT_FILE_FORMAT(c, ...) \
    TfType::Define<c BOOST_PP_COMMA_IF(TF_NUM_ARGS(__VA_ARGS__)) \
        BOOST_PP_IF(TF_NUM_ARGS(__VA_ARGS__), \
            TfType::Bases<__VA_ARGS__>, BOOST_PP_EMPTY) >();

#define SDF_FILE_FORMAT_FACTORY_ACCESS \
    template<typename T> friend class Sdf_FileFormatFactory

#endif // SDF_FILE_FORMAT_H
