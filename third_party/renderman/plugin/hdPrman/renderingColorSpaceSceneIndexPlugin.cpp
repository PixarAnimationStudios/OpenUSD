//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/renderingColorSpaceSceneIndexPlugin.h"
#include "pxr/base/tf/registryManager.h"

#if PXR_VERSION >= 2308

#include "hdPrman/tokens.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/color.h"
#include "pxr/base/gf/colorSpace.h"

#include "pxr/imaging/hd/dataSourceMaterialNetworkInterface.h"
#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/lightSchema.h"
#include "pxr/imaging/hd/materialFilteringSceneIndexBase.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/renderSettingsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneGlobalsSchema.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/utils.h"

#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdPrman_RenderingColorSpaceSceneIndexPlugin"))

    // Dependencies
    (material_depOn_renderingColorSpace)
    (material_depOn_activeRenderSettingsPrim)
    (__dependencies_depOn_activeRenderSettingsPrim)

    // Light material network terminal names.
    (light)
    (lightFilter)

    // Light Node input/output names.
    ((enableTemp, "enableColorTemperature"))
    ((temp, "colorTemperature"))

    // Legacy Color Space Names to help convert into GfColorSpace Names
    (sRGB)

    // Color Space Transform Node Type, Inputs and Output
    (UsdTransformColor)
    (ColorIn)
    (srcK0)
    (srcPhi)
    (srcGamma)
    (srcLinearBias)
    (dstK0)
    (dstPhi)
    (dstGamma)
    (dstLinearBias)
    (tRow1)
    (tRow2)
    (tRow3)
    (ResultColor)

    // Types of materials or terminals that we do not want to filter
    (mtlx)

    (PxrSurface)
    (PxrPrimvar)
);

TF_MAKE_STATIC_DATA(
    std::vector<
        HdPrman_RenderingColorSpaceSceneIndexPlugin
            ::ColorSpaceRemappingCallback>,
    _colorSpaceRemappingCallbacks)
{
    _colorSpaceRemappingCallbacks->clear();
}

TF_MAKE_STATIC_DATA(
    std::vector<
        HdPrman_RenderingColorSpaceSceneIndexPlugin::ExcludeMaterialCallback>,
    _excludeMaterialCallbacks)
{
    _excludeMaterialCallbacks->clear();
}

TF_MAKE_STATIC_DATA(
    std::vector<
        HdPrman_RenderingColorSpaceSceneIndexPlugin::FilterMaterialCallback>,
    _filterMaterialCallbacks)
{
    _filterMaterialCallbacks->clear();
}

TF_MAKE_STATIC_DATA(
    std::vector<
        HdPrman_RenderingColorSpaceSceneIndexPlugin::FilterLightCallback>,
    _filterLightCallbacks)
{
    _filterLightCallbacks->clear();
}

TF_MAKE_STATIC_DATA(
    std::vector<
        HdPrman_RenderingColorSpaceSceneIndexPlugin::DisableTransformCallback>,
    _disableTransformCallbacks)
{
    _disableTransformCallbacks->clear();
}

