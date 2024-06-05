//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_FILE_FORMAT_REGISTRY_H
#define PXR_USD_SDF_FILE_FORMAT_REGISTRY_H

/// \file sdf/fileFormatRegistry.h

#include "pxr/pxr.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/weakBase.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(SdfFileFormat);
TF_DECLARE_WEAK_PTRS(PlugPlugin);

/// \class Sdf_FileFormatRegistry
///
/// An object that tracks information about file format plugins in the system,
/// providing methods for finding registered formats either by format
/// identifier or file extension.
///
class Sdf_FileFormatRegistry
{
    Sdf_FileFormatRegistry(const Sdf_FileFormatRegistry&) = delete;
    Sdf_FileFormatRegistry& operator=(const Sdf_FileFormatRegistry&) = delete;
public:
    /// Constructor.
    Sdf_FileFormatRegistry();

    /// Returns the file format described by the \p formatId token.
    SdfFileFormatConstPtr FindById(const TfToken& formatId);

    /// Returns the file format associated with the specified file extension
    /// \p s and target \p target. Extension \p s may be a full file path name,
    /// or an extension with or without a leading dot (e.g. 'foo/bar.usd', 'usd'
    /// or '.usd' are acceptable).
    SdfFileFormatConstPtr FindByExtension(
        const std::string& s,
        const std::string& target = std::string());

    /// Returns a set containing the extension(s) corresponding to 
    /// all registered file formats.
    std::set<std::string> FindAllFileFormatExtensions();

    /// Returns a set containing the extension(s) corresponding to
    /// all registered file formats that derive from \p baseType.
    ///
    /// \p baseType must derive from SdfFileFormat.
    std::set<std::string> FindAllDerivedFileFormatExtensions(
        const TfType& baseType);

    /// Returns the id of the file format plugin that is registered as
    /// the primary format for the given file extension.
    TfToken GetPrimaryFormatForExtension(const std::string& ext);

    /// Returns true if the file format instance that supports the extension
    /// for the supplied \p path and \p target pair supports reading.
    bool FormatSupportsReading(
        const std::string& extension,
        const std::string& target = std::string());

    /// Returns true if the file format instance that supports the extension
    /// for the supplied \p path and \p target pair supports writing.
    bool FormatSupportsWriting(
        const std::string& extension,
        const std::string& target = std::string());

    /// Returns true if the file format instance that supports the extension
    /// for the supplied \p path and \p target pair supports editing.
    bool FormatSupportsEditing(
        const std::string& extension,
        const std::string& target = std::string());

private:
    /// \struct _Info
    ///
    /// Information about a file format plugin. This structure initially holds
    /// the type, a pointer to a plugin that has not yet been loaded, and a
    /// null format ref ptr. After the file format is requested, the plugin is
    /// loaded, and the file format is instantiated.
    ///
    class _Info {
    public:
        /// Enumerates specific Capabilities that can be authored in the 
        /// format's plugInfo.json file.
        enum class Capabilities: uint32_t {
            None        = 0,
            Reading     = 1 << 0,
            Writing     = 1 << 1,
            Editing     = 1 << 2
        };

        _Info(const TfToken& formatId,
              const TfType& type, 
              const TfToken& target, 
              const PlugPluginPtr& plugin,
              Capabilities capabilities)
            : formatId(formatId)
            , type(type)
            , target(target)
            , capabilities(capabilities)
            , _plugin(plugin)
            , _hasFormat(false)
        { }
        
        // Return this _Info's file format
        SdfFileFormatRefPtr GetFileFormat() const;

        const TfToken formatId;
        const TfType type;
        const TfToken target;
        const Capabilities capabilities;

    private:
        const PlugPluginPtr _plugin;
        mutable std::mutex _formatMutex;
        mutable std::atomic<bool> _hasFormat;
        mutable SdfFileFormatRefPtr _format;
    };

    typedef std::shared_ptr<_Info> _InfoSharedPtr;
    typedef std::vector<_InfoSharedPtr> _InfoSharedPtrVector;

    // 1-to-1 mapping from file format Id -> file format info
    typedef TfHashMap<
        TfToken, _InfoSharedPtr, TfToken::HashFunctor> _FormatInfo;

    // many-to-1 mapping from file extension -> file format info for primary
    // format. Each file extension must have one primary file format plugin,
    // but a file format plugin may be the primary one for multiple extensions.
    typedef TfHashMap<
        std::string, _InfoSharedPtr, TfHash> _ExtensionIndex;

    // many-to-many mapping from file extensions -> file format info
    // A file with a given extension may be supported by any number of file
    // formats plugins.
    typedef TfHashMap<
        std::string, _InfoSharedPtrVector, TfHash> _FullExtensionIndex;

    // Populates the _formatInfo structure if it is empty. This causes plugin
    // discovery to run, but does not load any plugins.
    void _RegisterFormatPlugins();

    // Given information about a file format plugin in \p format, load the
    // associated plugin, instantiate the format, cache the instance and
    // return it.
    SdfFileFormatConstPtr _GetFileFormat(const _InfoSharedPtr& format);

    // Gets the format info for the supplied path, target pair
    _InfoSharedPtr _GetFormatInfo(
        const std::string& path,
        const std::string& target);

    // Given a path and target: returns true if the file format supports the
    // given capability.
    bool _FormatSupportsCapability(
        const std::string& extension,
        const std::string& target, 
        _Info::Capabilities capability);

    static _Info::Capabilities _ParseFormatCapabilities(
        const TfType& fileFormatType);

    _FormatInfo _formatInfo;
    _ExtensionIndex _extensionIndex;
    _FullExtensionIndex _fullExtensionIndex;

    std::atomic<bool> _registeredFormatPlugins;
    std::mutex _mutex;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_FILE_FORMAT_REGISTRY_H
