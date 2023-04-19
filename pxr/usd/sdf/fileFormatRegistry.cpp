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
/// \file Sdf/fileFormatRegistry.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/fileFormatRegistry.h"
#include "pxr/usd/sdf/debugCodes.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/scopeDescription.h"
#include "pxr/base/tf/staticTokens.h"

#include <algorithm>
#include <type_traits>

using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(_PlugInfoKeyTokens,
    ((FormatId,   "formatId"))
    ((Extensions, "extensions"))
    ((Target,     "target"))
    ((Primary,    "primary"))
    ((SupportsReading, "supportsReading"))
    ((SupportsWriting, "supportsWriting"))
    ((SupportsEditing, "supportsEditing"))
    );

// Locale-independent tolower only for ascii/utf-8 A-Z.
static inline std::string
_ToLower(std::string const &str)
{
    std::string lowered;
    lowered.resize(str.size());
    std::transform(str.begin(), str.end(), lowered.begin(),
                   [](char ch) {
                       return ('A' <= ch && ch <= 'Z') ? ch - 'A' + 'a' : ch;
                   });
    return lowered;
}

// Searches plugin meta data to determine if a capability is supported for the
// file format type.  All capabilities are enabled by default (for backwards
// compatability) so the user must explicitly opt out in plugInfo.json
static bool 
_FormatCapabilityIsSupported(
    PlugRegistry& reg,
    TfType formatType,
    const string& key)
{
    JsValue capability = reg.GetDataFromPluginMetaData(formatType, key);

    if (capability.IsBool() && !capability.GetBool()) {
        return false;
    }

    return true;
}


SdfFileFormatRefPtr
Sdf_FileFormatRegistry::_Info::GetFileFormat() const
{
    if (_hasFormat)
        return _format;

    if (_plugin)
        _plugin->Load();

    SdfFileFormatRefPtr newFormat;
    if (Sdf_FileFormatFactoryBase* factory =
            type.GetFactory<Sdf_FileFormatFactoryBase>()) {
        newFormat = factory->New();
    }
    
    if (newFormat) {
        std::lock_guard<std::mutex> lock(_formatMutex);
        if (!_hasFormat) {
            _format = newFormat;
            _hasFormat = true;
        }
    }

    return _format;
}

Sdf_FileFormatRegistry::Sdf_FileFormatRegistry()
    : _registeredFormatPlugins(false)
{
    // Do Nothing.
}

SdfFileFormatConstPtr
Sdf_FileFormatRegistry::FindById(
    const TfToken& formatId)
{
    TRACE_FUNCTION();

    if (formatId.IsEmpty()) {
        TF_CODING_ERROR("Cannot find file format for empty id");
        return TfNullPtr;
    }

    _RegisterFormatPlugins();
    _FormatInfo::iterator it = _formatInfo.find(formatId);
    if (it != _formatInfo.end())
        return _GetFileFormat(it->second);

    return TfNullPtr;
}

Sdf_FileFormatRegistry::_InfoSharedPtr 
Sdf_FileFormatRegistry::_GetFormatInfo(
    const string& s,
    const string& target)
{
    _InfoSharedPtr formatInfo;

    if (s.empty()) {
        TF_CODING_ERROR("Cannot find file format for empty string");
        return formatInfo;
    }

    // Convert to lowercase for lookup.
    string ext = _ToLower(SdfFileFormat::GetFileExtension(s));
    if (ext.empty()) {
        TF_CODING_ERROR("Unable to determine extension for '%s'", s.c_str());
        return formatInfo;
    }

    _RegisterFormatPlugins();

    if (target.empty()) {
        _ExtensionIndex::const_iterator it = _extensionIndex.find(ext);
        if (it != _extensionIndex.end())
            formatInfo = it->second;
    }
    else {
        _FullExtensionIndex::const_iterator it = _fullExtensionIndex.find(ext);
        if (it != _fullExtensionIndex.end()) {
            TF_FOR_ALL(infoIt, it->second) {
                if ((*infoIt)->target == target) {
                    formatInfo = (*infoIt);
                    break;
                }
            }
        }
    }

    return formatInfo;
}

SdfFileFormatConstPtr
Sdf_FileFormatRegistry::FindByExtension(
    const string& s,
    const string& target)
{
    TRACE_FUNCTION();

    _InfoSharedPtr formatInfo = _GetFormatInfo(s, target);

    return formatInfo ? _GetFileFormat(formatInfo) : TfNullPtr;
}