namespace {

using VtArrayVec3f = VtArray<GfVec3f>;

////////////////////////////////////////////////////////////////////////////////
// Color Space Helpers


static GfColorSpace
_GetSourceColorSpace(
    const TfToken& nodeName,
    const TfToken& paramName,
    const TfToken& paramColorSpaceName)
{
    TfToken sourceCSName = paramColorSpaceName;

    // If unauthored default to linearRec709
    if (sourceCSName.IsEmpty()) {
        sourceCSName = GfColorSpaceNames->LinearRec709;
        // TF_WARN("Source Color Space on param '%s' node <%s> is not specified "
        //         "using '%s'.", paramName.GetText(), nodeName.GetText(),
        //         sourceCSName.GetText());
    }
    // Assume 'sRGB' is srgb_rec709
    if (sourceCSName == _tokens->sRGB) {
        sourceCSName = GfColorSpaceNames->SRGBRec709;
    }

    sourceCSName =
        HdPrman_RenderingColorSpaceSceneIndexPlugin::RemapColorSpaceName(
            sourceCSName);

    // For other color spaces that we don't recognize default back to raw
    if (!GfColorSpace::IsValid(sourceCSName)) {
        TF_WARN("Source Color Space on param '%s' - node <%s> is invalid (%s) "
                "using 'Raw' instead.",
                paramName.GetText(), nodeName.GetText(), sourceCSName.GetText());
        sourceCSName = GfColorSpaceNames->Raw;
    }
    return GfColorSpace(sourceCSName);
}

static GfColorSpace
_GetRenderingColorSpace(const HdSceneIndexBaseRefPtr &si)
{
    // Default to Linear Rec 709 as the Rendering Color Space.
    TfToken renderingCsName = GfColorSpaceNames->LinearRec709;
    if (!si) {
        return GfColorSpace(renderingCsName);
    }

    // Get the active render settings prim's rendering color space opinion.
    SdfPath activeRspPath;
    if (HdUtils::HasActiveRenderSettingsPrim(si, &activeRspPath)) {
        HdContainerDataSourceHandle primContainer =
            si->GetPrim(activeRspPath).dataSource;

        if (HdRenderSettingsSchema rsSchema =
                HdRenderSettingsSchema::GetFromParent(primContainer)) {
            if (auto rcsHandle = rsSchema.GetRenderingColorSpace()) {
                const TfToken rcsName = rcsHandle->GetTypedValue(0);
                if (!rcsName.IsEmpty()) {
                    renderingCsName = rcsName;
                }
            }
        }
    }

    renderingCsName =
        HdPrman_RenderingColorSpaceSceneIndexPlugin::RemapColorSpaceName(
            renderingCsName);

    // if the authored color space is invalid
    if (!GfColorSpace::IsValid(renderingCsName)) {
        TF_WARN("Rendering Color Space name (%s) is invalid, using "
                "LinearRec709.", renderingCsName.GetText());
        renderingCsName = GfColorSpaceNames->LinearRec709;
    }

    return GfColorSpace(renderingCsName);
}

// Skip color transformations if the source and rendering color spaces are the
// same, or if the source color space should not receive be transformed.
static bool
_SkipColorTransform(
    const GfColorSpace& sourceColorSpace,
    const GfColorSpace& renderingColorSpace)
{
    return (sourceColorSpace == renderingColorSpace ||
            sourceColorSpace.GetName() == GfColorSpaceNames->Raw ||
            sourceColorSpace.GetName() == GfColorSpaceNames->Data ||
            sourceColorSpace.GetName() == GfColorSpaceNames->Identity);
}


////////////////////////////////////////////////////////////////////////////////
// Dependency Data Source
//
// This Data Source is used in both the Material and Light Prim Data Sources.
// It adds dependencies on the Active Render Settings Prim so that when the
// Rendering Color Space on the Active RenderSettings Prim changes that
// material data source can be updated.
class _DependenciesDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_DependenciesDataSource);

    _DependenciesDataSource(
        const HdContainerDataSourceHandle& dependenciesDs,
        const HdSceneIndexBasePtr &si)
    : _input(dependenciesDs)
    , _si(si)
    {
        if (!dependenciesDs) {
            // plausible scenario if no dependencies were added by prior
            // scene indices.
            static HdContainerDataSourceHandle emptyContainer =
                HdRetainedContainerDataSource::New();
            _input = emptyContainer;
        }
    }

    TfTokenVector
    GetNames() override
    {
        TfTokenVector names = _input->GetNames();
        names.push_back(_tokens->__dependencies_depOn_activeRenderSettingsPrim);
        names.push_back(_tokens->material_depOn_activeRenderSettingsPrim);
        names.push_back(_tokens->material_depOn_renderingColorSpace);
        return names;
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        // "dependedOn" locator DS
        static const HdLocatorDataSourceHandle sgActiveRspLocator =
            HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                HdSceneGlobalsSchema::GetActiveRenderSettingsPrimLocator());

        static const HdLocatorDataSourceHandle rsRenderingColorSpaceLocator =
            HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                HdRenderSettingsSchema::GetRenderingColorSpaceLocator());

        // "affected" locator DS
        static const HdLocatorDataSourceHandle dependenciesLocator =
            HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                HdDependenciesSchema::GetDefaultLocator());

        static const HdLocatorDataSourceHandle materialLocator =
            HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                HdMaterialSchema::GetDefaultLocator());

        // "dependedOn" prim path DS
        static const auto sgDefaultPrimPathDS =
            HdRetainedTypedSampledDataSource<SdfPath>::New(
                HdSceneGlobalsSchema::GetDefaultPrimPath());

        if (name == _tokens->__dependencies_depOn_activeRenderSettingsPrim) {
            // Invalidate the prim's dependencies when the active RSP changes.
            static const HdContainerDataSourceHandle __dependenciesDepDs =
                HdDependencySchema::Builder()
                .SetDependedOnPrimPath(sgDefaultPrimPathDS)
                .SetDependedOnDataSourceLocator(sgActiveRspLocator)
                .SetAffectedDataSourceLocator(dependenciesLocator)
                .Build();

            return __dependenciesDepDs;
        }

        if (name == _tokens->material_depOn_activeRenderSettingsPrim) {
            // XXX Unfortunate but necessary. Alternatively, we could use
            //     the active locator on the render settings prim.
            static const HdContainerDataSourceHandle materialDepDS =
                HdDependencySchema::Builder()
                .SetDependedOnPrimPath(sgDefaultPrimPathDS)
                .SetDependedOnDataSourceLocator(sgActiveRspLocator)
                .SetAffectedDataSourceLocator(materialLocator)
                .Build();

            return materialDepDS;
        }

        if (name == _tokens->material_depOn_renderingColorSpace) {
            SdfPath activeRspPath;
            if (HdUtils::HasActiveRenderSettingsPrim(_si, &activeRspPath)) {
                // Invalidate the material locator when the rendering color
                // space on the active RSP changes.
                return
                    HdDependencySchema::Builder()
                    .SetDependedOnPrimPath(
                        HdRetainedTypedSampledDataSource<SdfPath>::New(
                            activeRspPath))
                    .SetDependedOnDataSourceLocator(
                        rsRenderingColorSpaceLocator)
                    .SetAffectedDataSourceLocator(materialLocator)
                    .Build();
            } else {
                return nullptr;
            }
        }

        return _input->Get(name);
    }

