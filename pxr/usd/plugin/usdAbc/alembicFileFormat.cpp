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
#include "pxr/usd/usdAbc/alembicFileFormat.h"

#include "pxr/usd/usdAbc/alembicData.h"
#include "pxr/usd/usd/usdaFileFormat.h"

#include "pxr/usd/sdf/layer.h"

#include "pxr/base/tracelite/trace.h"

#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticData.h"

#include <boost/assign.hpp>
#include <ostream>

using std::string;

TF_DEFINE_PUBLIC_TOKENS(
    UsdAbcAlembicFileFormatTokens, 
    USDABC_ALEMBIC_FILE_FORMAT_TOKENS);

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(UsdAbcAlembicFileFormat, SdfFileFormat);
}


UsdAbcAlembicFileFormat::UsdAbcAlembicFileFormat()
    : SdfFileFormat(
        UsdAbcAlembicFileFormatTokens->Id,
        UsdAbcAlembicFileFormatTokens->Version,
        UsdAbcAlembicFileFormatTokens->Target,
        UsdAbcAlembicFileFormatTokens->Id),
    _usda(SdfFileFormat::FindById(UsdUsdaFileFormatTokens->Id))
{
}

UsdAbcAlembicFileFormat::~UsdAbcAlembicFileFormat()
{
}

SdfAbstractDataRefPtr
UsdAbcAlembicFileFormat::InitData(const FileFormatArguments& args) const
{
    return UsdAbc_AlembicData::New();
}

bool
UsdAbcAlembicFileFormat::CanRead(const string& filePath) const
{
    // XXX: Add more verification of file header magic
    auto extension = TfGetExtension(filePath);
    if (extension.empty()) {
        return false;
    }

    return extension == this->GetFormatId();
}

bool
UsdAbcAlembicFileFormat::Read(
    const SdfLayerBasePtr& layerBase,
    const string& resolvedPath,
    bool metadataOnly) const
{
    TRACE_FUNCTION();

    SdfLayerHandle layer = TfDynamic_cast<SdfLayerHandle>(layerBase);
    if (not TF_VERIFY(layer)) {
        return false;
    }

    SdfAbstractDataRefPtr data = InitData(layerBase->GetFileFormatArguments());
    UsdAbc_AlembicDataRefPtr abcData = TfStatic_cast<UsdAbc_AlembicDataRefPtr>(data);
    if (not abcData->Open(resolvedPath)) {
        return false;
    }

    // Just swap out the data - unlike text layers fully populated into memory,
    // we have no way of tracking change state for undo and inverses.

    // XXX:
    // Do we need to pre- and/or post-process the existing AlembicData object?
    _SwapLayerData(layer, data);

    return true;
}

bool
UsdAbcAlembicFileFormat::WriteToFile(
    const SdfLayerBase* layerBase,
    const std::string& filePath,
    const std::string& comment,
    const FileFormatArguments& args) const
{
    const SdfLayer* layer = dynamic_cast<const SdfLayer*>(layerBase);
    if (not TF_VERIFY(layer)) {
        return false;
    }

    // Write.
    SdfAbstractDataConstPtr data = 
        _GetLayerData(SdfCreateNonConstHandle(layer));
    return TF_VERIFY(data) and UsdAbc_AlembicData::Write(data, filePath, comment);
}

bool 
UsdAbcAlembicFileFormat::ReadFromString(
    const SdfLayerBasePtr& layerBase,
    const std::string& str) const
{
    // XXX: For now, defer to the usda file format for this. May need to
    //      revisit this as the alembic reader gets fully fleshed out.
    return _usda->ReadFromString(layerBase, str);
}

bool 
UsdAbcAlembicFileFormat::WriteToString(
    const SdfLayerBase* layerBase,
    std::string* str,
    const std::string& comment) const
{
    // XXX: For now, defer to the usda file format for this. May need to
    //      revisit this as the alembic reader gets fully fleshed out.
    return _usda->WriteToString(layerBase, str, comment);
}

bool
UsdAbcAlembicFileFormat::WriteToStream(
    const SdfSpecHandle &spec,
    std::ostream& out,
    size_t indent) const
{
    // XXX: Because WriteToString() uses the usda file format and because
    //      a spec will always use it's own file format for writing we'll
    //      get here trying to write an Alembic layer as usda.  So we
    //      turn around call usda.
    return _usda->WriteToStream(spec, out, indent);
}

bool 
UsdAbcAlembicFileFormat::_IsStreamingLayer(
    const SdfLayerBase& layer) const
{
    return true;
}
