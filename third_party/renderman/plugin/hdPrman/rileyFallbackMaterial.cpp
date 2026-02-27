//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/rileyFallbackMaterial.h"

#include "hdPrman/rileyMaterialSchema.h"
#include "hdPrman/rileyParamSchema.h"
#include "hdPrman/rileyShadingNodeSchema.h"
#include "hdPrman/sceneIndexObserverApi.h"
#include "hdPrman/tokens.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdr/declare.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/usd/sdr/registry.h"

#include "pxr/imaging/hd/mergingSceneIndex.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/retainedSceneIndex.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#if PXR_VERSION < 2505
using SdrIdentifier = NdrIdentifier;
using SdrTokenVec = NdrTokenVec;
#endif

namespace {

HdContainerDataSourceHandle
_MaterialNodeDataSource(
    const TfToken &rileyShadingNodeType,
    const SdrIdentifier &identifier,
    const TfToken &rileyHandle,
    const HdContainerDataSourceHandle &params)
{
    static const SdrTokenVec sourceTypes = {
        TfToken("OSL"),
        TfToken("RmanCpp")
    };

    SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
    SdrShaderNodeConstPtr sdrEntry =
        sdrRegistry.GetShaderNodeByIdentifier(identifier, sourceTypes);

    if (!sdrEntry) {
        TF_CODING_ERROR("Cannot create fallback material. This is because "
                        "the shader identifier %s for node <%s> is unknown.\n",
                        identifier.GetText(), rileyHandle.GetText());
        return nullptr;
    }

    return
        HdPrmanRileyShadingNodeSchema::Builder()
            .SetType(
                HdPrmanRileyShadingNodeSchema::BuildTypeDataSource(
                    rileyShadingNodeType))
            .SetName(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    TfToken(sdrEntry->GetResolvedImplementationURI())))
            .SetHandle(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    rileyHandle))
            .SetParams(
                HdPrmanRileyParamListSchema::Builder()
                    .SetParams(params)
                    .Build())
           .Build();
}

HdContainerDataSourceHandle
_PxrSurfaceParams()
{
    static const TfToken names[] = {
        TfToken("specularModelType"),
        TfToken("diffuseDoubleSided"),
        TfToken("specularDoubleSided"),

        TfToken("diffuseColor"),
        TfToken("diffuseGain"),
        TfToken("specularFaceColor"),
        TfToken("specularEdgeColor"),
        TfToken("specularRoughness"),
        TfToken("presence")
    };

    static HdDataSourceBaseHandle const params[] = {
        /* specularModelType */
        HdPrmanRileyParamSchema::Builder()
            .SetValue(
                HdRetainedTypedSampledDataSource<int>::New(1))
            .Build(),

        /* diffuseDoubleSided */
        HdPrmanRileyParamSchema::Builder()
            .SetValue(
                HdRetainedTypedSampledDataSource<int>::New(1))
            .Build(),

        /* specularDoubleSided */
        HdPrmanRileyParamSchema::Builder()
            .SetValue(
                HdRetainedTypedSampledDataSource<int>::New(1))
            .Build(),


        /* diffuseColor */
        HdPrmanRileyParamSchema::Builder()
            .SetValue(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    TfToken("/UsdPreviewSurfaceParameters:diffuseColorOut")))
            .SetRole(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    HdPrmanRileyAdditionalRoleTokens->colorReference))
            .Build(),

        /* diffuseGain */
        HdPrmanRileyParamSchema::Builder()
            .SetValue(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    TfToken("/UsdPreviewSurfaceParameters:diffuseGainOut")))
            .SetRole(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    HdPrmanRileyAdditionalRoleTokens->floatReference))
            .Build(),

        /* specularFaceColor */
        HdPrmanRileyParamSchema::Builder()
            .SetValue(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    TfToken("/UsdPreviewSurfaceParameters:specularFaceColorOut")))
            .SetRole(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    HdPrmanRileyAdditionalRoleTokens->colorReference))
            .Build(),

        /* specularEdgeColor */
        HdPrmanRileyParamSchema::Builder()
            .SetValue(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    TfToken("/UsdPreviewSurfaceParameters:specularEdgeColorOut")))
            .SetRole(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    HdPrmanRileyAdditionalRoleTokens->colorReference))
            .Build(),

        /* specularRoughness */
        HdPrmanRileyParamSchema::Builder()
            .SetValue(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    TfToken("/UsdPreviewSurfaceParameters:specularRoughnessOut")))
            .SetRole(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    HdPrmanRileyAdditionalRoleTokens->floatReference))
            .Build(),

        /* presence */
        HdPrmanRileyParamSchema::Builder()
            .SetValue(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    TfToken("/Primvar_displayOpacity:resultF")))
            .SetRole(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    HdPrmanRileyAdditionalRoleTokens->floatReference))
            .Build()
    };

    static_assert(TfArraySize(names) == TfArraySize(params));

    return HdPrmanRileyParamContainerSchema::BuildRetained(
        TfArraySize(names), names, params);
}