private:
    HdContainerDataSourceHandle _input;
    HdSceneIndexBasePtr _si;
};


////////////////////////////////////////////////////////////////////////////////
// Light Conversion Functions

// Checks if a shading node has color temperature enabled.
static bool
_HasTemperatureEnabled(
    const TfToken& nodeName,
    const SdrShaderNodeConstPtr& sdrNode,
    const HdDataSourceMaterialNetworkInterface& interface)
{
    if (const auto input = sdrNode->GetShaderInput(_tokens->enableTemp)) {
        const VtValue value = interface.GetNodeParameterValue(
            nodeName, _tokens->enableTemp);

        return value.GetWithDefault<bool>(
            input->GetDefaultValue().GetWithDefault<bool>(false));
    }

    return false;
}

// Gets the color temperature in a shading node (if set).
static float
_GetTemperatureValue(
    const TfToken& nodeName,
    const SdrShaderNodeConstPtr& sdrNode,
    const HdDataSourceMaterialNetworkInterface& interface)
{
    static const float defaultTemp = 6500.0f;
    float temp = defaultTemp;
    if (const auto input = sdrNode->GetShaderInput(_tokens->temp)) {
        const VtValue value = interface.GetNodeParameterValue(
            nodeName, _tokens->temp);

        temp = value.GetWithDefault<float>(
            input->GetDefaultValue().GetWithDefault<float>(defaultTemp));
    }
    return temp;
}

// XXX the following two helper functions also live in HdEmbree/lightSamplers
// Ideally, we could could move this to GfColor::GetLuminance()
inline float
_GetLuminance(GfColor const& color)
{
    static const GfColorSpace _xyzColorSpace(GfColorSpaceNames->CIEXYZ);
    GfColor xyzColor(color, _xyzColorSpace);
    // The "Y" component in XYZ space is luminance
    return xyzColor.GetRGB()[1];
}

static GfVec3f
_GetLuminanceComponents(const GfColorSpace& sourceColorSpace)
{
    return GfVec3f(
        _GetLuminance(GfColor(GfVec3f::XAxis(), sourceColorSpace)),
        _GetLuminance(GfColor(GfVec3f::YAxis(), sourceColorSpace)),
        _GetLuminance(GfColor(GfVec3f::ZAxis(), sourceColorSpace)));
}

// Returns the neutral temperature color, which can be multiplied with the
// color params in that node to replicate the effect of applying that
// temperature to those colors.
static GfVec3f
_GetTempNeutralColor(const float temp, const GfColorSpace& sourceColorSpace)
{
    GfColor color(sourceColorSpace);
    color.SetFromPlanckianLocus(temp, 1.0f);

    // Perceptually normalize the output color using Luma coefficients.
    GfVec3f colorVec = color.GetRGB();
    float lum = GfDot(colorVec, _GetLuminanceComponents(sourceColorSpace));
    return colorVec /= lum;
}


