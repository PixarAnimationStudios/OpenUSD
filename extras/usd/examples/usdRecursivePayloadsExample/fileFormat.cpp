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
#include "pxr/pxr.h"
#include "fileFormat.h"

#include "pxr/usd/pcp/dynamicFileFormatContext.h"
#include "pxr/usd/usd/usdaFileFormat.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/usd/sdf/primSpec.h"

#include <fstream>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

using std::string;

TF_DEFINE_PUBLIC_TOKENS(
    UsdRecursivePayloadsExampleFileFormatTokens, 
    USD_RECURSIVE_PAYLOADS_EXAMPLE_FILE_FORMAT_TOKENS);

TF_DEFINE_PRIVATE_TOKENS(
    _propertyNameTokens,
    ((xformOpOrder, "xformOpOrder"))
    ((xformOpTranslate, "xformOp:translate"))
);

TF_DEFINE_PRIVATE_TOKENS(
    _primNameTokens,
    ((Root, "Root"))
    ((Geom, "geom"))
    ((Xform, "Xform"))
);

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(UsdRecursivePayloadsExampleFileFormat, SdfFileFormat);
}

namespace {

// Struct that provides the relevant parameters for the dynamically generated
// layer content. This helps provide a consistent method for extracting 
// parameter values from prim fields and file format arguments as well as 
// converting them back to file format arguments.
struct _Params{
    int depth {0};
    int num {3};
    double radius {10.0};
    double height {0.0};

    // Extracts the param values from any extractor object that provides a 
    // templated Extract function for retrieve any of the param value fields
    // by name token. This is to enforce that we use the same logic when 
    // computing the file format arguments from dynamic file format context
    // as when extracting from the file format arguments when reading the layer.
    template <typename ValueExtractor>
    void
    ExtractValues(const ValueExtractor &extractor)
    {
        // First extract the recursion depth. At depth 0, we don't generate
        // any dynamic content so we don't need to extract any of the other
        // parameters. This is especially helpful when extracting from a 
        // PcpDynamicFileFormatContext since it means we can avoid introducing
        // change dependencies on the fields we don't compute if we early out 
        // here.
        extractor.Extract(
            UsdRecursivePayloadsExampleFileFormatTokens->Depth, &depth);
        // Treat negative values as zero so we have consistent parameter values
        // when there's no recursive layer content to generate.
        depth = std::max(0, depth);
        if (depth < 1) {
            return;
        }

        // Extract The number of transforms to add in a ring for each level of 
        // depth. We clamp this value to be positive so there's always at least
        // one prim generated at each depth.
        extractor.Extract(
            UsdRecursivePayloadsExampleFileFormatTokens->Num, &num);
        num = std::max(1, num);

        // Radius and height are only needed when depth is 2 or higher as they
        // are used to layout the recursively generated prims and depth 1 just
        // generates the geom prim.
        if (depth > 1) {
            extractor.Extract(
                UsdRecursivePayloadsExampleFileFormatTokens->Radius, &radius);
            extractor.Extract(
                UsdRecursivePayloadsExampleFileFormatTokens->Height, &height);

        }
    }

    // Converts these parameters into file format arguments in a way that is
    // consistent with how ExtractValues populates the parameter values. 
    SdfFileFormat::FileFormatArguments ToArgs() const
    {
        SdfFileFormat::FileFormatArguments args;

        // For depth 0, don't return any file format arguments. The layer will
        // be read as a normal usda file.
        if (depth < 1) {
            return args;
        }

        args[UsdRecursivePayloadsExampleFileFormatTokens->Depth] = 
            TfStringify(depth);
        args[UsdRecursivePayloadsExampleFileFormatTokens->Num] = 
            TfStringify(num);

        // Radius and height only apply to depth > 1 so only write them in 
        // that case. This gives consistent identities to dynamic layers of
        // depth 1 regardless of the radius and height metadata values.
        if (depth > 1) {
            args[UsdRecursivePayloadsExampleFileFormatTokens->Radius] = 
                TfStringify(radius);
            args[UsdRecursivePayloadsExampleFileFormatTokens->Height] = 
                TfStringify(height);
        }
        return args;
    }
};

// Params extractor for from file format arguments.
class FromArgsValueExtractor 
{
public:
    FromArgsValueExtractor(const SdfFileFormat::FileFormatArguments &args) :
        _args(args) {}

