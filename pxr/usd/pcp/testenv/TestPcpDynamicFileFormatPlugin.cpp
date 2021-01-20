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
#include "pxr/pxr.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/textFileFormat.h"
#include "pxr/usd/pcp/dynamicFileFormatContext.h"
#include "pxr/usd/pcp/dynamicFileFormatInterface.h"

#include <string>
#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

using std::string;

static const char * CONTEXT_ID_KEY = "contextId";

#define TEST_PCP_DYNAMIC_FILE_FORMAT_TOKENS       \
    ((Id, "Test_PcpDynamicFileFormat")) \
    ((Version, "1.0")) \
    ((Target, "usd")) \
    ((Extension, "testpcpdynamic")) \
    ((Depth, "TestPcp_depth")) \
    ((Num, "TestPcp_num")) \
    ((Radius, "TestPcp_radius")) \
    ((Height, "TestPcp_height")) \
    ((ArgDict, "TestPcp_argDict")) \
    ((PayloadId, "TestPcp_payloadId"))

TF_DECLARE_PUBLIC_TOKENS(Test_PcpDynamicFileFormatPlugin_FileFormatTokens, 
                         TEST_PCP_DYNAMIC_FILE_FORMAT_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(
    Test_PcpDynamicFileFormatPlugin_FileFormatTokens, 
    TEST_PCP_DYNAMIC_FILE_FORMAT_TOKENS);

/// \class Test_PcpDynamicFileFormatPlugin_FileFormat
///
/// This is an example of a dynamic file format plugin for testing the APIs
/// related to generating dynamic content from composed metadata fields in scene
/// description through payloads.
/// 
/// This contents of a file of this format are expected to be the same as sdf
/// file content. If the file is opened with file format arguments for "num" and
/// "depth" that are greater than 0, then it will generate a ring of Xform prim
/// children that will each have a payload to this file again but with depth-1.
/// It will also adds a "geom" child that references the payload asset file
/// with no parameters, just reading it as an sdf file and referencing the 
/// default prim. Thus we end up with a recursively generated set of prims 
/// containing the contents of the dynamic file.
/// 
/// As an example if you have the following prim defined in an sdf file:
///   
///     def Xform "Root" (
///         payload = @cone.testpcpdyanic@ num=2 depth=3 radius = 20.0) {}
/// 
/// It will generate a prim structure that looks something like this:
/// 
/// Root (payload = @cone.testpcpdyanic@ num=2 depth=3 radius = 20.0)
///    | geom (reference = @cone.testpcpdyanic@)
///    |
///    | Xform__2_0 (payload = @cone.testpcpdyanic@ num=2 depth=2 radius = 10.0)
///    |   | geom (reference = @cone.testpcpdyanic@)
///    |   |
///    |   | Xform__1_0 (payload = @cone.testpcpdyanic@ num=2 depth=1 radius = 5.0)
///    |   |   | geom (reference = @cone.testpcpdyanic@)
///    |   |
///    |   | Xform__1_1 (payload = @cone.testpcpdyanic@ num=2 depth=1 radius = 5.0)
///    |   |   | geom (reference = @cone.testpcpdyanic@)
///    |
///    | Xform__2_1 (payload = @cone.testpcpdyanic@ num=2 depth=2 radius = 10.0)
///    |   | geom (reference = @cone.testpcpdyanic@)
///    |   |
///    |   | Xform__1_0 (payload = @cone.testpcpdyanic@ num=2 depth=1 radius = 5.0)
///    |   |   | geom (reference = @cone.testpcpdyanic@)
///    |   |
///    |   | Xform__1_1 (payload = @cone.testpcpdyanic@ num=2 depth=1 radius = 5.0)
///    |   |   | geom (reference = @cone.testpcpdyanic@)
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
class Test_PcpDynamicFileFormatPlugin_FileFormat : public SdfFileFormat,
    public PcpDynamicFileFormatInterface
{
public:

    // SdfFileFormat overrides.
    bool CanRead(const std::string &file) const override;
    bool Read(SdfLayer *layer,
              const std::string& resolvedPath,
              bool metadataOnly) const override;
    bool ReadFromString(SdfLayer *layer,
                        const std::string& str) const override;

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

    virtual ~Test_PcpDynamicFileFormatPlugin_FileFormat();
    Test_PcpDynamicFileFormatPlugin_FileFormat();

private:
    // Helper cache for testing the functionality that the contextId passed in
    // to ComposeFieldsForFileFormatArguments will match the one passed 
    // into CanFieldChangeAffectFileFormatArguments during change processing 
    // if the change should indeed cause a primIndex to recompose.
    mutable std::unordered_set<uint64_t> _contextIds;
};

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(Test_PcpDynamicFileFormatPlugin_FileFormat, 
                           SdfFileFormat);
}

Test_PcpDynamicFileFormatPlugin_FileFormat
    ::Test_PcpDynamicFileFormatPlugin_FileFormat()
    : SdfFileFormat(
        Test_PcpDynamicFileFormatPlugin_FileFormatTokens->Id,
        Test_PcpDynamicFileFormatPlugin_FileFormatTokens->Version,
        Test_PcpDynamicFileFormatPlugin_FileFormatTokens->Target,
        Test_PcpDynamicFileFormatPlugin_FileFormatTokens->Extension)
{
}

Test_PcpDynamicFileFormatPlugin_FileFormat
    ::~Test_PcpDynamicFileFormatPlugin_FileFormat()
{
}

bool
Test_PcpDynamicFileFormatPlugin_FileFormat::CanRead(
    const string& filePath) const
{
    return true;
}

// Extract a value type from file format arguments if the argument is present
template <class T>
bool
_GetFileFormatArg(SdfFileFormat::FileFormatArguments args, 
                  const TfToken &argName,
                  T* outValue)
{
    auto it = args.find(argName.GetString());
    if (it == args.end()) {
        return false;
    }
    *outValue = TfUnstringify<T>(it->second);
    return true;
}

bool
Test_PcpDynamicFileFormatPlugin_FileFormat::Read(
    SdfLayer *layer,
    const string& resolvedPath,
    bool metadataOnly) const
{
    if (!TF_VERIFY(layer)) {
        return false;
    }

    // We extract the parameters from the layers file format arguments
    FileFormatArguments args = layer->GetFileFormatArguments();

    // The number of transforms to add in a ring for each level of depth. 
    int num = 1;
    _GetFileFormatArg(
        args, Test_PcpDynamicFileFormatPlugin_FileFormatTokens->Num, &num);

    // The number of times to recurse adding "num" transforms at each level
    int depth = 0;
    _GetFileFormatArg(
        args, Test_PcpDynamicFileFormatPlugin_FileFormatTokens->Depth, &depth);

    // Payload ID string. This is not a generated argument but it can be added
    // in to the asset path in scene description as a way of distinguishing 
    // payloads from each other if a prim references multiple payloads and you 
    // want set different parameters for each.
    std::string payloadId;
    _GetFileFormatArg(
        args, Test_PcpDynamicFileFormatPlugin_FileFormatTokens->PayloadId,
        &payloadId);

    // At depth 0, we're done recursing. just read in the contents of our file 
    // as an sdf text file format into the layer. 
    if (depth <= 0) {
        const SdfFileFormatConstPtr fileFormat = 
            SdfFileFormat::FindById(SdfTextFileFormatTokens->Id);
        return fileFormat->Read(layer, resolvedPath, metadataOnly);
    }

    // Otherwise, here we generate new file content.
    SdfLayerRefPtr genLayer = SdfLayer::CreateAnonymous(".sdf");
    SdfChangeBlock block;

    // Create a "Root" Xform prim at the root of the genLayer. 
    SdfPrimSpecHandle rootSpec = SdfPrimSpec::New(
        SdfLayerHandle(genLayer), std::string("Root"), SdfSpecifierDef, "Xform");
    // Make Root the generated layer's default prim. This is so that our 
    // recursively generated payloads below can reference in generated layers.
    genLayer->SetDefaultPrim(rootSpec->GetNameToken());

    // Add a "geom" reference to this layer. References don't generate dynamic 
    // file format arguments so the original contents of the layer will be 
    // referenced if this layer has a default prim specified.
    SdfPrimSpecHandle geomSpec = SdfPrimSpec::New(
        rootSpec, std::string("geom"), SdfSpecifierDef);
    geomSpec->GetReferenceList().Add(SdfReference(resolvedPath, SdfPath()));

    // Generate the ring of dynamic prims
    if (depth > 1) {
        // Radius is how far from the parent Root prim, newly generated Xform 
        // prims are placed.
        double radius = 3.0;
        _GetFileFormatArg(
            args, Test_PcpDynamicFileFormatPlugin_FileFormatTokens->Radius, 
            &radius);

        // Height is how high the new set of Xform prims are placed above its
        // Root prim.
        double height = 3.0;
        _GetFileFormatArg(
            args, Test_PcpDynamicFileFormatPlugin_FileFormatTokens->Height, 
            &height);

        // Add child Xform prims in a ring around the Root prim. 
        // The arguments:
        //   num - How many Xform prims to place around the ring
        //   radius - The radius of the ring.
        //   hieght - how high the ring is placed above the root prim.
        for (int i = 0; i < num; ++i) {
            // Create Xform spec as child of "Root"
            std::string specName = 
                TfStringPrintf("Xform_%s_%d_%d", payloadId.c_str(), depth - 1, i);
            SdfPrimSpecHandle xformSpec = SdfPrimSpec::New(
                rootSpec, specName, SdfSpecifierDef, "Xform");

            // Place Xform at its spot in the ring.
            double angle = M_PI * 2.0 * i / num;
            VtValue val(GfVec3d(radius * cos(angle), radius * sin(angle), height));
            SdfAttributeSpecHandle attrSpec = SdfAttributeSpec::New(
                xformSpec, "xformOp:translate", SdfGetValueTypeNameForValue(val));
            attrSpec->SetDefaultValue(val);
            VtTokenArray orderVal({TfToken("xformOp:translate")});
            SdfAttributeSpecHandle orderAttrSpec = SdfAttributeSpec::New(
                xformSpec, "xformOpOrder", SdfValueTypeNames->TokenArray);
            orderAttrSpec->SetDefaultValue(VtValue(orderVal));

            // Recurse by adding a payload to this same layer asset path but 
            // with updated metadata for generating the contents.

            // Pass through the same values of num and height for the payload.
            xformSpec->SetInfo(
                Test_PcpDynamicFileFormatPlugin_FileFormatTokens->Num, 
                VtValue(num));
            xformSpec->SetInfo(
                Test_PcpDynamicFileFormatPlugin_FileFormatTokens->Height, 
                VtValue(height));
            // Decrease depth by one. This is the most important as it stops
            // the recursion.
            xformSpec->SetInfo(
                Test_PcpDynamicFileFormatPlugin_FileFormatTokens->Depth, 
                VtValue(depth - 1 ));
            // Halve the radius
            xformSpec->SetInfo(
                Test_PcpDynamicFileFormatPlugin_FileFormatTokens->Radius, 
                VtValue(radius/ 2.0));
            // Add the payload to this asset and use the default prim. This 
            // will generate a new layer since the file format arguments will
            // be different.
            if (payloadId.empty()) {
                xformSpec->GetPayloadList().Add(SdfPayload(resolvedPath, SdfPath()));
            } else {
                FileFormatArguments newArgs;
                newArgs[Test_PcpDynamicFileFormatPlugin_FileFormatTokens->PayloadId] = payloadId;
                xformSpec->GetPayloadList().Add(SdfPayload(
                    SdfLayer::CreateIdentifier(resolvedPath, newArgs), 
                    SdfPath()));
            }
        }
    }

    layer->TransferContent(genLayer);

    return true;
}