// Converts the color values in a shading node into the rendering color space,
// burning the temperature into them (if enabled) before that conversion.
// Since the default colors of parameters that are not authored will also need
// to be converted, the corresponding Sdr node is used to find those parameters
// to avoid missing unauthored ones in the scene index prim.
static void
_ConvertLightNodeColors(
    const SdfPath& primPath,
    const TfToken& nodeName,
    HdDataSourceMaterialNetworkInterface& interface,
    const GfColorSpace& renderingColorSpace)
{
    const TfToken& nodeType = interface.GetNodeType(nodeName);

    // Get the corresponding Sdr node to find out its color params.
    const SdrShaderNodeConstPtr sdrNode =
        SdrRegistry::GetInstance().GetShaderNodeByIdentifier(nodeType);

    if (!sdrNode) {
        TF_WARN("Node type '%s' not found in Sdr on <%s>",
            nodeType.GetText(), primPath.GetText());
        return;
    }

    // Get the neutral color for the temperature if enabled.
    float tempValue(1.0f);
    const bool tempEnabled = _HasTemperatureEnabled(nodeName, sdrNode, interface);
    if (tempEnabled) {
        tempValue = _GetTemperatureValue(nodeName, sdrNode, interface);

        // Disable temperature, as it will be burnt into the color params.
        interface.SetNodeParameterValue(
            nodeName, _tokens->enableTemp, VtValue(false));
    }

    // Iterate the color properties in the node, burn the temperature into
    // them (if enabled) and convert them into the rendering color space.
    for (const auto& inputName : sdrNode->GetShaderInputNames()) {
        const auto input = sdrNode->GetShaderInput(inputName);
        if (input->GetType() != SdrPropertyTypes->Color) {
            continue;
        }

        const TfToken& paramName = input->GetName();
        const auto paramData = interface.GetNodeParameterData(
            nodeName, paramName);

        // XXX should we be skipping the built in lights? Or explicitly
        // assigning them a color space?
        const GfColorSpace sourceColorSpace =
            _GetSourceColorSpace(paramName, nodeName, paramData.colorSpace);
        const bool convertColorSpace = (sourceColorSpace != renderingColorSpace);

        if (input->IsArray() || input->IsDynamicArray()) {
            // Get the colors array, falling back into the defaults if the
            // parameter is not authored or has an invalid type.
            VtArrayVec3f colors =
                paramData.value.GetWithDefault<VtArrayVec3f>(
                    input->GetDefaultValue().GetWithDefault<VtArrayVec3f>(
                        VtArrayVec3f()));

            for (size_t i = 0; i < colors.size(); ++i) {
                const GfVec3f adjSourceColor =
                    tempEnabled
                        ? GfCompMult(colors[i],
                            _GetTempNeutralColor(tempValue, sourceColorSpace))
                        : colors[i];
                colors[i] =
                    convertColorSpace 
                        ? renderingColorSpace.Convert(
                            sourceColorSpace, adjSourceColor).GetRGB()
                        : adjSourceColor;
            }

            // Set the modified colors back into the parameter data.
            HdMaterialNetworkInterface::NodeParamData newParamData;
            newParamData.typeName = paramData.typeName;
            newParamData.colorSpace = renderingColorSpace.GetName();
            newParamData.value = VtValue(colors);
            // Update the interface with the new parameter data.
            interface.SetNodeParameterData(nodeName, paramName, newParamData);

        } else {
            // Single color value parameter case.
            // Get the color value, falling back into the default if the
            // parameter is not authored or has an invalid type.
            GfVec3f color(
                paramData.value.GetWithDefault<GfVec3f>(
                    input->GetDefaultValue().GetWithDefault<GfVec3f>(
                        GfVec3f(1.0f))));

            const GfVec3f adjSourceColor =
                tempEnabled
                    ? GfCompMult(color,
                        _GetTempNeutralColor(tempValue, sourceColorSpace))
                    : color;
            color =
                convertColorSpace
                    ? renderingColorSpace.Convert(
                        sourceColorSpace, adjSourceColor).GetRGB()
                    : adjSourceColor;

            // Set the modified colors back into the parameter data.
            HdMaterialNetworkInterface::NodeParamData newParamData;
            newParamData.typeName = paramData.typeName;
            newParamData.colorSpace = renderingColorSpace.GetName();
            newParamData.value = VtValue(color);
            // Update the interface with the new parameter data.
            interface.SetNodeParameterData(nodeName, paramName, newParamData);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// Material Conversion Functions

// Splices in a UsdTransformColor node.
//
// Rewires the network data flow from:
// UpstreamNode.output -> DownstreamNode.input
// to:
// UpstreamNode.output -> ColorTransformNode -> DownstreamNode.input
//
static void
_InsertColorTransform(
    HdMaterialNetworkInterface *material,
    GfColorSpace const& sourceColorSpace,
    GfColorSpace const& renderingColorSpace,
    TfToken const& upstreamNodeName,
    TfToken const& upstreamOutputName,
    TfToken const& downstreamNodeName,
    TfToken const& downstreamInputName)
{
    // Create a UsdTransformColor node
    const TfToken tNode(TfStringPrintf(
        "HdPrman_TransformColor_%s_%s_from_%s_to_%s",
        upstreamNodeName.GetText(),
        upstreamOutputName.GetText(),
        sourceColorSpace.GetName().GetString().c_str(),
        renderingColorSpace.GetName().GetString().c_str()));
    material->SetNodeType(tNode, _tokens->UsdTransformColor);

    // Source Color Space Inputs
    auto [srcK0, srcPhi] = sourceColorSpace.GetTransferFunctionParams();
    float srcGamma = sourceColorSpace.GetGamma();
    float srcLinearBasis = sourceColorSpace.GetLinearBias();
    // Destination Color Space Inputs
    auto [dstK0, dstPhi] = renderingColorSpace.GetTransferFunctionParams();
    float dstGamma = renderingColorSpace.GetGamma();
    float dstLinearBasis = renderingColorSpace.GetLinearBias();
    // Matrix Transform from the source to rendering color space
    const GfMatrix3f tMatrix = renderingColorSpace.GetRGBToRGB(sourceColorSpace);
    const GfVec3f tRow1 = tMatrix.GetRow(0);
    const GfVec3f tRow2 = tMatrix.GetRow(1);
    const GfVec3f tRow3 = tMatrix.GetRow(2);

    // Set variables on UsdTransformColor node
    material->SetNodeParameterValue(tNode, _tokens->srcK0, VtValue(srcK0));
    material->SetNodeParameterValue(tNode, _tokens->srcPhi, VtValue(srcPhi));
    material->SetNodeParameterValue(tNode,
        _tokens->srcGamma, VtValue(srcGamma));
    material->SetNodeParameterValue(
        tNode, _tokens->srcLinearBias, VtValue(srcLinearBasis));
    material->SetNodeParameterValue(tNode, _tokens->dstK0, VtValue(dstK0));
    material->SetNodeParameterValue(tNode, _tokens->dstPhi, VtValue(dstPhi));
    material->SetNodeParameterValue(tNode,  
        _tokens->dstGamma, VtValue(dstGamma));
    material->SetNodeParameterValue(
        tNode, _tokens->dstLinearBias, VtValue(dstLinearBasis));
    material->SetNodeParameterValue(tNode, _tokens->tRow1, VtValue(tRow1));
    material->SetNodeParameterValue(tNode, _tokens->tRow2, VtValue(tRow2));
    material->SetNodeParameterValue(tNode, _tokens->tRow3, VtValue(tRow3));

    // Connect the transform node's input to the upstream output.
    material->SetNodeInputConnection(
        tNode, _tokens->ColorIn,
        {{upstreamNodeName, upstreamOutputName}});

    // Connect the downstream node's input to the transform node's output.
    // Note that the input may have multiple connections, so check
    // specifically for the one that was previously connected the the
    // upstream node's output.
    HdMaterialNetworkInterface::InputConnectionVector updatedConns =
        material->GetNodeInputConnection(
            downstreamNodeName, downstreamInputName);
    for (auto& conn : updatedConns) {
        if (conn.upstreamNodeName == upstreamNodeName &&
            conn.upstreamOutputName == upstreamOutputName) {
            conn = {tNode, _tokens->ResultColor};
        }
    }
    material->SetNodeInputConnection(
        downstreamNodeName, downstreamInputName, updatedConns);
}

static
TfToken
_GetSdrInputType(const TfToken& nodeType, const TfToken& inputName)
{
    const SdrShaderNodeConstPtr sdrNode =
        SdrRegistry::GetInstance().GetShaderNodeByIdentifier(nodeType);
    if (!sdrNode) {
        return TfToken();
    }
    const SdrShaderPropertyConstPtr sdrInput = sdrNode->GetShaderInput(inputName);
    if (!sdrInput) {
        return TfToken();
    }
    return sdrInput->GetType();
}

// This function converts colors from their source color space to the rendering
// color space.
// It uses GfColorSpace to convert authored color values from the source color
// space to the rendering color space.
// It adds a UsdTransformColor node (which is based on the same underlying
// functions used in GfColorSpace) to translate color values that are coming
// from textures - connections to a node with an asset input.
static void
_FilterMaterial(
    HdMaterialNetworkInterface *material,
    const GfColorSpace& renderingColorSpace)
{
    // Rewire all color inputs connected to assets or PxrPrimvar nodes
    for (const TfToken& nodeName : material->GetNodeNames()) {
        const TfToken& nodeType = material->GetNodeType(nodeName);

        for (const TfToken& inputName :
             material->GetNodeInputConnectionNames(nodeName)) {

            // Only adjust the color inputs
            const TfToken inputType = _GetSdrInputType(nodeType, inputName);
            if (inputType != SdrPropertyTypes->Color) {
                continue;
            }

            for (const auto& conn :
                 material->GetNodeInputConnection(nodeName, inputName)) {
                const TfToken& upstreamNodeName = conn.upstreamNodeName;
                const TfToken& upstreamOutputName = conn.upstreamOutputName;

                // Check the upstream node for an authored asset parameter.
                //
                // Note that the asset parameter is not the same as the
                // upstream output, which has type color.  We consult the
                // asset parameter data colorspace to determine the colorspace
                // of the color output.
                for (const auto& paramName :
                     material->GetAuthoredNodeParameterNames(upstreamNodeName)) {
                    const auto paramData = material->GetNodeParameterData(
                        upstreamNodeName, paramName);
                    if (paramData.typeName != SdfValueTypeNames->Asset) {
                        continue;
                    }

                    // We found an asset parameter on the upstream node.
                    // Consult it for the source colorspace.
                    const GfColorSpace sourceColorSpace =
                        _GetSourceColorSpace(
                            upstreamNodeName, upstreamOutputName,
                            paramData.colorSpace);
                    if (_SkipColorTransform(
                            sourceColorSpace, renderingColorSpace)) {
                        continue;
                    }

                    _InsertColorTransform(
                        material, sourceColorSpace, renderingColorSpace,
                        upstreamNodeName, upstreamOutputName,
                        nodeName, inputName);

                    // Update the asset param data with the raw colorspace.
                    // This ensures that no further colorspace transformation
                    // happens.
                    //
                    // XXX Is this correct?  What if there are multiple
                    // downstream nodes connected to this one output?
                    //
                    // XXX Revisit this to make sure we should put this color
                    // space vs the rendering color space
                    //
                    // Create new ParamData in the Rendering Color Space
                    HdMaterialNetworkInterface::NodeParamData newParamData;
                    newParamData.typeName = paramData.typeName;
                    newParamData.value = paramData.value;
                    newParamData.colorSpace = GfColorSpaceNames->Raw;
                    material->SetNodeParameterData(
                        upstreamNodeName, paramName, newParamData);
                }

                // Check for connections to PxrPrimvar color outputs
                const TfToken& upstreamNodeType =
                    material->GetNodeType(upstreamNodeName);
                if (upstreamNodeType == _tokens->PxrPrimvar) {
                    // XXX Currently there is nowhere to specify the
                    // colorspace on a PxrPrimvar, so assume rec709,
                    // as elsewhere
                    const GfColorSpace sourceColorSpace(
                        GfColorSpaceNames->LinearRec709);
                    if (_SkipColorTransform(
                            sourceColorSpace, renderingColorSpace)) {
                        continue;
                    }
                    _InsertColorTransform(
                        material, sourceColorSpace, renderingColorSpace,
                        upstreamNodeName, upstreamOutputName,
                        nodeName, inputName);
                }
            }
        }
    }

    // Adjust the authored color parameters
    for (const TfToken &nodeName : material->GetNodeNames()) {
        for (const TfToken& paramName :
                    material->GetAuthoredNodeParameterNames(nodeName)) {
            const auto paramData =
                material->GetNodeParameterData(nodeName, paramName);

            // Only convert color parameters
            if (paramData.typeName != SdfValueTypeNames->Color3f) {
                continue;
            }

            // Get the Source Color Space
            const GfColorSpace sourceColorSpace =
                _GetSourceColorSpace(
                    paramName, nodeName, paramData.colorSpace);
            if (_SkipColorTransform(sourceColorSpace, renderingColorSpace)){
                continue;
            }

            GfVec3f color(paramData.value.Get<GfVec3f>());
            const GfVec3f convertedColor =
                renderingColorSpace.Convert(sourceColorSpace, color).GetRGB();

            // Create new ParamData in the Rendering Color Space
            HdMaterialNetworkInterface::NodeParamData newParamData;
            newParamData.typeName = paramData.typeName;
            newParamData.colorSpace = renderingColorSpace.GetName();
            newParamData.value = VtValue(convertedColor);
            // Update the material with the new parameter data.
            material->SetNodeParameterData(nodeName, paramName, newParamData);
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
// Material Data Sources
//
// Material Data Source to convert all the material's color and color array
// values to the rendering color space.
class _MaterialDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_MaterialDataSource);

    _MaterialDataSource(
        const HdContainerDataSourceHandle& materialInput,
        const HdContainerDataSourceHandle& primInput,
        const SdfPath& materialPath,
        const HdSceneIndexBaseRefPtr &si)
    : _materialInput(materialInput)
    , _primInput(primInput)
    , _materialPath(materialPath)
    , _si(si)
    {}

    TfTokenVector
    GetNames() override
    {
        return _materialInput->GetNames();
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        HdDataSourceBaseHandle result = _materialInput->Get(name);

        if (HdContainerDataSourceHandle networkContainer =
                HdContainerDataSource::Cast(result)) {

            HdDataSourceMaterialNetworkInterface interface(
                _materialPath, networkContainer, _primInput);

            // Convert color inputs & textures from the sourceCS to renderingCS.
            const GfColorSpace renderingCS = _GetRenderingColorSpace(_si);
            TfToken terminalNodeName;
            if (_ShouldFilterMaterial(&interface, &terminalNodeName)) {
                if (!HdPrman_RenderingColorSpaceSceneIndexPlugin
                        ::FilterMaterial(
                            &interface, terminalNodeName, renderingCS)) {
                    _FilterMaterial(&interface, renderingCS);
                }
            }
            return interface.Finish();
        }

        return result;
    }

private:
    HdContainerDataSourceHandle _materialInput;
    HdContainerDataSourceHandle _primInput;
    SdfPath _materialPath;
    HdSceneIndexBaseRefPtr _si;

    bool _ShouldFilterMaterial(
        HdMaterialNetworkInterface *interface, 
        TfToken *terminalNode = nullptr)
    {
        if (HdPrman_RenderingColorSpaceSceneIndexPlugin
                ::ExcludeMaterial(_si, interface)) {
            return false;
        }

        // Do not filter MaterialX materials as color space trnasformation
        // are currently handled through the generated shaders in the
        // MaterialX filter which runs after this scene index.
        if (_materialInput->Get(_tokens->mtlx)) {
            return false;
        }

        // Filter materials only if they have a "surface" terminal node.
        auto conn = interface->GetTerminalConnection(
            HdMaterialTerminalTokens->surface);
        if (conn.first) {
            TfToken& node = conn.second.upstreamNodeName;
            *terminalNode = node;
            return true;
        }
        return false;
    }
};

// Data Source for Material prims. This overrides the material data source
// to handle color space transformations, and declares necessary dependencies
// to be notified of changes that may affect the rendering color space.
class _MaterialPrimDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_MaterialPrimDataSource);

    _MaterialPrimDataSource(
        const HdContainerDataSourceHandle& inputDataSource,
        const SdfPath& primPath,
        const HdSceneIndexBasePtr &si)
    : _input(inputDataSource)
    , _primPath(primPath)
    , _si(si)
    {}

    TfTokenVector
    GetNames() override
    {
        TfTokenVector names = _input->GetNames();
        names.push_back(HdDependenciesSchemaTokens->__dependencies);
        return names;
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        HdDataSourceBaseHandle result = _input->Get(name);

        // Override the material data source of the prim so we can update the
        // network st colors are transformed into the rendering color space.
        if (name == HdMaterialSchemaTokens->material) {
            if (HdContainerDataSourceHandle materialContainer =
                    HdContainerDataSource::Cast(result)) {
                return _MaterialDataSource::New(
                    materialContainer, _input, _primPath, _si);
            }
        }

        if (name == HdDependenciesSchemaTokens->__dependencies) {
            return _DependenciesDataSource::New(
                HdContainerDataSource::Cast(result), _si);
        }

        return result;
    }

private:
    HdContainerDataSourceHandle _input;
    SdfPath _primPath;
    HdSceneIndexBasePtr _si;
};


////////////////////////////////////////////////////////////////////////////////
// Light Data Sources
//
// Data source for the material of a Light or Light Filter. This converts all
// the light material's color and color array values into the rendering color
// space, and apply the color temperature, if enabled in the material.
class _LightMaterialDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_LightMaterialDataSource);

    _LightMaterialDataSource(
        const HdContainerDataSourceHandle& materialInput,
        const HdContainerDataSourceHandle& primInput,
        const SdfPath& primPath,
        const HdSceneIndexBasePtr &si,
        const TfToken& terminalName)
    : _materialInput(materialInput)
    , _primInput(primInput)
    , _primPath(primPath)
    , _si(si)
    , _terminalName(terminalName)
    {}

    TfTokenVector
    GetNames() override
    {
        return _materialInput->GetNames();
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        HdDataSourceBaseHandle result = _materialInput->Get(name);
        if (HdContainerDataSourceHandle networkContainer =
                HdContainerDataSource::Cast(result)) {

            // Convert color inputs & textures from sourceCS -> to renderingCS.
            HdDataSourceMaterialNetworkInterface interface(
                _primPath, networkContainer, _primInput);

            // Get the terminal node.
            const auto conn = interface.GetTerminalConnection(_terminalName);
            if (conn.first) {
                const GfColorSpace renderingCS = _GetRenderingColorSpace(_si);
                if (!HdPrman_RenderingColorSpaceSceneIndexPlugin::FilterLight(
                        &interface, _primPath, conn.second.upstreamNodeName, 
                        renderingCS)) {
                    // Convert the colors inputs in the node.
                    _ConvertLightNodeColors(_primPath,
                        conn.second.upstreamNodeName, interface, renderingCS);
                }
            }

            return interface.Finish();
        }

        return result;
    }

private:
    HdContainerDataSourceHandle _materialInput;
    HdContainerDataSourceHandle _primInput;
    SdfPath _primPath;
    HdSceneIndexBasePtr _si;
    TfToken _terminalName;
};