    template <class T>
    bool Extract(const TfToken &argName, T *outValue) const
    {
        // Find the named arg string value in the map if its there.
        auto it = _args.find(argName.GetString());
        if (it == _args.end()) {
            return false;
        }
        // Try to convert the string value to the actual output value type.
        T extractVal;
        bool success = true;
        extractVal = TfUnstringify<T>(it->second, &success);
        if (!success) {
            TF_CODING_ERROR(
                "Could not convert arg string '%s' to value of type %s", 
                argName.GetText(), 
                TfType::Find<T>().GetTypeName().c_str());
            return false;
        }
        *outValue = extractVal;
        return true;
    }

private:
    const SdfFileFormat::FileFormatArguments &_args;
};

// Params extractor from the pcp context for ComposeFieldsForFileFormatArguments
class FromContextValueExtractor 
{
public:
    FromContextValueExtractor(const PcpDynamicFileFormatContext& context,
                              const VtDictionary &argDict) :
        _context(context), _argDict(argDict) {}

    template <class T>
    bool Extract(const TfToken &argName, T *outValue) const
    {
        // Value in argDict takes precedent if it exists. This has the added
        // effect that if the value comes from argDict, we don't compose the
        // value of the scalared value field at all, thus preventing 
        // change dependencies on the field when the value is overridden anyway.
        if (VtDictionaryIsHolding<T>(_argDict, argName)) {
            *outValue = VtDictionaryGet<T>(_argDict, argName);
            return true;
        }

        // Otherwise compose the value from the prim field context.
        VtValue val;
        if (!_context.ComposeValue(argName, &val) || val.IsEmpty()) {
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

private:
    const PcpDynamicFileFormatContext& _context;
    const VtDictionary &_argDict;
};

};

UsdRecursivePayloadsExampleFileFormat::UsdRecursivePayloadsExampleFileFormat()
    : SdfFileFormat(
        UsdRecursivePayloadsExampleFileFormatTokens->Id,
        UsdRecursivePayloadsExampleFileFormatTokens->Version,
        UsdRecursivePayloadsExampleFileFormatTokens->Target,
        UsdRecursivePayloadsExampleFileFormatTokens->Extension)
{
}

UsdRecursivePayloadsExampleFileFormat::~UsdRecursivePayloadsExampleFileFormat()
{
}

bool
UsdRecursivePayloadsExampleFileFormat::CanRead(const string& filePath) const
{
    return true;
}

// Creates a new prim spec that contains a payload to the same layer but
// and a different set of parameter fields that will dynamically generate
// the layer's content again when the payload is processed.
static SdfPrimSpecHandle 
_CreateRecursiveChildSpec(const std::string &payloadId,
                          const std::string &layerPath,
                          int childIndex,
                          const _Params &childParams,
                          const SdfPrimSpecHandle &rootSpec)
{
    // Generate this prim spec's name. with the recursion depth of the new
    // prim and its generated child index.
    std::string specName = 
        TfStringPrintf("%s_%s_%d_%d", _primNameTokens->Xform.GetText(),
                       payloadId.c_str(), childParams.depth, childIndex);
    // Create the Xform type spec as child of the root spec.
    SdfPrimSpecHandle xformSpec = SdfPrimSpec::New(
        rootSpec, specName, SdfSpecifierDef, _primNameTokens->Xform);

    // Recurse by adding a payload to this same layer asset path for 
    // the prim but with updated child prim param fields for generating
    // the contents. The layer contents for the 

    // Set all the child parameter fields. These will provide a new set of 
    // parameters for generating the payload's file format arguments
    xformSpec->SetField(
        UsdRecursivePayloadsExampleFileFormatTokens->Num, 
        childParams.num);
    xformSpec->SetField(
        UsdRecursivePayloadsExampleFileFormatTokens->Height, 
        childParams.height);
    xformSpec->SetField(
        UsdRecursivePayloadsExampleFileFormatTokens->Depth, 
        childParams.depth);
    xformSpec->SetField(
        UsdRecursivePayloadsExampleFileFormatTokens->Radius, 
        childParams.radius);

    // Lastly add the payload of the same layer asset. This provides the 
    // recursion as this payload will generate dynamic file format arguments
    // using the field values we just set and will thus generate its own child
    // prim specs when read.
    if (payloadId.empty()) {
        xformSpec->GetPayloadList().Add(SdfPayload(layerPath));
    } else {
        // If the original file path provided a payload ID in its file format
        // arguments, make sure we reinclude it in the path we provide to our
        // payload. It keeps consistency in the names of the recursively 
        // generated child prims.
        SdfFileFormat::FileFormatArguments newArgs;
        newArgs[UsdRecursivePayloadsExampleFileFormatTokens->PayloadId] = payloadId;
        xformSpec->GetPayloadList().Add(SdfPayload(
            SdfLayer::CreateIdentifier(layerPath, newArgs)));
    }

    return xformSpec;
}

// Generates a new dynamic layer for given file format arguments if the 
// arguments allow. Returns a null layer if the arguments are not valid
// for a dynamic layer.
static SdfLayerRefPtr 
_GenerateDynamicLayer(const SdfFileFormat::FileFormatArguments &args,
                      const string& layerPath)
{
    // Extract the layer parameters from the file format arguments.
    _Params params;
    params.ExtractValues(FromArgsValueExtractor(args));

    // At recursion depth 0, we're done recursing and we don't generate the 
    // layer content again.
    if (params.depth < 1) {
        return SdfLayerRefPtr();
    }

    // Payload ID string. This is not a generated argument but it can be added
    // in to the asset path in scene description as a way of distinguishing 
    // payloads from each other if a prim references multiple payloads and you 
    // want set different parameters for each. We include this in the generated
    // prim specs if present.
    std::string payloadId;
    TfMapLookup(args, UsdRecursivePayloadsExampleFileFormatTokens->PayloadId, 
                &payloadId);

    // Create the new anonymous layer.
    SdfLayerRefPtr genLayer = SdfLayer::CreateAnonymous(".usd");
    SdfChangeBlock block;

    // Create a "Root" Xform prim at the root of the generated layer. 
    SdfPrimSpecHandle rootSpec = SdfPrimSpec::New(
        SdfLayerHandle(genLayer), _primNameTokens->Root, 
        SdfSpecifierDef, _primNameTokens->Xform);
    // Make Root the layer's default prim. This is so that our recursively 
    // generated payloads below can reference in generated layers more simply.
    genLayer->SetDefaultPrim(rootSpec->GetNameToken());

    // Add a "geom" reference to this layer. References don't generate dynamic 
    // file format arguments and we don't provide them in this layer path, so 
    // the original contents of the layer will be referenced if this layer has 
    // a default prim specified.
    SdfPrimSpecHandle geomSpec = SdfPrimSpec::New(
        rootSpec, _primNameTokens->Geom, SdfSpecifierDef);
    geomSpec->GetReferenceList().Add(SdfReference(layerPath, SdfPath()));

    // At depth 1 we only create the geometry prim that references the layer.
    // We only generate the xforms that include dynamic payloads at recursion
    // depth 2 or higher.
    if (params.depth > 1) {
        // Get params that will be used to populate the child prim metadata.
        // We use the same parameters except we reduce the recursion depth by
        // one and half the radius. The depth change is particularly necessary
        // to make sure we stop recursing eventually as we continue to add
        // child payloads.
        _Params childParams = params;
        childParams.depth -= 1;
        childParams.radius *= 0.5;

        // Distance vector for helping create the position around the center 
        // prim.
        const GfVec3d distVec(params.radius, params.radius, params.height);

        // Add child Xform prims in a ring around the Root prim. 
        // The arguments:
        //   num - How many Xform prims to place around the ring
        //   radius - The radius of the ring.
        //   hieght - how high the ring is placed above the root prim.
        for (int i = 0; i < params.num; ++i) {
            // Create the child Xform spec with its recursive payload and 
            // paremeter fields.
            SdfPrimSpecHandle xformSpec = _CreateRecursiveChildSpec(
                payloadId, layerPath, i, childParams, rootSpec);

            // Place new Xform spec at its spot in the ring by computing the 
            // angle vector and comp-wise multiplying in the distance vector.
            double angle = M_PI * 2.0 * i / params.num;
            GfVec3d angleVec(1.0);
            GfSinCos(angle, &angleVec[1], &angleVec[0]);
            VtValue posVal(GfCompMult(distVec, angleVec));
            // Create and set the translation attribute spec.
            SdfAttributeSpecHandle attrSpec = SdfAttributeSpec::New(
                xformSpec, _propertyNameTokens->xformOpTranslate, 
                SdfGetValueTypeNameForValue(posVal));
            attrSpec->SetDefaultValue(posVal);

            // The order attribute spec is necessary and is always the same 
            // for all these prims.
            static const VtValue orderVal(
                VtTokenArray({_propertyNameTokens->xformOpTranslate}));
            SdfAttributeSpecHandle orderAttrSpec = SdfAttributeSpec::New(
                xformSpec, _propertyNameTokens->xformOpOrder, 
                SdfGetValueTypeNameForValue(orderVal));
            orderAttrSpec->SetDefaultValue(orderVal);
        }
    }

    return genLayer;
}

bool
UsdRecursivePayloadsExampleFileFormat::Read(
    SdfLayer *layer,
    const string& resolvedPath,
    bool metadataOnly) const
{
    if (!TF_VERIFY(layer)) {
        return false;
    }

    FileFormatArguments args;
    std::string layerPath;
    SdfLayer::SplitIdentifier(layer->GetIdentifier(), &layerPath, &args);

    // First try to generate a new dynamic layer from the given file format
    // arguments. If dynamic layer is generated, we'll give its content to our
    // layer.
    SdfLayerRefPtr genLayer = _GenerateDynamicLayer(args, layerPath);
    if (genLayer) {
        layer->TransferContent(genLayer);
    } else {
        // If we didn't generate a dynamic layer, we're done recursing and
        // can just read in the contents the actual file as a usda file into 
        // the layer. 
        const SdfFileFormatConstPtr fileFormat = 
            SdfFileFormat::FindById(UsdUsdaFileFormatTokens->Id);
        return fileFormat->Read(layer, resolvedPath, metadataOnly);
    }

    return true;
}

bool 
UsdRecursivePayloadsExampleFileFormat::WriteToString(
    const SdfLayer& layer,
    std::string* str,
    const std::string& comment) const
{
    // Write the generated contents in usda text format.
    return SdfFileFormat::FindById(
        UsdUsdaFileFormatTokens->Id)->WriteToString(layer, str, comment);
}

bool
UsdRecursivePayloadsExampleFileFormat::WriteToStream(
    const SdfSpecHandle &spec,
    std::ostream& out,
    size_t indent) const
{
    // Write the generated contents in usda text format.
    return SdfFileFormat::FindById(
        UsdUsdaFileFormatTokens->Id)->WriteToStream(spec, out, indent);
}

// Extracts a string valued payload ID from the file format arguments of the 
// the given asset path if there is one present.
static bool
_ExtractPayloadId(const std::string& assetPath, std::string *payloadId)
{
    // Split the file format arguments from the asset path so we can
    // look for a specified "payloadId"
    std::string layerPath;
    SdfFileFormat::FileFormatArguments args;
    SdfLayer::SplitIdentifier(assetPath, &layerPath, &args);

    // Find the payloadId in the args if its there and output it.
    const auto &it = args.find(
        UsdRecursivePayloadsExampleFileFormatTokens->PayloadId.GetString());
    if (it != args.end()) {
        *payloadId = it->second;
        return true;
    }

    return false;
}

// Helper for composing the "argDict" metadata field as a dictionary value from
// the prim field context and extracting the correct subdictionary for the
// given payload ID.
static void _ExtractArgDict(
    const PcpDynamicFileFormatContext& context,
    const std::string& payloadId,
    VtDictionary* dict)
{
    // Compose the "argDict" metadata from the prim field context.
    VtValue value;
    if (!context.ComposeValue(
            UsdRecursivePayloadsExampleFileFormatTokens->ArgDict, &value)) {
        return;
    }
    if (!value.IsHolding<VtDictionary>()) {
        return;
    }
    const VtDictionary &argDict = value.UncheckedGet<VtDictionary>();

    if (VtDictionaryIsHolding<VtDictionary>(argDict, payloadId)) {
        *dict = VtDictionaryGet<VtDictionary>(argDict, payloadId);
    }
}

void 
UsdRecursivePayloadsExampleFileFormat::ComposeFieldsForFileFormatArguments(
    const std::string& assetPath, 
    const PcpDynamicFileFormatContext& context,
    FileFormatArguments* args,
    VtValue *dependencyContextData) const
{
    // dependencyContextData is used to provide arbitrary data about how this
    // composes field from the context, so that this information can be 
    // provided to CanFieldChangeAffectFileFormatArguments during change 
    // processing when a field value changes.
    // 
    // Our dependencyContextData will contain a dictionary
    VtDictionary customDependencyData;

    // First get the argument dictionary for the asset as it may override
    // values of the other metadata fields. We only extract an argDict if the
    // asset has a payloadId
    std::string payloadId;
    VtDictionary argDict;
    if (_ExtractPayloadId(assetPath, &payloadId)) {
        _ExtractArgDict(context, payloadId, &argDict);
        // Store the payloadId in the dependency data for the change processing.
        customDependencyData[
            UsdRecursivePayloadsExampleFileFormatTokens->PayloadId] = 
                payloadId;
    }

    // Create a new params object and populate it with values extracted from
    // the context and argument override dictionary and then convert them into
    // the file format arguments.
    _Params params;
    params.ExtractValues(FromContextValueExtractor(context, argDict));
    FileFormatArguments paramArgs = params.ToArgs();
    args->swap(paramArgs);

    // Put our dictionary in the dependency data.
    dependencyContextData->Swap(customDependencyData);
}

bool 
UsdRecursivePayloadsExampleFileFormat::CanFieldChangeAffectFileFormatArguments(
    const TfToken& field,
    const VtValue& oldValue,
    const VtValue& newValue,
    const VtValue &dependencyContextData) const
{
    // In this example, argDict only applies to assets that have a payloadId 
    // in its file arguments. This is a demonstration of how we can use
    // dependencyContextData to fine tune what is considered to be a change
    // that affects the file format arguments; it is by no means a
    // comprehensive example.
    if (field == UsdRecursivePayloadsExampleFileFormatTokens->ArgDict) {
        // Our implementation of ComposeFieldsForFileFormatArguments sets a 
        // VtDictionary in the dependencyContextData. That better be what we get
        // back here.
        if (!TF_VERIFY(dependencyContextData.IsHolding<VtDictionary>())) {
            return false;
        }
        const VtDictionary &depDataDict = 
            dependencyContextData.UncheckedGet<VtDictionary>();

        // argDict changes will not affect arguments if the dependency data 
        // doesn't include a payloadId.
        auto it = depDataDict.find(
            UsdRecursivePayloadsExampleFileFormatTokens->PayloadId);
        if (it == depDataDict.end() || !it->second.IsHolding<std::string>()) {
            return false;
        }

        // We have a payloadId.
        const std::string &payloadId = it->second.UncheckedGet<std::string>();

        // The field values should be dictionaries if they exist.
        const VtDictionary &oldDict = oldValue.IsHolding<VtDictionary>() ?
            oldValue.UncheckedGet<VtDictionary>() : VtGetEmptyDictionary();
        const VtDictionary &newDict = newValue.IsHolding<VtDictionary>() ?
            newValue.UncheckedGet<VtDictionary>() : VtGetEmptyDictionary();

        // Find the old and new subdictionary values for the payloadId and do
        // a simple check if the value for the payloadId is different at all.
        auto oldIt = oldDict.find(payloadId);
        auto newIt = newDict.find(payloadId);
        const bool oldValExists = oldIt == oldDict.end();
        const bool newValExists = newIt == newDict.end();

        if (oldValExists != newValExists) {
            return true;
        }
        if (newValExists && oldIt->second != newIt->second) {
            return true;
        }
    } else if (field == UsdRecursivePayloadsExampleFileFormatTokens->Depth) {
        // Another simple example of an extra check we can do with the depth
        // field. Since we clamp depth to be non-negative, if the value changes
        // from one non-positive value to another, it's not going to affect 
        // the arguments.
        if (oldValue.IsHolding<int>() && 
            newValue.IsHolding<int>() &&
            oldValue.UncheckedGet<int>() < 1 && 
            newValue.UncheckedGet<int>() < 1) {
            return false;
        }
    }

    // Otherwise, assume all other field changes are relevant. Note that we
    // don't need to check if the field name itself is one of the fields we
    // use as the dependency and change management in pcp will have already 
    // taken care of filtering that for us.
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE



