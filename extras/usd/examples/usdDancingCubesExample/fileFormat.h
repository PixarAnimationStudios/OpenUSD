//
// Copyright 2019 Pixar
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
#ifndef PXR_EXTRAS_USD_EXAMPLES_USD_DANCING_CUBES_EXAMPLE_FILE_FORMAT_H
#define PXR_EXTRAS_USD_EXAMPLES_USD_DANCING_CUBES_EXAMPLE_FILE_FORMAT_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/pcp/dynamicFileFormatInterface.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define USD_DANCING_CUBES_EXAMPLE_FILE_FORMAT_TOKENS       \
    ((Id, "usdDancingCubesExample"))                  \
    ((Version, "1.0"))                  \
    ((Target, "usd"))                  \
    ((Extension, "usddancingcubesexample")) \
    ((Params, "Usd_DCE_Params")) 

TF_DECLARE_PUBLIC_TOKENS(UsdDancingCubesExampleFileFormatTokens, 
                         USD_DANCING_CUBES_EXAMPLE_FILE_FORMAT_TOKENS);

TF_DECLARE_WEAK_AND_REF_PTRS(UsdDancingCubesExampleFileFormat);

/// \class UsdDancingCubesExampleFileFormat
///
/// Example of a dynamic file format plugin implementation that is entirely 
/// procedurally generated from the layer's file format arguments.
/// 
/// It employs its own custom SdfAbstractData subclass, 
/// UsdDancingCubesExample_Data, which is used to generate multiple animated 
/// prims entirely from a small set of parameters. Since layers of this file 
/// format have their contents solely generated from the file format arguments, 
/// they do not need to read or parse the contents of the file at all. The file
/// format arguments provided by the layer's identifier are converted to a 
/// params object defined with UsdDancingCubesExample_Data which the layer's
/// data uses to generate its specs when requested. See data.h and dataImpl.h 
/// more details on the params and how the data generates its content.
///
/// This being a PcpDynamicFileFormatInterface subclass, if a file of this 
/// format is a payload for a prim spec, the file format arguments can be 
/// generated through the composed fields on the prim. There is a single custom 
/// metadata field defined in plugInfo.json called "Usd_DCE_Params" that can be
/// used to customize the payload's file format arguments. This field needs to 
/// hold a dictionary value which can provide overrides to the individual param 
/// value arguments defined in UsdDancingCubesExample_Data.
///
class UsdDancingCubesExampleFileFormat : public SdfFileFormat,
    public PcpDynamicFileFormatInterface
{
public:
    /// Override this function from SdfFileFormat to provide our own procedural
    /// SdfAbstractData class.
    SdfAbstractDataRefPtr InitData(
        const FileFormatArguments &args) const override;

    /// Must implement SdfFileFormat's CanRead function. Returns true for all 
    /// files as the contents of the file aren't used.
    bool CanRead(const std::string &filePath) const override;

    /// Must implement SdfFileFormat's Read function, though this implemenation
    /// doesn't do anything. There is nothing from the file that needs to be
    /// read as data will have already been initialized from file format 
    /// arguments.
    bool Read(SdfLayer *layer,
              const std::string &resolvedPath,
              bool metadataOnly) const override;

    /// We override WriteToString and WriteToStream methods so 
    /// SdfLayer::ExportToString() etc, work. Writing this layer will write out
    /// the generated layer contents.
    /// We do NOT implement WriteToFile as it doesn't make sense to write to
    /// files of this format when the contents are completely generated from the
    /// file format arguments.
    bool WriteToString(const SdfLayer & layer,
                       std::string *str,
                       const std::string &comment=std::string()) const override;
    bool WriteToStream(const SdfSpecHandle &spec,
                       std::ostream& out,
                       size_t indent) const override;

    /// A required PcpDynamicFileFormatInterface override for generating 
    /// the file format arguments in context.
    void ComposeFieldsForFileFormatArguments(
        const std::string &assetPath, 
        const PcpDynamicFileFormatContext &context,
        FileFormatArguments *args,
        VtValue *contextDependencyData) const override;

    /// A required PcpDynamicFileFormatInterface override for processing whether
    /// a field change may affect the file format arguments within a given
    /// context.
    bool CanFieldChangeAffectFileFormatArguments(
        const TfToken &field,
        const VtValue &oldValue,
        const VtValue &newValue,
        const VtValue &contextDependencyData) const override;

protected:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    virtual ~UsdDancingCubesExampleFileFormat();
    UsdDancingCubesExampleFileFormat();
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_EXTRAS_USD_EXAMPLES_USD_DANCING_CUBES_EXAMPLE_FILE_FORMAT_H