// Data Source for a Light or Light Filter prim. This overrides the prim's
// material data source with to handle color space conversions, and declares
// necessary dependencies that may affect the rendering color space.
class _LightPrimDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_LightPrimDataSource);

    _LightPrimDataSource(
        const HdContainerDataSourceHandle& inputDataSource,
        const SdfPath& primPath,
        const HdSceneIndexBasePtr &si,
        const TfToken& terminalName)
    : _primInput(inputDataSource)
    , _primPath(primPath)
    , _si(si)
    , _terminalName(terminalName)
    {}

    TfTokenVector
    GetNames() override
    {
        TfTokenVector names = _primInput->GetNames();
        names.push_back(HdDependenciesSchemaTokens->__dependencies);
        return names;
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        HdDataSourceBaseHandle result = _primInput->Get(name);

        // Override the light's material data source of the prim.
        if (name == HdMaterialSchemaTokens->material) {
            if (HdContainerDataSourceHandle materialContainer =
                    HdContainerDataSource::Cast(result)) {
                return _LightMaterialDataSource::New(
                    materialContainer, _primInput, _primPath, _si, _terminalName);
            }

        } else if (name == HdDependenciesSchemaTokens->__dependencies) {
            return _DependenciesDataSource::New(
                HdContainerDataSource::Cast(result), _si);
        }

        return result;
    }

