//
// Copyright 2018 Pixar
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
#include "pxr/pxr.h"
#include "pxr/usd/plugin/usdMtlx/fileFormat.h"
#include "pxr/usd/plugin/usdMtlx/reader.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/usdaFileFormat.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/trace/trace.h"

#include <MaterialXCore/Document.h>
#include <MaterialXFormat/XmlIo.h>

namespace mx = MaterialX;

PXR_NAMESPACE_OPEN_SCOPE

namespace {

template <typename R>
static
bool
_Read(UsdStagePtr stage, R&& reader)
{
    try {
        auto doc = mx::createDocument();
        reader(doc);
        UsdMtlxRead(doc, stage);
        return true;
    }
    catch (mx::ExceptionFoundCycle& x) {
        TF_RUNTIME_ERROR("MaterialX cycle found: %s\n", x.what());
        return false;
    }
    catch (mx::Exception& x) {
        TF_RUNTIME_ERROR("MaterialX error: %s\n", x.what());
        return false;
    }
}

} // anonymous namespace

TF_DEFINE_PUBLIC_TOKENS(
    UsdMtlxFileFormatTokens, 
    USDMTLX_FILE_FORMAT_TOKENS);

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(UsdMtlxFileFormat, SdfFileFormat);
}


UsdMtlxFileFormat::UsdMtlxFileFormat()
    : SdfFileFormat(
        UsdMtlxFileFormatTokens->Id,
        UsdMtlxFileFormatTokens->Version,
        UsdMtlxFileFormatTokens->Target,
        UsdMtlxFileFormatTokens->Id)
{
}

UsdMtlxFileFormat::~UsdMtlxFileFormat()
{
}

SdfAbstractDataRefPtr
UsdMtlxFileFormat::InitData(const FileFormatArguments& args) const
{
    return SdfFileFormat::InitData(args);
}

bool
UsdMtlxFileFormat::CanRead(const std::string& filePath) const
{
    // XXX -- MaterialX doesn't provide this function.  We should attempt
    //        to parse XML as far as finding the first 'materialx' node.

    // XXX -- Emergency backup test.  This should be removed when the
    //        proper test described above is implemented because the
    //        actual filename extension shouldn't matter.
    const auto extension = TfGetExtension(filePath);
    if (extension != this->GetFormatId()) {
        return false;
    }

    return true;
}

bool
UsdMtlxFileFormat::Read(
    SdfLayer* layer,
    const std::string& resolvedPath,
    bool metadataOnly) const
{
    TRACE_FUNCTION();

    auto stage = UsdStage::CreateInMemory();
    if (!_Read(stage,
               [&resolvedPath](mx::DocumentPtr d) {
                    mx::readFromXmlFile(d, resolvedPath);
               })) {
        return false;
    }

    layer->TransferContent(stage->GetRootLayer());
    return true;
}

bool
UsdMtlxFileFormat::WriteToFile(
    const SdfLayer& layer,
    const std::string& filePath,
    const std::string& comment,
    const FileFormatArguments& args) const
{
    return false;
}

bool 
UsdMtlxFileFormat::ReadFromString(
    SdfLayer* layer,
    const std::string& str) const
{
    TRACE_FUNCTION();

    auto stage = UsdStage::CreateInMemory();
    if (!_Read(stage,
               [&str](mx::DocumentPtr d) {
                    mx::readFromXmlString(d, str);
               })) {
        return false;
    }

    layer->TransferContent(stage->GetRootLayer());
    return true;
}

bool 
UsdMtlxFileFormat::WriteToString(
    const SdfLayer& layer,
    std::string* str,
    const std::string& comment) const
{
    return SdfFileFormat::FindById(UsdUsdaFileFormatTokens->Id)->
        WriteToString(layer, str, comment);
}

bool
UsdMtlxFileFormat::WriteToStream(
    const SdfSpecHandle &spec,
    std::ostream& out,
    size_t indent) const
{
    return SdfFileFormat::FindById(UsdUsdaFileFormatTokens->Id)->
        WriteToStream(spec, out, indent);
}

PXR_NAMESPACE_CLOSE_SCOPE
