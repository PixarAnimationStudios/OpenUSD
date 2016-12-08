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
#include "fileFormat.h"

#include "pxr/usd/usd/usdaFileFormat.h"
#include "pxr/usd/sdf/layer.h"

#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/registryManager.h"

#include "stream.h"
#include "streamIO.h"
#include "translator.h"

#include <fstream>
#include <string>

using std::string;

TF_DEFINE_PUBLIC_TOKENS(
    UsdObjFileFormatTokens, 
    USDOBJ_FILE_FORMAT_TOKENS);

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(UsdObjFileFormat, SdfFileFormat);
}

UsdObjFileFormat::UsdObjFileFormat()
    : SdfFileFormat(
        UsdObjFileFormatTokens->Id,
        UsdObjFileFormatTokens->Version,
        UsdObjFileFormatTokens->Target,
        UsdObjFileFormatTokens->Id)
{
}

UsdObjFileFormat::~UsdObjFileFormat()
{
}

bool
UsdObjFileFormat::CanRead(const string& filePath) const
{
    // Could check to see if it looks like valid obj data...
    return true;
}

bool
UsdObjFileFormat::_ReadFromStream(
    const SdfLayerBasePtr &layerBase,
    std::istream &input,
    bool metadataOnly,
    string *outErr) const
{
    SdfLayerHandle layer = TfDynamic_cast<SdfLayerHandle>(layerBase);
    if (not TF_VERIFY(layer)) {
        return false;
    }

    // Read Obj data stream.
    UsdObjStream objStream;
    if (not UsdObjReadDataFromStream(input, &objStream, outErr))
        return false;

    // Translate obj to usd schema.
    SdfLayerRefPtr objAsUsd = UsdObjTranslateObjToUsd(objStream);
    if (not objAsUsd)
        return false;
    
    // Move generated content into final layer.
    layer->TransferContent(objAsUsd);
    return true;
}

bool
UsdObjFileFormat::Read(
    const SdfLayerBasePtr& layerBase,
    const string& resolvedPath,
    bool metadataOnly) const
{
    // try and open the file
    std::ifstream fin(resolvedPath.c_str());
    if (!fin.is_open()) {
        TF_RUNTIME_ERROR("Failed to open file \"%s\"", resolvedPath.c_str());
        return false;
    }

    string error;
    if (not _ReadFromStream(layerBase, fin, metadataOnly, &error)) {
        TF_RUNTIME_ERROR("Failed to read OBJ from file \"%s\": %s",
                         resolvedPath.c_str(), error.c_str());
        return false;
    }
    return true;
}

bool 
UsdObjFileFormat::ReadFromString(
    const SdfLayerBasePtr& layerBase,
    const std::string& str) const
{
    string error;
    std::stringstream ss(str);
    if (not _ReadFromStream(layerBase, ss, /*metadataOnly=*/false, &error)) {
        TF_RUNTIME_ERROR("Failed to read OBJ data from string: %s",
                         error.c_str());
        return false;
    }
    return true;
}


bool 
UsdObjFileFormat::WriteToString(
    const SdfLayerBase* layerBase,
    std::string* str,
    const std::string& comment) const
{
    // XXX: For now, defer to the usda file format for this.  We don't support
    // writing Usd content as a OBJ.
    return SdfFileFormat::FindById(
        UsdUsdaFileFormatTokens->Id)->WriteToString(layerBase, str, comment);
}

bool
UsdObjFileFormat::WriteToStream(
    const SdfSpecHandle &spec,
    std::ostream& out,
    size_t indent) const
{
    // XXX: For now, defer to the usda file format for this.  We don't support
    // writing Usd content as a OBJ.
    return SdfFileFormat::FindById(
        UsdUsdaFileFormatTokens->Id)->WriteToStream(spec, out, indent);
}