private:
    HdContainerDataSourceHandle _primInput;
    SdfPath _primPath;
    HdSceneIndexBasePtr _si;
    TfToken _terminalName;
};


// -------------------------------------------------------------------------- //

TF_DECLARE_REF_PTRS(_HdPrmanRenderingColorSpaceSceneIndex);

class _HdPrmanRenderingColorSpaceSceneIndex
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    static _HdPrmanRenderingColorSpaceSceneIndexRefPtr
        New(const HdSceneIndexBaseRefPtr &inputSceneIndex)
    {
        return TfCreateRefPtr(
            new _HdPrmanRenderingColorSpaceSceneIndex(inputSceneIndex));
    }

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override
    {
        const HdSceneIndexBaseRefPtr& si = _GetInputSceneIndex();
        HdSceneIndexPrim prim = si->GetPrim(primPath);

        // Do nothing if we should disable this color space transformation
        if (HdPrman_RenderingColorSpaceSceneIndexPlugin::DisableTransform(si)) {
            return prim;
        }

        if (prim.dataSource) {
            if (prim.primType == HdPrimTypeTokens->material) {
                prim.dataSource =
                    _MaterialPrimDataSource::New(prim.dataSource, primPath, si);
            }
            // Override the prim data source for lights and light filters,
            // passing the correct network material terminal name expected
            // for each case.
            if (HdPrimTypeIsLight(prim.primType)) {
                // Lights.
                prim.dataSource =
                    _LightPrimDataSource::New(
                        prim.dataSource, primPath, si, _tokens->light);

            }
            else if (prim.primType == HdPrimTypeTokens->lightFilter) {
                // Light Filters.
                prim.dataSource =
                    _LightPrimDataSource::New(
                        prim.dataSource, primPath, si, _tokens->lightFilter);
            }
        }

        return prim;
    }

    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override
    {
        return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    }