bool 
Test_PcpDynamicFileFormatPlugin_FileFormat::ReadFromString(
    SdfLayer *layer,
    const std::string& str) const
{
    return true;
}


bool 
Test_PcpDynamicFileFormatPlugin_FileFormat::WriteToString(
    const SdfLayer& layer,
    std::string* str,
    const std::string& comment) const
{
    // Write the contents as an sdf text file.
    return SdfFileFormat::FindById(
        SdfTextFileFormatTokens->Id)->WriteToString(layer, str, comment);
}

bool
Test_PcpDynamicFileFormatPlugin_FileFormat::WriteToStream(
    const SdfSpecHandle &spec,
    std::ostream& out,
    size_t indent) const
{
    // Write the contents as an sdf text file.
    return SdfFileFormat::FindById(
        SdfTextFileFormatTokens->Id)->WriteToStream(spec, out, indent);
}

// Helper for extracting a value by name from composed metadata or already
// computed argument dictionary
template <class T>
static bool _ExtractArg(const TfToken &argName,
                        const PcpDynamicFileFormatContext& context,
                        const VtDictionary &argDict,
                        T* outValue)
{
    // Value in argDict takes precedent if it exists.
    if (VtDictionaryIsHolding<T>(argDict, argName)) {
        *outValue = VtDictionaryGet<T>(argDict, argName);
        return true;
    }

    // Otherwise compose the value from the prim field context.
    VtValue val;
    if (!context.ComposeValue(argName, &val) || val.IsEmpty()) {
        return false;
    }

    if (!val.IsHolding<T>()) {
        TF_CODING_ERROR(
            "Expected '%s' value to hold an %s, got '%s'", 
            argName.GetText(), 
            TfType::Find<T>().GetTypeName().c_str(),
            TfStringify(val).c_str());
        return false;
    } 
    *outValue = val.UncheckedGet<T>();
    return true;
}

