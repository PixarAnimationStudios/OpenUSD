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
/// \file Sdf/fileFormat.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/assetPathResolver.h"
#include "pxr/usd/sdf/data.h"
#include "pxr/usd/sdf/fileFormatRegistry.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/layerBase.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

static TfStaticData<Sdf_FileFormatRegistry> _FileFormatRegistry;

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<SdfFileFormat>();
}

TF_DEFINE_PUBLIC_TOKENS(SdfFileFormatTokens, SDF_FILE_FORMAT_TOKENS);

SdfFileFormat::SdfFileFormat(
    const TfToken& formatId,
    const TfToken& versionString,
    const TfToken& target,
    const std::string& extension)
    : _formatId(formatId)
    , _target(target)
    , _cookie("#" + formatId.GetString())
    , _versionString(versionString)
    , _extensions(1, extension)
    , _isPrimaryFormat(
        _FileFormatRegistry
            ->GetPrimaryFormatForExtension(extension) == formatId)
{
    // Do Nothing.
}

SdfFileFormat::SdfFileFormat(
    const TfToken& formatId,
    const TfToken& versionString,
    const TfToken& target,
    const std::vector<std::string>& extensions)
    : _formatId(formatId)
    , _target(target)
    , _cookie("#" + formatId.GetString())
    , _versionString(versionString)
    , _extensions(extensions)

    // If a file format is marked as primary, then it must be the
    // primary format for all of the extensions it supports. So,
    // it's sufficient to just check the first extension in the list.
    , _isPrimaryFormat(
        _FileFormatRegistry
            ->GetPrimaryFormatForExtension(extensions[0]) == formatId)
{
    // Do Nothing.
}

SdfFileFormat::~SdfFileFormat()
{
    // Do Nothing.
}

SdfFileFormat::FileFormatArguments 
SdfFileFormat::GetDefaultFileFormatArguments() const
{
    return FileFormatArguments();
}

SdfAbstractDataRefPtr
SdfFileFormat::InitData(const FileFormatArguments& args) const
{
    SdfData* metadata = new SdfData;

    // The pseudo-root spec must always exist in a layer's SdfData, so
    // add it here.
    metadata->CreateSpec(
        SdfAbstractDataSpecId(&SdfPath::AbsoluteRootPath()), 
        SdfSpecTypePseudoRoot);

    return TfCreateRefPtr(metadata);
}

SdfLayerBaseRefPtr
SdfFileFormat::NewLayer(const SdfFileFormatConstPtr &fileFormat,
                        const std::string &identifier,
                        const std::string &realPath,
                        const ArAssetInfo& assetInfo,
                        const FileFormatArguments &args) const
{
    return TfCreateRefPtr(
        _InstantiateNewLayer(
            fileFormat, identifier, realPath, assetInfo, args));
}

bool
SdfFileFormat::ShouldSkipAnonymousReload() const
{
    return _ShouldSkipAnonymousReload();
}

bool 
SdfFileFormat::IsStreamingLayer(const SdfLayerBase& layer) const
{
    if (layer.GetFileFormat()->GetFormatId() != GetFormatId()) {
        TF_CODING_ERROR(
            "Layer does not use file format '%s'",
            layer.GetFileFormat()->GetFormatId().GetText());
        return true;
    }

    return _IsStreamingLayer(layer);
}

bool
SdfFileFormat::LayersAreFileBased() const
{
    return _LayersAreFileBased();
}

const TfToken&
SdfFileFormat::GetFormatId() const
{
    return _formatId;
}

const TfToken&
SdfFileFormat::GetTarget() const
{
    return _target;
}

const std::string&
SdfFileFormat::GetFileCookie() const
{
    return _cookie;
}

const TfToken&
SdfFileFormat::GetVersionString() const
{
    return _versionString;
}

bool 
SdfFileFormat::IsPrimaryFormatForExtensions() const
{
    return _isPrimaryFormat;
}

const std::vector<std::string>&
SdfFileFormat::GetFileExtensions() const {
    return _extensions;
}

const std::string&
SdfFileFormat::GetPrimaryFileExtension() const
{
    static std::string emptyString;
    return TF_VERIFY(!_extensions.empty()) ? _extensions[0] : emptyString;
}