std::set<std::string>
Sdf_FileFormatRegistry::FindAllFileFormatExtensions()
{
    _RegisterFormatPlugins();

    std::set<std::string> result; 
    for (const auto& p : _extensionIndex) {
        result.insert(p.first);
    }

    return result;
}

std::set<std::string>
Sdf_FileFormatRegistry::FindAllDerivedFileFormatExtensions(
    const TfType& baseType)
{
    TRACE_FUNCTION();

    _RegisterFormatPlugins();

    if (!baseType.IsA<SdfFileFormat>()) {
        TF_CODING_ERROR("Type %s does not derive from SdfFileFormat",
                        baseType.GetTypeName().c_str());
        return {};
    }

    std::set<std::string> result; 
    for (const auto& p : _fullExtensionIndex) {
        for (const _InfoSharedPtr &info : p.second) {
            if (info->type.IsA(baseType)) {
                result.insert(p.first);
            }
        }
    }

    return result;
}

TfToken
Sdf_FileFormatRegistry::GetPrimaryFormatForExtension(
    const std::string& ext)
{
    _RegisterFormatPlugins();

    // Convert to lowercase for lookup.
    _ExtensionIndex::const_iterator it = _extensionIndex.find(_ToLower(ext));
    if (it != _extensionIndex.end()) {
        return it->second->formatId;
    }
    
    return TfToken();
}

Sdf_FileFormatRegistry::_Info::Capabilities 
Sdf_FileFormatRegistry::_ParseFormatCapabilities(
    const TfType& formatType)
{
    using UT = std::underlying_type<_Info::Capabilities>::type;
    using Capabilities = _Info::Capabilities;

    PlugRegistry& reg = PlugRegistry::GetInstance();

    _Info::Capabilities capabilities = Capabilities::None;
    if (_FormatCapabilityIsSupported( 
        reg, formatType, _PlugInfoKeyTokens->SupportsReading)) {
        capabilities = static_cast<Capabilities>(
            static_cast<UT>(capabilities) | 
            static_cast<UT>(Capabilities::Reading));
    }

    if (_FormatCapabilityIsSupported( 
        reg, formatType, _PlugInfoKeyTokens->SupportsWriting)) {
        capabilities = static_cast<Capabilities>(
            static_cast<UT>(capabilities) | 
            static_cast<UT>(Capabilities::Writing));
    }

    if (_FormatCapabilityIsSupported( 
        reg, formatType, _PlugInfoKeyTokens->SupportsEditing)) {
        capabilities = static_cast<Capabilities>(
            static_cast<UT>(capabilities) | 
            static_cast<UT>(Capabilities::Editing));
    }

    return capabilities;
}

