//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/materialPrimvarTransferSceneIndexPlugin.h"

#if PXR_VERSION >= 2302

#include "hdPrman/tokens.h"

#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hdsi/materialPrimvarTransferSceneIndex.h"
#include "pxr/imaging/hdsi/version.h"
#include "pxr/usd/usdRi/rmanUtilities.h"
#include "pxr/usd/sdf/listOp.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdPrman_MaterialPrimvarTransferSceneIndexPlugin"))
);

////////////////////////////////////////////////////////////////////////////////
// Plugin registrations
////////////////////////////////////////////////////////////////////////////////

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<
        HdPrman_MaterialPrimvarTransferSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    // Should be chained *after* the extComputationPrimvarPruningSceneIndex and
    // procedural expansion.
    // Avoiding an additional dependency on hdGp in hdPrman and hardcoding it
    // for now.
    const HdSceneIndexPluginRegistry::InsertionPhase
        insertionPhase = 3; // HdGpSceneIndexPlugin()::GetInsertionPhase() + 1;

    for( auto const& rendererDisplayName : HdPrman_GetPluginDisplayNames()) {
        // Register the plugins conditionally.
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            rendererDisplayName,
            _tokens->sceneIndexPluginName,
            nullptr, // no argument data necessary
            insertionPhase,
            HdSceneIndexPluginRegistry::InsertionOrderAtStart);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Scene Index Implementations
////////////////////////////////////////////////////////////////////////////////

HdPrman_MaterialPrimvarTransferSceneIndexPlugin::
HdPrman_MaterialPrimvarTransferSceneIndexPlugin() = default;

#if HDSI_API_VERSION >= 18
inline static SdfStringListOp
_ConvertPrimvarToListOp(
    HdContainerDataSourceHandle const& primvarsDs,
    TfToken const& name
) {
    const HdPrimvarSchema primvar =
        HdPrimvarsSchema(primvarsDs).GetPrimvar(name);
    if (auto ds = HdStringDataSource::Cast(primvar.GetPrimvarValue())) {
        const std::string s = ds->GetTypedValue(0.0f);
        return UsdRiConvertRManSetSpecificationToListOp(s);
    }
    return SdfStringListOp();
}

static HdDataSourceBaseHandle
HdPrman_ComposePrimvar(
    HdContainerDataSourceHandle const& geometryPrimvarsDs,
    HdContainerDataSourceHandle const& materialPrimvarsDs,
    const TfToken& name)
{
    if (!UsdRiDoesAttributeUseSetSpecification(name)) {
        return HdsiMaterialPrimvarTransferSceneIndex::DefaultComposeFn(
            geometryPrimvarsDs, materialPrimvarsDs, name);
    }

    // This attribute uses set-specification algebra syntax.
    // Convert set-specfication syntax to list-op form.
    const SdfStringListOp geometryListOp =
        _ConvertPrimvarToListOp(geometryPrimvarsDs, name);
    const SdfStringListOp materialListOp =
        _ConvertPrimvarToListOp(materialPrimvarsDs, name);

    // Apply list operations in weak-to-strong order.  Note that in
    // RenderMan, the material is stronger than the geometry.
    std::vector<std::string> setNames;
    geometryListOp.ApplyOperations(&setNames);
    materialListOp.ApplyOperations(&setNames);

    // Convert result back to RenderMan syntax for a set.  The separator
    // can be either space or comma; here we use space.
    auto resultValueDs =
        HdRetainedTypedSampledDataSource<std::string>::New(
            TfStringJoin(setNames.begin(), setNames.end()));
    static HdTokenDataSourceHandle const interpolationDs =
        HdPrimvarSchema::BuildInterpolationDataSource(
            HdPrimvarSchemaTokens->constant);
    return HdPrimvarSchema::Builder()
        .SetPrimvarValue(resultValueDs)
        .SetInterpolation(interpolationDs)
        .Build();
}
#endif // HDSI_API_VERSION >= 18

HdSceneIndexBaseRefPtr
HdPrman_MaterialPrimvarTransferSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    TF_UNUSED(inputArgs);
#if HDSI_API_VERSION >= 18
    return HdsiMaterialPrimvarTransferSceneIndex::New(
        inputScene, HdPrman_ComposePrimvar);
#else
    return HdsiMaterialPrimvarTransferSceneIndex::New(inputScene);
#endif
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_VERSION >= 2302