bool
SdfFileFormat::IsSupportedExtension(
    const std::string& extension) const
{
    std::string ext = GetFileExtension(extension);

    return ext.empty() ?
        false : std::count(_extensions.begin(), _extensions.end(), ext);
}

bool 
SdfFileFormat::IsPackage() const
{
    return false;
}

std::string 
SdfFileFormat::GetPackageRootLayerPath(
    const std::string& resolvedPath) const
{
    return std::string();
}

bool
SdfFileFormat::WriteToFile(
    const SdfLayerBase*,
    const std::string&,
    const std::string&,
    const FileFormatArguments&) const
{
    return false;
}

bool 
SdfFileFormat::ReadFromString(
    const SdfLayerBasePtr& layerBase,
    const std::string& str) const
{
    return false;
}

bool
SdfFileFormat::WriteToStream(
    const SdfSpecHandle &spec,
    std::ostream& out,
    size_t indent) const
{
    return false;
}

bool 
SdfFileFormat::WriteToString(
    const SdfLayerBase* layerBase,
    std::string* str,
    const std::string& comment) const
{
    return false;
}

/* static */
std::string
SdfFileFormat::GetFileExtension(
    const std::string& s)
{
    if (s.empty()) {
        return s;
    }

    // XXX: if it is a dot file (e.g. .sdf) we append a temp
    // name to retain behavior of specifier stripping.
    // this is in place for backwards compatibility
    std::string strippedExtension = (s[0] == '.' ? 
        "temp_file_name" + s : s);

    std::string extension = Sdf_GetExtension(strippedExtension);
       
    return extension.empty() ? s : extension;
}

/* static */
std::set<std::string>
SdfFileFormat::FindAllFileFormatExtensions()
{
    return _FileFormatRegistry->FindAllFileFormatExtensions();
}

/* static */
SdfFileFormatConstPtr
SdfFileFormat::FindById(
    const TfToken& formatId)
{
    return _FileFormatRegistry->FindById(formatId);
}

/* static */
SdfFileFormatConstPtr
SdfFileFormat::FindByExtension(
    const std::string& extension,
    const std::string& target)
{
    return _FileFormatRegistry->FindByExtension(extension, target);
}

bool
SdfFileFormat::_ShouldSkipAnonymousReload() const
{
    return true;
}

bool 
SdfFileFormat::_LayersAreFileBased() const
{
    return true;
}

// Helper to issue an error in case the method template NewLayer fails.
void
SdfFileFormat::_IssueNewLayerFailError(SdfLayerBaseRefPtr const &l,
                                       std::type_info const &type,
                                       std::string const &identifier,
                                       std::string const &realPath) const
{
    TF_CODING_ERROR("NewLayer: expected %s to create a %s, got %s%s instead "
                    "(identifier: %s, realPath: %s)\n",
                    ArchGetDemangled(typeid(*this)).c_str(),
                    ArchGetDemangled(type).c_str(),
                    l ? "a " : "",
                    l ? ArchGetDemangled(TfTypeid(l)).c_str() : "NULL",
                    identifier.c_str(),
                    realPath.c_str());
}

void
SdfFileFormat::_SetLayerData(
    const SdfLayerHandle& layer,
    SdfAbstractDataRefPtr& data)
{
    // If layer initialization has not completed, then this
    // is being loaded as a new layer; otherwise we are loading
    // data into an existing layer.
    //
    // Note that this is an optional::bool and we are checking if it has
    // been set, not what its held value is.
    //
    const bool layerIsLoadingAsNew = !layer->_initializationWasSuccessful;
    if (layerIsLoadingAsNew) {
        layer->_SwapData(data);
    }
    else {
        layer->_SetData(data);
    }
}

SdfAbstractDataConstPtr
SdfFileFormat::_GetLayerData(const SdfLayerHandle& layer)
{
    return layer->_GetData();
}

/* virtual */
SdfLayerBase*
SdfFileFormat::_InstantiateNewLayer(
    const SdfFileFormatConstPtr &fileFormat,
    const std::string &identifier,
    const std::string &realPath,
    const ArAssetInfo& assetInfo,
    const FileFormatArguments &args) const
{
    return new SdfLayer(fileFormat, identifier, realPath, assetInfo, args);
}

PXR_NAMESPACE_CLOSE_SCOPE