void
Sdf_FileFormatRegistry::_RegisterFormatPlugins()
{
    if (_registeredFormatPlugins)
        return;

    _FormatInfo formatInfo;
    _ExtensionIndex extensionIndex;
    _FullExtensionIndex fullExtensionIndex;

    TRACE_FUNCTION();

    TF_DEBUG(SDF_FILE_FORMAT).Msg("Sdf_FileFormatRegistry::_RegisterFormatPlugins");

    PlugRegistry& reg = PlugRegistry::GetInstance();

    TF_DESCRIBE_SCOPE("Registering file format plugins");

    std::set<TfType> formatTypes;
    TfType formatBaseType = TfType::Find<SdfFileFormat>();
    if (TF_VERIFY(!formatBaseType.IsUnknown()))
        PlugRegistry::GetAllDerivedTypes(formatBaseType, &formatTypes);

    for (auto formatType : formatTypes) {

        TF_DEBUG(SDF_FILE_FORMAT).Msg("_RegisterFormatPlugins: "
            "Type '%s'\n", formatType.GetTypeName().c_str());

        PlugPluginPtr plugin = reg.GetPluginForType(formatType);
        if (!plugin)
            continue;

        TF_DEBUG(SDF_FILE_FORMAT).Msg("_RegisterFormatPlugins: "
            "  plugin '%s'\n", plugin->GetName().c_str());

        JsValue aFormatId = reg.GetDataFromPluginMetaData(
            formatType, _PlugInfoKeyTokens->FormatId);
        if (aFormatId.IsNull()) {
            TF_DEBUG(SDF_FILE_FORMAT).Msg("_RegisterFormatPlugins: "
                "No format identifier for type '%s', skipping.",
                formatType.GetTypeName().c_str());
            continue;
        }

        if (!aFormatId.IsString()) {
            TF_CODING_ERROR("Unexpected value type for key '%s' "
                "in plugin meta data for file format type '%s'",
                _PlugInfoKeyTokens->FormatId.GetText(),
                formatType.GetTypeName().c_str());
            continue;
        }

        string formatId = aFormatId.GetString();
        if (formatId.empty()) {
            TF_CODING_ERROR("File format '%s' plugin meta data '%s' is empty",
                formatType.GetTypeName().c_str(),
                _PlugInfoKeyTokens->FormatId.GetText());
            continue;
        }

        TF_DEBUG(SDF_FILE_FORMAT).Msg("_RegisterFormatPlugins: "
            "  formatId '%s'\n", formatId.c_str());

        JsValue aExtensions = reg.GetDataFromPluginMetaData(
            formatType, _PlugInfoKeyTokens->Extensions);
        if (aExtensions.IsNull()) {
            TF_DEBUG(SDF_FILE_FORMAT).Msg("_RegisterFormatPlugins: "
                "No extensions registered for type '%s', skipping.",
                formatType.GetTypeName().c_str());
            continue;
        }

        if (!aExtensions.IsArrayOf<string>()) {
            TF_CODING_ERROR("Unexpected value type for key '%s' "
                "in plugin meta data for file format type '%s'",
                _PlugInfoKeyTokens->Extensions.GetText(),
                formatType.GetTypeName().c_str());
            continue;
        }

        vector<string> extensions = aExtensions.GetArrayOf<string>();
        if (extensions.empty()) {
            TF_CODING_ERROR("File format '%s' plugin meta data '%s' is empty",
                formatType.GetTypeName().c_str(),
                _PlugInfoKeyTokens->Extensions.GetText());
            continue;
        }

        // Convert 'extensions' to be all lower-case.
        std::transform(extensions.begin(), extensions.end(),
                       extensions.begin(),
                       [](std::string const &ext) { return _ToLower(ext); });

        // The 'target' entry does not need to be specified in every
        // file format's plugin info. If it is not, then the value will be
        // inherited from the file format's base class.
        JsValue aTarget;
        {
            std::vector<TfType> typeHierarchy;
            formatType.GetAllAncestorTypes(&typeHierarchy);
            TF_FOR_ALL(type, typeHierarchy) {
                aTarget = reg.GetDataFromPluginMetaData(
                    *type, _PlugInfoKeyTokens->Target);
                if (!aTarget.IsNull()) {
                    TF_DEBUG(SDF_FILE_FORMAT).Msg("_RegisterFormatPlugins: "
                        "    Found target for type '%s' from type '%s'\n",
                        formatType.GetTypeName().c_str(),
                        type->GetTypeName().c_str());
                    break;
                }
            }
        }

        if (aTarget.IsNull()) {
            TF_DEBUG(SDF_FILE_FORMAT).Msg("_RegisterFormatPlugins: "
                "No target for type '%s', skipping.\n",
                formatType.GetTypeName().c_str());
            continue;
        }

        if (!aTarget.IsString()) {
            TF_CODING_ERROR("Unexpected value type for key '%s' "
                "in plugin meta data for file format type '%s'",
                _PlugInfoKeyTokens->Target.GetText(),
                formatType.GetTypeName().c_str());
            continue;
        }

        const string target = aTarget.GetString();
        if (target.empty()) {
            TF_CODING_ERROR("File format '%s' plugin meta data '%s' is empty",
                formatType.GetTypeName().c_str(),
                _PlugInfoKeyTokens->Target.GetText());
            continue;
        }

        _Info::Capabilities capabilities = _ParseFormatCapabilities(formatType);

        TF_DEBUG(SDF_FILE_FORMAT).Msg("_RegisterFormatPlugins: "
            "  target '%s'\n", target.c_str());

        const TfToken formatIdToken(formatId);

        _InfoSharedPtr& info = formatInfo[formatIdToken];
        if (info) {
            TF_CODING_ERROR("Duplicate registration for file format '%s'",
                formatId.c_str());
            continue;
        }
        info = std::make_shared<_Info>(
            formatIdToken, formatType, TfToken(target), plugin, capabilities);

        // Record the extensions that this file format plugin can handle.
        // Note that an extension may be supported by multiple file format
        // plugins.
        for (auto ext : extensions) {
            if (ext.empty())
                continue;

            if (ext[0] == '.')
                ext = ext.substr(1);

            TF_DEBUG(SDF_FILE_FORMAT).Msg("_RegisterFormatPlugins: "
                "  extension '%s'\n", ext.c_str());

            bool foundRegisteredInfoWithSameTarget = false;

            _InfoSharedPtrVector& infosForExt = fullExtensionIndex[ext];
            TF_FOR_ALL(infoIt, infosForExt) {
                if ((*infoIt)->target == info->target) {
                    foundRegisteredInfoWithSameTarget = true;
                    break;
                }
            }

            if (foundRegisteredInfoWithSameTarget) {
                TF_CODING_ERROR(
                    "Multiple file formats with target '%s' "
                    "registered for extension '%s', skipping.",
                    info->target.GetText(), ext.c_str());
            }
            else {
                infosForExt.push_back(info);
            }
        }
    }

    // Determine the 'primary' file format plugin for each extension.
    // This is the plugin that will be used for a given extension if no
    // target is specified.
    std::set<std::string> errorExtensions;

    TF_FOR_ALL(extIt, fullExtensionIndex) {
        const std::string& ext = extIt->first;
        const _InfoSharedPtrVector& infos = extIt->second;
        TF_VERIFY(!infos.empty());

        _InfoSharedPtr primaryFormatInfo;
        if (infos.size() == 1) {
            primaryFormatInfo = infos.front();
        }
        else {
            TF_FOR_ALL(infoIt, infos) {
                const JsValue aPrimary = reg.GetDataFromPluginMetaData(
                    (*infoIt)->type, _PlugInfoKeyTokens->Primary);
                if (aPrimary.IsNull()) {
                    continue;
                }

                if (!aPrimary.IsBool()) {
                    TF_CODING_ERROR("Unexpected value type for key '%s' "
                        "in plugin meta data for file format type '%s'",
                        _PlugInfoKeyTokens->Primary.GetText(),
                        (*infoIt)->type.GetTypeName().c_str());
                    continue;
                }
                const bool isPrimary = aPrimary.GetBool();

                if (isPrimary) {
                    if (!primaryFormatInfo) {
                        primaryFormatInfo = *infoIt;
                        // Note we do not break out of this for loop after
                        // finding the primary format; allow the loop to
                        // continue so we flag the error case where an
                        // extension has multiple primary formats.
                    }
                    else {
                        primaryFormatInfo = _InfoSharedPtr();
                        if (errorExtensions.insert(ext).second) {
                            TF_CODING_ERROR(
                                "Multiple primary file formats specified for "
                                "extension '%s', skipping.", ext.c_str());
                        }
                        break;
                    }
                }
            }

            if (!primaryFormatInfo && errorExtensions.insert(ext).second) {
                TF_CODING_ERROR(
                    "No primary file format specified for extension '%s', "
                    "skipping.", ext.c_str());
            }
        }

        if (primaryFormatInfo) {
            extensionIndex[ext] = primaryFormatInfo;
        }
    }

    // Now take the lock and see if we're the thread that gets to set the real
    // state.  Another thread may have beaten us to it.
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_registeredFormatPlugins) {
        // Publish.
        TF_VERIFY(_formatInfo.empty());
        TF_VERIFY(_extensionIndex.empty());
        TF_VERIFY(_fullExtensionIndex.empty());

        _formatInfo.swap(formatInfo);
        _extensionIndex.swap(extensionIndex);
        _fullExtensionIndex.swap(fullExtensionIndex);

        _registeredFormatPlugins = true;
    }

}

