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
#ifndef PXR_EXTRAS_USD_EXAMPLES_USD_RECURSIVE_PAYLOADS_EXAMPLE_FILE_FORMAT_H
#define PXR_EXTRAS_USD_EXAMPLES_USD_RECURSIVE_PAYLOADS_EXAMPLE_FILE_FORMAT_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/pcp/dynamicFileFormatInterface.h"
#include "pxr/base/tf/staticTokens.h"
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

#define USD_RECURSIVE_PAYLOADS_EXAMPLE_FILE_FORMAT_TOKENS \
    ((Id, "usdRecursivePayloadsExample")) \
    ((Version, "1.0")) \
    ((Target, "usd")) \
    ((Extension, "usdrecursivepayloadsexample")) \
    ((Depth, "UsdExample_depth")) \
    ((Num, "UsdExample_num")) \
    ((Radius, "UsdExample_radius")) \
    ((Height, "UsdExample_height")) \
    ((ArgDict, "UsdExample_argDict")) \
    ((PayloadId, "UsdExample_payloadId"))

TF_DECLARE_PUBLIC_TOKENS(UsdRecursivePayloadsExampleFileFormatTokens, 
                         USD_RECURSIVE_PAYLOADS_EXAMPLE_FILE_FORMAT_TOKENS);

TF_DECLARE_WEAK_AND_REF_PTRS(UsdRecursivePayloadsExampleFileFormat);
TF_DECLARE_WEAK_AND_REF_PTRS(SdfLayerBase);

/// \class UsdRecursivePayloadsExampleFileFormat
///
/// This is an example of a dynamic file format plugin that demonstrates one
/// method of generating dynamic content from composed metadata fields in scene
/// description by creating scene description with payloads to the same dynamic
/// file but with different parameters.
/// 
/// This contents of a file of this format are expected to be the same as usda
/// file content. If the file is opened with file format arguments for "num" and
/// "depth" that are greater than 0, then it will generate a ring of Xform prim
/// children that will each have a payload to this file again but with depth-1.
/// It will also adds a "geom" child that references the payload asset file
/// with no parameters, just reading it as an sdf file and referencing the 
/// default prim. Thus we end up with a recursively generated set of prims 
/// containing the contents of the dynamic file.
/// 
/// As an example if you have the following prim defined in a usd file:
///   
///     def Xform "Root" (
///         UsdExample_num = 2 
///         UsdExample_depth = 3 
///         UsdExample_radius = 20.0
///         payload = @cone.usdrecursivepayloadsexample@ 
///     ) {}
/// 
/// It will generate a prim structure that looks something like this:
/// 
/// Root (payload = @cone.usdrecursivepayloadsexample@ 
///    |  UsdExample_num = 2 
///    |  UsdExample_depth = 3 
///    |  UsdExample_radius = 20.0)
///    |
///    | geom (reference = @cone.usdrecursivepayloadsexample@)
///    |
///    | Xform__2_0 (payload = @cone.usdrecursivepayloadsexample@ 
///    |   |         UsdExample_num = 2 
///    |   |         UsdExample_depth = 2 
///    |   |         UsdExample_radius = 10.0)
///    |   |
///    |   | geom (reference = @cone.usdrecursivepayloadsexample@)
///    |   |
///    |   | Xform__1_0 (payload = @cone.usdrecursivepayloadsexample@ 
///    |   |   |         UsdExample_num = 2 
///    |   |   |         UsdExample_depth = 1 
///    |   |   |         UsdExample_radius = 5.0)
///    |   |   |
///    |   |   | geom (reference = @cone.usdrecursivepayloadsexample@)
///    |   |
///    |   | Xform__1_1 (payload = @cone.usdrecursivepayloadsexample@
///    |   |   |         UsdExample_num = 2 
///    |   |   |         UsdExample_depth = 1 
///    |   |   |         UsdExample_radius = 5.0)
///    |   |   |
///    |   |   | geom (reference = @cone.usdrecursivepayloadsexample@)
///    |
///    | Xform__2_1 (payload = @cone.usdrecursivepayloadsexample@
///    |   |         UsdExample_num = 2 
///    |   |         UsdExample_depth = 2 
///    |   |         UsdExample_radius = 10.0)
///    |   |
///    |   | geom (reference = @cone.usdrecursivepayloadsexample@)
///    |   |
///    |   | Xform__1_0 (payload = @cone.usdrecursivepayloadsexample@
///    |   |   |         UsdExample_num = 2 
///    |   |   |         UsdExample_depth = 1 
///    |   |   |         UsdExample_radius = 5.0)
///    |   |   |
///    |   |   | geom (reference = @cone.usdrecursivepayloadsexample@)
///    |   |
///    |   | Xform__1_1 (payload = @cone.usdrecursivepayloadsexample@
///    |   |   |         UsdExample_num = 2 
///    |   |   |         UsdExample_depth = 1 
///    |   |   |         UsdExample_radius = 5.0)
///    |   |   |
///    |   |   | geom (reference = @cone.usdrecursivepayloadsexample@)
/// 
/// Valid metadata field parameters:
///   depth - The number of times to recurse when generating rings of prims
///   num - The number of prims to place in the ring at each level of depth.
///   radius - The radius of the ring around the parent prim which generated 
///            prims are placed.
///   height - Vertical distance above the parent where the ring is placed.
///   argDict - A dictionary of values that can specify the above parameters for
///             specifically IDed payloads.
///
class UsdRecursivePayloadsExampleFileFormat : public SdfFileFormat,
    public PcpDynamicFileFormatInterface
{
public:

    // SdfFileFormat overrides.
    bool CanRead(const std::string &file) const override;
    bool Read(SdfLayer *layer,
              const std::string& resolvedPath,
              bool metadataOnly) const override;

    // We override Write methods so SdfLayer::ExportToString() etc, work. 
    // Writing this layer will write out the generated layer contents.
    bool WriteToString(const SdfLayer& layer,
                       std::string* str,
                       const std::string& comment=std::string()) const override;
    bool WriteToStream(const SdfSpecHandle &spec,
                       std::ostream& out,
                       size_t indent) const override;

    // A required override for generating dynamic arguments that are 
    // particular to this file format.
    void ComposeFieldsForFileFormatArguments(
        const std::string& assetPath, 
        const PcpDynamicFileFormatContext& context,
        FileFormatArguments* args,
        VtValue *dependencyContextData) const override;

    // Another required override for dynamic file arguments to help determine
    // which changes may cause prims using this file format to be invalidated.
    bool CanFieldChangeAffectFileFormatArguments(
        const TfToken& field,
        const VtValue& oldValue,
        const VtValue& newValue,
        const VtValue &dependencyContextData) const override;

protected:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    virtual ~UsdRecursivePayloadsExampleFileFormat();
    UsdRecursivePayloadsExampleFileFormat();
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_EXTRAS_USD_EXAMPLES_USD_RECURSIVE_PAYLOADS_EXAMPLE_FILE_FORMAT_H
