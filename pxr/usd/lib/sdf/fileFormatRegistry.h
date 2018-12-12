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
#ifndef SDF_FILE_FORMAT_REGISTRY_H
#define SDF_FILE_FORMAT_REGISTRY_H

/// \file sdf/fileFormatRegistry.h

#include "pxr/pxr.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/weakBase.h"
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <atomic>
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
class Sdf_FileFormatRegistry : boost::noncopyable
{
public:
    /// Constructor.
    Sdf_FileFormatRegistry();

    /// Returns the file format described by the \p formatId token.
    SdfFileFormatConstPtr FindById(const TfToken& formatId);

    /// Returns the file format associated with the specified file extension
    /// \p s and target \p target. Extension \p s may be passed with or 
    /// without a leading dot (e.g. either 'menva' or '.menva' are acceptable).
    SdfFileFormatConstPtr FindByExtension(
        const std::string& s,
        const std::string& target = std::string());

    /// Returns a set containing the extension(s) corresponding to 
    /// all registered file formats.
    std::set<std::string> FindAllFileFormatExtensions();

    /// Returns the id of the file format plugin that is registered as
    /// the primary format for the given file extension.
    TfToken GetPrimaryFormatForExtension(const std::string& ext);

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
        _Info(const TfToken& formatId,
              const TfType& type, 
              const TfToken& target, 
              const PlugPluginPtr& plugin)
            : formatId(formatId)
            , type(type)
            , target(target)
            , _plugin(plugin)
            , _hasFormat(false)
        { }
        
        // Return this _Info's file format
        SdfFileFormatRefPtr GetFileFormat() const;

        const TfToken formatId;
        const TfType type;
        const TfToken target;

    private:
        const PlugPluginPtr _plugin;
        mutable std::mutex _formatMutex;
        mutable std::atomic<bool> _hasFormat;
        mutable SdfFileFormatRefPtr _format;
    };

    typedef boost::shared_ptr<_Info> _InfoSharedPtr;
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

    _FormatInfo _formatInfo;
    _ExtensionIndex _extensionIndex;
    _FullExtensionIndex _fullExtensionIndex;

    std::atomic<bool> _registeredFormatPlugins;
    std::mutex _mutex;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_FILE_FORMAT_REGISTRY_H