SdfFileFormatConstPtr
Sdf_FileFormatRegistry::_GetFileFormat(
    const _InfoSharedPtr& info)
{
    if (!TF_VERIFY(info))
        return TfNullPtr;

    return info->GetFileFormat();
}

bool 
Sdf_FileFormatRegistry::FormatSupportsReading(
    const std::string& extension,
    const std::string& target)
{
    return _FormatSupportsCapability(extension, target, 
        _Info::Capabilities::Reading);
}

bool 
Sdf_FileFormatRegistry::FormatSupportsWriting(
    const std::string& extension,
    const std::string& target)
{
    return _FormatSupportsCapability(extension, target, 
        _Info::Capabilities::Writing);
}

bool 
Sdf_FileFormatRegistry::FormatSupportsEditing(
    const std::string& extension,
    const std::string& target)
{
    return _FormatSupportsCapability(extension, target, 
        _Info::Capabilities::Editing);
}

bool 
Sdf_FileFormatRegistry::_FormatSupportsCapability(
    const std::string& path,
    const std::string& target,
    _Info::Capabilities capabilityFlag)
{
    using UT = std::underlying_type<_Info::Capabilities>::type;
    _InfoSharedPtr formatInfo = _GetFormatInfo(path, target);
    
    if (!formatInfo) {
        return false;
    }

    return static_cast<UT>(formatInfo->capabilities) & 
        static_cast<UT>(capabilityFlag);
}

PXR_NAMESPACE_CLOSE_SCOPE
