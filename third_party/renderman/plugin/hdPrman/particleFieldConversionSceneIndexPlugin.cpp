//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#include "hdPrman/particleFieldConversionSceneIndexPlugin.h"

#include "pxr/imaging/hd/version.h"

// There was no hdsi/version.h before this HD_API_VERSION.
#if HD_API_VERSION >= 58

#include "pxr/imaging/hdsi/version.h"

#if HDSI_API_VERSION >= 19
#define HDPRMAN_USE_PARTICLE_FIELD_CONVERSION_SCENE_INDEX
#endif

#endif // #if HD_API_VERSION >= 58

#include "hdPrman/tokens.h"
#include "pxr/usd/usdVol/tokens.h"
#include "pxr/usdImaging/usdImaging/tokens.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/tokens.h"
#ifdef HDPRMAN_USE_PARTICLE_FIELD_CONVERSION_SCENE_INDEX
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/materialNetworkSchema.h"
#include "pxr/imaging/hd/materialNodeSchema.h"
#include "pxr/imaging/hd/materialNodeParameterSchema.h"
#include "pxr/imaging/hd/materialConnectionSchema.h"
#include "pxr/imaging/hd/materialBindingsSchema.h"
#include "pxr/imaging/hd/materialBindingSchema.h"
#include "pxr/imaging/hdsi/particleFieldConversionSceneIndex.h"
#endif

#include <array>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((rifalloffshape, "ri:falloffshape"))
    ((rifalloffpower, "ri:falloffpower"))
    ((ripointsshape, "ri:points:shape"))
    ((ripointscamerafacing, "ri:points:camerafacing"))

    (SphericalHarmonicsToColor)
    (resultRGB)
    (PxrConstant)
    (emitColor)
    (ri)
);

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<HdPrman_ParticleFieldConversionSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 1;

    for (const auto& pluginDisplayName : HdPrman_GetPluginDisplayNames()) {
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            pluginDisplayName, HdPrmanPluginTokens->particleFieldConversion, nullptr,
            insertionPhase, HdSceneIndexPluginRegistry::InsertionOrderAtStart);
    }
}

HdPrman_ParticleFieldConversionSceneIndexPlugin::HdPrman_ParticleFieldConversionSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_ParticleFieldConversionSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr& inputScene,
    const HdContainerDataSourceHandle& inputArgs)
{
#ifdef HDPRMAN_USE_PARTICLE_FIELD_CONVERSION_SCENE_INDEX
    /// Set a constant width of 5.6 to bound RenderMan's points within a gaussian falloff power of 2
    static const HdRetainedTypedSampledDataSource<float>::Handle constantWidth =
        HdRetainedTypedSampledDataSource<float>::New(5.6f);

    /// Overlay RenderMan geometry settings
    static const HdDataSourceBaseHandle values[] = {
        HdPrimvarSchema::Builder()
            .SetPrimvarValue(
                HdRetainedTypedSampledDataSource<int>::New(1))
            .SetInterpolation(
                HdPrimvarSchema::BuildInterpolationDataSource(
                    HdPrimvarSchemaTokens->constant)
                ).Build(),
        HdPrimvarSchema::Builder()
            .SetPrimvarValue(
                HdRetainedTypedSampledDataSource<float>::New(2))
            .SetInterpolation(
                HdPrimvarSchema::BuildInterpolationDataSource(
                    HdPrimvarSchemaTokens->constant)
                ).Build(),
        HdPrimvarSchema::Builder()
            .SetPrimvarValue(
                HdRetainedTypedSampledDataSource<int>::New(2))
            .SetInterpolation(
                HdPrimvarSchema::BuildInterpolationDataSource(
                    HdPrimvarSchemaTokens->constant)
                ).Build(),
        HdPrimvarSchema::Builder()
            .SetPrimvarValue(
                HdRetainedTypedSampledDataSource<int>::New(1))
            .SetInterpolation(
                HdPrimvarSchema::BuildInterpolationDataSource(
                    HdPrimvarSchemaTokens->constant)
                ).Build(),
    };

    static const TfToken tokens[] = {
        _tokens->rifalloffshape,
        _tokens->rifalloffpower,
        _tokens->ripointsshape,
        _tokens->ripointscamerafacing,
    };

    static const HdContainerDataSourceHandle geometryOverlay = HdRetainedContainerDataSource::New(
        HdPrimvarsSchema::GetSchemaToken(), HdPrimvarsSchema::BuildRetained(4, tokens, values));

    /// Create spherical harmonics material
    static const HdDataSourceBaseHandle sphericalHarmonicsToColorNode = HdMaterialNodeSchema::Builder()
        .SetNodeIdentifier(HdRetainedTypedSampledDataSource<TfToken>::New(_tokens->SphericalHarmonicsToColor))
        .Build();

    static const TfToken pxrConstantConnectionNames[] = {
        _tokens->emitColor
    };

    static const HdDataSourceBaseHandle pxrConstantConnections[] = {
        HdRetainedSmallVectorDataSource::New(1, std::array<HdDataSourceBaseHandle, 1>{
            HdMaterialConnectionSchema::Builder()
                .SetUpstreamNodePath(HdRetainedTypedSampledDataSource<TfToken>::New(_tokens->SphericalHarmonicsToColor))
                .SetUpstreamNodeOutputName(HdRetainedTypedSampledDataSource<TfToken>::New(_tokens->resultRGB))
                .Build()
        }.data())
    };

    static const HdDataSourceBaseHandle pxrConstantNode = HdMaterialNodeSchema::Builder()
        .SetInputConnections(HdRetainedContainerDataSource::New(
            1, pxrConstantConnectionNames, pxrConstantConnections))
        .SetNodeIdentifier(HdRetainedTypedSampledDataSource<TfToken>::New(_tokens->PxrConstant))
        .Build();

    static const TfToken pxrConstantNodeNames[] = { _tokens->SphericalHarmonicsToColor, _tokens->PxrConstant };
    static const HdDataSourceBaseHandle pxrConstantNodes[] = { sphericalHarmonicsToColorNode, pxrConstantNode };

    static const HdContainerDataSourceHandle nodesDs = HdRetainedContainerDataSource::New(
        2, pxrConstantNodeNames, pxrConstantNodes
    );

    static const HdContainerDataSourceHandle terminalsDs = HdRetainedContainerDataSource::New(
        HdMaterialTerminalTokens->surface,
        HdMaterialConnectionSchema::Builder()
            .SetUpstreamNodePath(HdRetainedTypedSampledDataSource<TfToken>::New(_tokens->PxrConstant))
            .SetUpstreamNodeOutputName(HdRetainedTypedSampledDataSource<TfToken>::New(HdMaterialTerminalTokens->surface))
            .Build()
    );

    static const HdDataSourceBaseHandle materialNetowrkDs = HdMaterialNetworkSchema::Builder()
        .SetNodes(nodesDs)
        .SetTerminals(terminalsDs)
        .Build();

    static const HdContainerDataSourceHandle materialDS = HdMaterialSchema::BuildRetained(
        1, &_tokens->ri, &materialNetowrkDs
    );

    static const HdContainerDataSourceHandle materialOverlay = HdRetainedContainerDataSource::New(HdMaterialSchema::GetSchemaToken(), materialDS);

    return HdsiParticleFieldConversionSceneIndex::New(inputScene, constantWidth, geometryOverlay, materialOverlay);
#else
    return inputScene;
#endif
}

PXR_NAMESPACE_CLOSE_SCOPE