HdContainerDataSourceHandle
_UsdPreviewSurfaceParametersParams()
{
    return
        HdRetainedContainerDataSource::New(
            TfToken("diffuseColor"),
            HdPrmanRileyParamSchema::Builder()
                .SetValue(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        TfToken("/Primvar_displayColor:resultRGB")))
                .SetRole(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        HdPrmanRileyAdditionalRoleTokens->colorReference))
                .Build(),
            TfToken("roughness"),
            HdPrmanRileyParamSchema::Builder()
                .SetValue(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        TfToken("/Primvar_displayRoughness:resultF")))
                .SetRole(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        HdPrmanRileyAdditionalRoleTokens->floatReference))
                .Build(),
            TfToken("metallic"),
            HdPrmanRileyParamSchema::Builder()
                .SetValue(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        TfToken("/Primvar_displayMetallic:resultF")))
                .SetRole(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        HdPrmanRileyAdditionalRoleTokens->floatReference))
                .Build(),
            TfToken("opacity"),
            HdPrmanRileyParamSchema::Builder()
                .SetValue(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        TfToken("/Primvar_displayOpacity:resultF")))
                .SetRole(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        HdPrmanRileyAdditionalRoleTokens->floatReference))
                .Build()
        );
}

HdContainerDataSourceHandle
_ColorPrimvarReader(
    const TfToken &rileyHandle,
    const TfToken &primvarName,
    const GfVec3f &defaultColor)
{
    return
        _MaterialNodeDataSource(
            HdPrmanRileyShadingNodeSchemaTokens->pattern,
            TfToken("PxrPrimvar"),
            rileyHandle,
            HdRetainedContainerDataSource::New(
                TfToken("type"),
                HdPrmanRileyParamSchema::Builder()
                    .SetValue(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            TfToken("color")))
                    .Build(),
                TfToken("varname"),
                HdPrmanRileyParamSchema::Builder()
                   .SetValue(
                       HdRetainedTypedSampledDataSource<TfToken>::New(
                           primvarName))
                   .Build(),
                TfToken("defaultColor"),
                HdPrmanRileyParamSchema::Builder()
                   .SetValue(
                       HdRetainedTypedSampledDataSource<GfVec3f>::New(
                           defaultColor))
                   .SetRole(
                       HdRetainedTypedSampledDataSource<TfToken>::New(
                           HdPrimvarRoleTokens->color))
                   .Build()));
}