protected:
    _HdPrmanRenderingColorSpaceSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex)
      : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
    {
    }

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override
    {
        _SendPrimsAdded(entries);
    }

    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override
    {
        _SendPrimsRemoved(entries);
    }

    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override
    {
        _SendPrimsDirtied(entries);
    }
};

} // anonymous namespace

////////////////////////////////////////////////////////////////////////////////
// Plugin registrations
////////////////////////////////////////////////////////////////////////////////

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<
        HdPrman_RenderingColorSpaceSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 5;

    for (auto const& pluginDisplayName : HdPrman_GetPluginDisplayNames()) {
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            pluginDisplayName,
            _tokens->sceneIndexPluginName,
            /* inputArgs =*/ nullptr,
            insertionPhase,
            HdSceneIndexPluginRegistry::InsertionOrderAtStart);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Scene Index Implementations
////////////////////////////////////////////////////////////////////////////////

HdPrman_RenderingColorSpaceSceneIndexPlugin::
HdPrman_RenderingColorSpaceSceneIndexPlugin()
{
    TfRegistryManager::GetInstance()
        .SubscribeTo<HdPrman_RenderingColorSpaceSceneIndexPlugin>();
}

/* static */
void
HdPrman_RenderingColorSpaceSceneIndexPlugin::RegisterColorSpaceRemappingCallback(
    const ColorSpaceRemappingCallback& callback)
{
    _colorSpaceRemappingCallbacks->push_back(callback);
}

