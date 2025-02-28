//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/arch/fileSystem.h"
#include "pxr/usd/sdr/filesystemDiscovery.h"
#include "pxr/usd/sdr/filesystemDiscoveryHelpers.h"

#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/envSetting.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

SDR_REGISTER_DISCOVERY_PLUGIN(_SdrFilesystemDiscoveryPlugin)

TF_DEFINE_ENV_SETTING(
    PXR_SDR_FS_PLUGIN_SEARCH_PATHS, "",
    "The paths that should be searched, recursively, for files that represent "
    "shader nodes. Paths should be separated by either a ':' or a ';' "
    "depending on your platform (it should mimic the PATH attribute).  See "
    "ARCH_PATH_LIST_SEP");

TF_DEFINE_ENV_SETTING(
    PXR_SDR_FS_PLUGIN_ALLOWED_EXTS, "",
    "The extensions on files that define nodes.  Do not include the leading "
    "'.'. Extensions should be separated by a colon.");

TF_DEFINE_ENV_SETTING(
    PXR_SDR_FS_PLUGIN_FOLLOW_SYMLINKS, false,
    "Whether symlinks should be followed while walking the search paths. Set "
    "to 'true' (case sensitive) if they should be followed.");

_SdrFilesystemDiscoveryPlugin::_SdrFilesystemDiscoveryPlugin()
{
    //
    // TODO: This needs to somehow be set up to find the nodes that USD
    //       ships with
    //
    _searchPaths = TfStringSplit(
            TfGetEnvSetting(PXR_SDR_FS_PLUGIN_SEARCH_PATHS),
            ARCH_PATH_LIST_SEP);
    _allowedExtensions = TfStringSplit(
            TfGetEnvSetting(PXR_SDR_FS_PLUGIN_ALLOWED_EXTS), ":");
    _followSymlinks = TfGetEnvSetting(PXR_SDR_FS_PLUGIN_FOLLOW_SYMLINKS);

    SdrStringVec legacySearchPaths = TfStringSplit(
        TfGetEnvSetting(PXR_NDR_FS_PLUGIN_SEARCH_PATHS),
        ARCH_PATH_LIST_SEP);
    _searchPaths.insert(_searchPaths.end(), legacySearchPaths.begin(),
                        legacySearchPaths.end());
    SdrStringVec legacyExtensions = TfStringSplit(
        TfGetEnvSetting(PXR_NDR_FS_PLUGIN_ALLOWED_EXTS), ":");
    _allowedExtensions.insert(_allowedExtensions.end(),
                              legacyExtensions.begin(),
                              legacyExtensions.end());
    _followSymlinks |= TfGetEnvSetting(PXR_NDR_FS_PLUGIN_FOLLOW_SYMLINKS);
}

_SdrFilesystemDiscoveryPlugin::_SdrFilesystemDiscoveryPlugin(Filter filter)
    : _SdrFilesystemDiscoveryPlugin()
{
    _filter = std::move(filter);
}

SdrShaderNodeDiscoveryResultVec
_SdrFilesystemDiscoveryPlugin::DiscoverShaderNodes(const Context& context)
{
    auto result = SdrFsHelpersDiscoverShaderNodes(
        _searchPaths, _allowedExtensions, _followSymlinks, &context
    );

    if (_filter) {
        auto j = result.begin();
        for (auto i = j; i != result.end(); ++i) {
            // If we pass the filter and any previous haven't then move.
            if (_filter(*i)) {
                if (j != i) {
                    *j = std::move(*i);
                }
                ++j;
            }
        }
        result.erase(j, result.end());
    }

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