static bool
_ExtractPayloadId(const std::string& assetPath, std::string *payloadId)
{
    // Extract the current file format arguments from the asset path so we can
    // look for a specified "payloadId"
    std::string layerPath;
    SdfFileFormat::FileFormatArguments args;
    SdfLayer::SplitIdentifier(assetPath, &layerPath, &args);

    const auto &it = args.find(
        Test_PcpDynamicFileFormatPlugin_FileFormatTokens->PayloadId.GetString());

    // Find the subdictionary in argDict for asset's payloadId.
    if (it != args.end()) {
        *payloadId = it->second;
        return true;
    }

    return false;
}

// Helper for composing the "argDict" metadata field as a dictionary value from
// the prim field context and extracting the correct subdictionary for the
// particular asset.
static void _ExtractArgDict(
    const PcpDynamicFileFormatContext& context,
    const std::string& assetPath,
    VtDictionary* dict)
{
    // Compose the "argDict" metadata from the prim field context.
    VtValue value;
    if (!context.ComposeValue(
            Test_PcpDynamicFileFormatPlugin_FileFormatTokens->ArgDict, &value)) {
        return;
    }
    if (!value.IsHolding<VtDictionary>()) {
        return;
    }
    const VtDictionary &argDict = value.UncheckedGet<VtDictionary>();

    // Extract the current file format arguments from the asset path so we can
    // look for a specified "payloadId"
    std::string layerPath;
    SdfFileFormat::FileFormatArguments args;
    SdfLayer::SplitIdentifier(assetPath, &layerPath, &args);

    const auto &it = args.find(
        Test_PcpDynamicFileFormatPlugin_FileFormatTokens->PayloadId.GetString());

    // Find the subdictionary in argDict for asset's payloadId.
    if (it != args.end()) {
        const std::string &payloadId = it->second;
        if (VtDictionaryIsHolding<VtDictionary>(argDict, payloadId)) {
            *dict = VtDictionaryGet<VtDictionary>(argDict, payloadId);
        }
    }
}