HdContainerDataSourceHandle
_FloatPrimvarReader(
    const TfToken &rileyHandle,
    const TfToken &primvarName,
    const float defaultFloat)
{
    return
        _MaterialNodeDataSource(
            HdPrmanRileyShadingNodeSchemaTokens->pattern,
            TfToken("PxrPrimvar"),
            rileyHandle,
            HdRetainedContainerDataSource::New(
                TfToken("type"),
                HdPrmanRileyParamSchema::Builder()
                    .SetValue(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            TfToken("float")))
                    .Build(),
                TfToken("varname"),
                HdPrmanRileyParamSchema::Builder()
                   .SetValue(
                       HdRetainedTypedSampledDataSource<TfToken>::New(
                           primvarName))
                   .Build(),
                TfToken("defaultFloat"),
                HdPrmanRileyParamSchema::Builder()
                    .SetValue(
                        HdRetainedTypedSampledDataSource<float>::New(
                            defaultFloat))
                    .Build()));
}

// This is an unrolled version of the fallback material from material.cpp
// translated to HdPrmanRileyMaterialSchema.
//
// This should really be an HdMaterialNetworkSchema that is translated
// to a HdPrmanRileyMaterialSchema.
//
// However, we do not have implemented yet the conversion function and scene
// index translating HdMaterialNetworkSchema to HdPrmanRileyMaterialSchema.
//
HdContainerDataSourceHandle
_FallbackMaterialDataSource()
{
    HdDataSourceBaseHandle const nodes[] = {
        _ColorPrimvarReader(
            TfToken("/Primvar_displayColor"),
            TfToken("displayColor"),
            {0.5, 0.5, 0.5}),

        _FloatPrimvarReader(
            TfToken("/Primvar_displayMetallic"),
            TfToken("displayMetallic"),
            0.0f),

        _FloatPrimvarReader(
            TfToken("/Primvar_displayOpacity"),
            TfToken("displayOpacity"),
            1.0f),

        _FloatPrimvarReader(
            TfToken("/Primvar_displayRoughness"),
            TfToken("displayRoughness"),
            1.0f
            ),

        _MaterialNodeDataSource(
            HdPrmanRileyShadingNodeSchemaTokens->pattern,
            TfToken("UsdPreviewSurfaceParameters"),
            TfToken("/UsdPreviewSurfaceParameters"),
            _UsdPreviewSurfaceParametersParams()),

        _MaterialNodeDataSource(
            HdPrmanRileyShadingNodeSchemaTokens->bxdf,
            TfToken("PxrSurface"),
            TfToken("/PxrSurface"),
            _PxrSurfaceParams())
    };

    return
        HdPrmanRileyMaterialSchema::Builder()
            .SetBxdf(
                HdPrmanRileyShadingNodeVectorSchema::BuildRetained(
                    TfArraySize(nodes),
                    nodes))
            .Build();
}

HdSceneIndexBaseRefPtr
_FallbackMaterialScene()
{
    HdRetainedSceneIndexRefPtr const scene = HdRetainedSceneIndex::New();

    scene->AddPrims(
        { { HdPrman_RileyFallbackMaterial::GetPrimPath(),
            HdPrmanRileyPrimTypeTokens->material,
            HdRetainedContainerDataSource::New(
                HdPrmanRileyMaterialSchema::GetSchemaToken(),
                _FallbackMaterialDataSource()) } });

    return scene;
}

} // anon

namespace HdPrman_RileyFallbackMaterial {

const SdfPath &
GetPrimPath()
{
    static const SdfPath result("/__RileyFallbackMaterial");
    return result;
}

HdSceneIndexBaseRefPtr
CreateRetainedSceneIndex()
{
    return _FallbackMaterialScene();
}

HdSceneIndexBaseRefPtr
AppendSceneIndex(const HdSceneIndexBaseRefPtr &inputScene)
{
    HdMergingSceneIndexRefPtr const result = HdMergingSceneIndex::New();

    result->AddInputScene(inputScene, SdfPath::AbsoluteRootPath());

    static HdSceneIndexBaseRefPtr fallbackMaterialScene =
        _FallbackMaterialScene();
    result->AddInputScene(fallbackMaterialScene, GetPrimPath());

    return result;
}

} // HdPrman_RileyFallbackMaterial

PXR_NAMESPACE_CLOSE_SCOPE