TfToken
HdPrman_RenderingColorSpaceSceneIndexPlugin::RemapColorSpaceName(
    const TfToken& csName)
{
    TfToken convertedCsName = csName;
    for (const auto& callback : *_colorSpaceRemappingCallbacks) {
        convertedCsName = callback(csName);
    }
    return convertedCsName;
}

/* static */
void
HdPrman_RenderingColorSpaceSceneIndexPlugin::
RegisterExcludeMaterialCallback(const ExcludeMaterialCallback& callback)
{
    _excludeMaterialCallbacks->push_back(callback);
}

bool
HdPrman_RenderingColorSpaceSceneIndexPlugin::ExcludeMaterial(
    const HdSceneIndexBaseRefPtr &si,
    HdMaterialNetworkInterface* interface)
{
    for (const auto& callback : *_excludeMaterialCallbacks) {
        if (callback(si, interface)) {
            return true;
        }
    }
    return false;
}

/* static */
void
HdPrman_RenderingColorSpaceSceneIndexPlugin::
RegisterFilterMaterialCallback(const FilterMaterialCallback& callback)
{
    _filterMaterialCallbacks->push_back(callback);
}

bool
HdPrman_RenderingColorSpaceSceneIndexPlugin::FilterMaterial(
    HdMaterialNetworkInterface* interface,
    const TfToken& terminalNodeName,
    const GfColorSpace& renderingCS)
{
    for (const auto& callback : *_filterMaterialCallbacks) {
        if (callback(interface, terminalNodeName, renderingCS)) {
            return true;
        }
    }
    return false;
}

/* static */
void
HdPrman_RenderingColorSpaceSceneIndexPlugin::
RegisterFilterLightCallback(const FilterLightCallback& callback)
{
    _filterLightCallbacks->push_back(callback);
}

bool
HdPrman_RenderingColorSpaceSceneIndexPlugin::FilterLight(
    HdMaterialNetworkInterface* interface,
    const SdfPath& primPath,
    const TfToken& nodeName,
    const GfColorSpace& renderingCS)
{
    for (const auto& callback : *_filterLightCallbacks) {
        if (callback(interface, primPath, nodeName, renderingCS)) {
            return true;
        }
    }
    return false;
}

/* static */
void
HdPrman_RenderingColorSpaceSceneIndexPlugin::RegisterDisableTransformCallback(
    const DisableTransformCallback& callback)
{
    _disableTransformCallbacks->push_back(callback);
}

bool
HdPrman_RenderingColorSpaceSceneIndexPlugin::DisableTransform(
    const HdSceneIndexBaseRefPtr& si)
{
    for (const auto& callback : *_disableTransformCallbacks) {
        if (callback(si)) {
            return true;
        }
    }
    return false;
}

HdSceneIndexBaseRefPtr
HdPrman_RenderingColorSpaceSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    return _HdPrmanRenderingColorSpaceSceneIndex::New(inputScene);
}




PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2308