void 
Test_PcpDynamicFileFormatPlugin_FileFormat::ComposeFieldsForFileFormatArguments(
    const std::string& assetPath, 
    const PcpDynamicFileFormatContext& context,
    FileFormatArguments* args,
    VtValue *dependencyContextData) const
{
    // Our dependencyContextData will contain a dictionary
    VtDictionary customDependencyData;

    // Create an ID for the context of this function call so we can test the
    // dependency checking in CanFieldChangeAffectFileFormatArguments. For this
    // test format, we'll just get the current tick time as the ID value. We 
    // store a set of all context Ids used during the test to make this easy.
    uint64_t contextId = ArchGetTickTime();
    _contextIds.insert(contextId);
    customDependencyData[CONTEXT_ID_KEY] = contextId;

    // First get the argument dictionary for the asset as it may override
    // values of the other metadata fields. We only extract an argDict if the
    // asset has a payloadId and if it does, we'll store the payloadId in the
    // dependency data.
    std::string payloadId;
    VtDictionary argDict;
    if (_ExtractPayloadId(assetPath, &payloadId)) {
        _ExtractArgDict(context, assetPath, &argDict);
        customDependencyData[
            Test_PcpDynamicFileFormatPlugin_FileFormatTokens->PayloadId] = 
                payloadId;
    }

    // Put our dictionary in the dependency data.
    dependencyContextData->Swap(customDependencyData);

    // Compose the depth and num metadata and add them to the file format 
    // arguments. We bail if either are nonpositive.
    int depth = 0;
    if (_ExtractArg(Test_PcpDynamicFileFormatPlugin_FileFormatTokens->Depth, 
                    context, argDict, &depth)) {
        if (depth < 1) {
            return;
        }
    }
    int num = 0;
    if (_ExtractArg(Test_PcpDynamicFileFormatPlugin_FileFormatTokens->Num, 
                    context, argDict, &num)) {
        if (num < 1) {
            return;
        }
    }
    (*args)[Test_PcpDynamicFileFormatPlugin_FileFormatTokens->Depth] = 
        TfStringify(depth);
    (*args)[Test_PcpDynamicFileFormatPlugin_FileFormatTokens->Num] = 
        TfStringify(num);

    // Compose the radius and height metadata and add them as well.
    double radius = 10.0;
    if (_ExtractArg(Test_PcpDynamicFileFormatPlugin_FileFormatTokens->Radius, 
                    context, argDict, &radius)) {
        (*args)[Test_PcpDynamicFileFormatPlugin_FileFormatTokens->Radius] = 
            TfStringify(radius);
    }
    double height = 0.0;
    if (_ExtractArg(Test_PcpDynamicFileFormatPlugin_FileFormatTokens->Height, 
                    context, argDict, &height)) {
        (*args)[Test_PcpDynamicFileFormatPlugin_FileFormatTokens->Height] = 
            TfStringify(height);
    }
}

bool 
Test_PcpDynamicFileFormatPlugin_FileFormat::CanFieldChangeAffectFileFormatArguments(
    const TfToken& field,
    const VtValue& oldValue,
    const VtValue& newValue,
    const VtValue &dependencyContextData) const
{
    // Our implementation of ComposeFieldsForFileFormatArguments sets a 
    // VtDictionary in the dependencyContextData. That better be what we get
    // back here.
    if (!TF_VERIFY(dependencyContextData.IsHolding<VtDictionary>())) {
        return false;
    }
    const VtDictionary &depDataDict = 
        dependencyContextData.UncheckedGet<VtDictionary>();

    // Return false if the contextId stored in the dependency has never been 
    // generated by ComposeFieldForFileFormatArguments. Our unit test will use 
    // this to verify that Pcp is sending the generated dependency data back to
    // this function.
    uint64_t contextId = VtDictionaryGet<uint64_t>(depDataDict, CONTEXT_ID_KEY);
    if (_contextIds.find(contextId) == _contextIds.end()) {
        return false;
    }

    // For this test example, argDict only applies to assets that have a
    // payloadId in its file arguments. Reject the argDict changes if the file 
    // arguments do not.
    if (field == Test_PcpDynamicFileFormatPlugin_FileFormatTokens->ArgDict) {
        if (depDataDict.count(
                Test_PcpDynamicFileFormatPlugin_FileFormatTokens->PayloadId) == 0) {
            return false;
        }
    }

    if (oldValue == newValue) {
        return false;
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE



