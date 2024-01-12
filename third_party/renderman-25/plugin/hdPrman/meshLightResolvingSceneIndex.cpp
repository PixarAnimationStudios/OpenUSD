//
// Copyright 2022 Pixar
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

#include "hdPrman/meshLightResolvingSceneIndex.h"

#include "hdPrman/debugCodes.h"
#include "hdPrman/tokens.h"

#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hd/containerDataSourceEditor.h"
#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/imaging/hd/dataSourceMaterialNetworkInterface.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/instancedBySchema.h"
#include "pxr/imaging/hd/lightSchema.h"
#if HD_API_VERSION >= 51
#include "pxr/imaging/hd/materialBindingsSchema.h"
#else
#include "pxr/imaging/hd/materialBindingSchema.h"
#endif
#include "pxr/imaging/hd/materialNetworkSchema.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/meshSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/schema.h" 
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/visibilitySchema.h"
#include "pxr/imaging/hd/volumeFieldBindingSchema.h"
#include "pxr/imaging/hd/xformSchema.h"

#include "pxr/usdImaging/usdImaging/modelSchema.h"

#include "pxr/usd/usdLux/tokens.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

/** Mesh Lights
 *
 * Mesh lights are meshes with the MeshLightAPI applied. They have aspects of
 * both traditional Rprims and Sprims. Hydra generally treats them as Rprims.
 * It's up to the render bridge to notice the applied API and do something
 * about it.
 * 
 * For Prman, that means splitting the mesh light into a mesh (Rprim) and a
 * light (Sprim). The Rprim part is easy enough -- we just take the incoming
 * prim (called the "origin" prim throughout) and strip off the features
 * that were added by the Light API. It continues downstream as just an ordinary
 * mesh. The Sprim part (which I'll call the "meshLight" prim, after its prim
 * type) is a bit trickier, due to limitations in Prman and other special
 * considerations.
 * 
 * The first issue is that Prman does not allow us to reuse the riley geometry
 * prototype we create for the stripped-down origin (mesh) prim as the geometry
 * prototype for the meshLight prim. https://jira.pixar.com/browse/RMAN-19686
 * We must make a second riley geometry prototype for the meshLight prim.
 * This required a special, prototype-only path through HdPrman_Gprim::Sync(),
 * which is triggered by certain gprim prim types. In our case, 
 * "meshLightSource" will be the prim type of this third prim. (I call this
 * the "source" prim.) The source prim must be synced before we can fully sync
 * the meshLight prim, since we need its geometry prototype id to create the
 * riley light instance for the meshLight. We resolve this by explicitly syncing
 * the source mesh during sync of the meshLight; see HdPrmanLight::Sync() for
 * details.
 * 
 * The next issue is that there is a parameter in the Light API that controls
 * the color of the light emitted by the meshLight prim based on the material
 * bound to the origin prim. This parameter ("materialSyncMode") has three
 * possible values:
 * 
 *   * "materialGlowTintsLight" : The "glow" signal from the bound material
 *     should be forwarded to the light shader's "textureColor" input. This is
 *     the default for mesh lights.
 * 
 *   * "independent" : The bound material's glow signal and the light's emission
 *     color are independent of one another, and both affect the scene.
 * 
 *   * "noMaterialResponse" : The material bound to the mesh light has no
 *     contribution to lighting at all. This means that it's not directly
 *     visible at all, and only the light's emission affects the scene.
 * 
 * When set to "materialGlowTintsLight", we have to alter the light shader
 * we got from the Light API on the incoming origin prim to include the glow
 * signal from the bound material and any additional shader nodes it requires.
 * 
 * When set to "noMaterialResponse", we have to omit (or remove) the stripped-
 * down origin prim. By not passing it through at all, we achieve the required
 * visual response.
 * 
 * When set to "independent", we do not have to modify anything, since our
 * overall approach is one of independence. We can just use the light shader
 * we got from the Light API as-is, and forward the stripped-down origin prim
 * as normal.
 * 
 * The final issue we can work around is that there is another bug in Prman
 * that causes a crash when the geometry prototype and light shader associated
 * with a light instance both undergo rapid simultaneous changes. 
 * https://jira.pixar.com/browse/RMAN-20136. We won't handle that issue here;
 * see HdPrmanLight::Sync() for details.
 * 
 * There are still some caveats. First, mesh lights are not expected to work
 * without the stage scene index. They are a Hydra-2.0 thing. Backporting them
 * to legacy hydra would be challenging given the constraints of the scene
 * delegate interface. Using mesh lights in the prototype of a point instancer
 * will certainly cause errors if the stage scene index is not enabled, because
 * there is no way to add the meshLight and source prims as children of a point
 * instancer prototype in legacy Hydra. Second, geom subsets with different
 * material bindings will not affect the emission color when materialSyncMode
 * is set to "materialGlowTintsLight". This is because there is currently no
 * way to communicate multiple, subset-specific light shader resources through
 * the scene delegate interface. Material bindings on subsets are ignored. (Note
 * however that some of the code below anticipates support for geom subsets 
 * becoming possible in the future.)
 */

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // material network tokens
    (PxrSurface)
    (PxrVolume)
    (glowColor)
    (emitColor)
    ((textureColor, "ri:light:textureColor"))

    // prim tokens not exported elsewhere
    (usdCollections)

    // dependency tokens
    (meshLight_dep_instancedBy)
    (meshLight_dep_light)
    (meshLight_dep_material)
    (meshLight_dep_material_boundMaterial)
    (meshLight_dep_material_materialBinding)
    (meshLight_dep_materialBinding)
    (meshLight_dep_mesh)
    (meshLight_dep_primvars)
    (meshLight_dep_usdCollections)
    (meshLight_dep_visibility)
    (meshLight_dep_volumeFieldBinding)
    (meshLight_dep_xform)
    
    // synthesized prim names
    ((meshLightLightName, "__meshLight_light"))
    ((meshLightSourceName, "__meshLight_sourceMesh"))
    ((meshLightMeshName, "__meshLight_mesh"))

    // render context / material network selector
    ((renderContext, "ri"))
);

// Internal helpers
namespace {

bool
_IsMeshLight(
    const HdSceneIndexPrim& prim)
{
    if ((prim.primType == HdPrimTypeTokens->mesh) ||
        (prim.primType == HdPrimTypeTokens->volume)) {
        if (auto lightSchema = HdLightSchema::GetFromParent(prim.dataSource)) {
            if (auto dataSource = HdBoolDataSource::Cast(
                    lightSchema.GetContainer()->Get(HdTokens->isLight))) {
                return dataSource->GetTypedValue(0.0f);
            }
        }
    }
    return false;
}

bool
_HasValidMaterialNetwork(
    const HdSceneIndexPrim& prim)
{
    HdMaterialSchema matSchema = HdMaterialSchema::GetFromParent(prim.dataSource);
    if (!matSchema.IsDefined()) {
        return false;
    }
    HdContainerDataSourceHandle matDS = matSchema
        .GetMaterialNetwork(_tokens->renderContext);
    if (!matDS) {
        return false;
    }
    auto netSchema = HdMaterialNetworkSchema(matDS);
    if (!netSchema.IsDefined()) {
        return false;
    }
    HdContainerDataSourceHandle nodesDS = netSchema.GetNodes();
    HdContainerDataSourceHandle terminalsDS = netSchema.GetTerminals();
    return nodesDS && terminalsDS;
}

TfToken
_GetMaterialSyncMode(
    const HdContainerDataSourceHandle& primDs)
{
    if (auto lightSchema = HdLightSchema::GetFromParent(primDs)) {
        if (auto dataSource = HdTokenDataSource::Cast(
                lightSchema.GetContainer()->Get(HdTokens->materialSyncMode))) {
            return dataSource->GetTypedValue(0.0f);
        }
    }

    // default for mesh lights
    return UsdLuxTokens->materialGlowTintsLight;
}

SdfPath
_GetBoundMaterialPath(
    const HdContainerDataSourceHandle& primDS)
{
#if HD_API_VERSION >= 51
    HdMaterialBindingsSchema materialBindings =
        HdMaterialBindingsSchema::GetFromParent(primDS);
    HdMaterialBindingSchema materialBinding =
        materialBindings.GetMaterialBinding();
    if (HdPathDataSourceHandle const ds = materialBinding.GetPath()) {
        return ds->GetTypedValue(0.0f);
    }
#else
    if (auto matBindSchema = HdMaterialBindingSchema::GetFromParent(primDS)) {
        if (auto matBindDS = matBindSchema.GetMaterialBinding()) {
            return matBindDS->GetTypedValue(0.0f);
        }
    }
#endif
    return SdfPath();
}

SdfPathVector
_GetLightFilterPaths(
    const HdContainerDataSourceHandle &inputContainer)
{
    if (HdLightSchema lightSchema = 
            HdLightSchema::GetFromParent(inputContainer)) {
        if (auto dataSource = HdTypedSampledDataSource<SdfPathVector>::Cast(
                lightSchema.GetContainer()->Get(HdTokens->filters))) {
            return dataSource->GetTypedValue(0.0f);
        }
    }
    return SdfPathVector();
}

HdContainerDataSourceHandle
_BuildLightShaderDataSource(
    const SdfPath& originPath,
    const HdSceneIndexPrim& originPrim,
    const HdContainerDataSourceHandle& bindingSourceDS,
    const HdSceneIndexBaseRefPtr& inputSceneIndex)
{
    // XXX: bindingSourceDS and originPrim.dataSource will typically be the
    // same. bindingSourceDS exists in case the origin prim and material
    // binding source are different, as would be the case with geom subsets.
    // Having different light shaders for each geom subset is not supported yet
    // as there is no way to squeeze them through the scene delegate interface,
    // but we do expect to support this in the future.

    const TfToken terminalToken =
        originPrim.primType == HdPrimTypeTokens->volume
          ? HdMaterialTerminalTokens->volume
          : HdMaterialTerminalTokens->surface;
    const TfToken expectedShader =
        terminalToken == HdMaterialTerminalTokens->volume
          ? _tokens->PxrVolume
          : _tokens->PxrSurface;
    const TfToken glowParam =
        expectedShader == _tokens->PxrVolume
          ? _tokens->emitColor
          : _tokens->glowColor;

    // Get the original light shader network
    const HdContainerDataSourceHandle& originalShaderDS = 
        HdMaterialSchema::GetFromParent(originPrim.dataSource)
            .GetMaterialNetwork(_tokens->renderContext);

    // check materialSyncMode
    if (_GetMaterialSyncMode(originPrim.dataSource) !=
        UsdLuxTokens->materialGlowTintsLight) {
        // material does not affect light shader; return unmodified
        return originalShaderDS;
    }

    // check the bound material path
    const SdfPath matPath = _GetBoundMaterialPath(bindingSourceDS);
    if (matPath.IsEmpty()) {
        // no material bound to origin prim; return unmodified
        return originalShaderDS;
    }

    // retrieve the material prim and get its shader network
    const HdSceneIndexPrim& matPrim = inputSceneIndex->GetPrim(matPath);
    const HdContainerDataSourceHandle& matDS =
        HdMaterialSchema::GetFromParent(matPrim.dataSource)
        .GetMaterialNetwork(_tokens->renderContext);
    if (!matDS) {
        // could not get material shader network from material prim;
        // return unmodified
        TF_DEBUG(HDPRMAN_MESHLIGHT).Msg("Could not get material shader network "
            "from material prim; shader for %s light <%s> will not be "
            "modified\n", originPrim.primType.GetText(), originPath.GetText());
        return originalShaderDS;
    }
    
    // interface with the material shader network
    const HdDataSourceMaterialNetworkInterface srcMatNI(matPath, matDS,
                                                        originPrim.dataSource);
    // look up the surface/volume terminal connection
    const auto& terminalConn = srcMatNI.GetTerminalConnection(terminalToken);
    if (!terminalConn.first) {
        // no surface/volume terminal connection; return unmodified
        TF_DEBUG(HDPRMAN_MESHLIGHT).Msg("Could not locate %s terminal "
            "connection; shader for %s light <%s> will not be modified\n",
            terminalToken.GetText(), originPrim.primType.GetText(),
            originPath.GetText());
        return originalShaderDS;
    }

    // check the terminal's upstream node is of a supported type
    const TfToken& nodeType = srcMatNI.GetNodeType(
        terminalConn.second.upstreamNodeName);
    
    if (nodeType != expectedShader) {
        // unsupported node type; return unmodified
        TF_DEBUG(HDPRMAN_MESHLIGHT).Msg("%s terminal upstream node is not "
            "%s; shader for %s light <%s> will not be modified\n",
            terminalToken.GetText(), expectedShader.GetText(),
            originPrim.primType.GetText(), originPath.GetText());
        return originalShaderDS;
    }

    // interface with the original light shader network
    HdDataSourceMaterialNetworkInterface shaderNI(originPath, originalShaderDS,
                                                  originPrim.dataSource);
    // look up the light terminal connection
    const auto lightTC = shaderNI.GetTerminalConnection(
        HdMaterialTerminalTokens->light);
    
    // try for material's glow input connection
    const auto glowIC = srcMatNI.GetNodeInputConnection(
        terminalConn.second.upstreamNodeName, glowParam);
    if (!glowIC.empty()) {
        // glow input connection exists; set as textureColor on
        // light terminal's upstream node
        shaderNI.SetNodeInputConnection(
            lightTC.second.upstreamNodeName,
            _tokens->textureColor,
            glowIC);
        
        // return the modified light shader

        // XXX: We a local copy of the nodes, since shader networks cannot
        // reference nodes in other networks. But we are not bothering to walk
        // the graph and pull only the nodes we actually need. We just copy
        // over all the nodes.
        return HdOverlayContainerDataSource::New(
            shaderNI.Finish(),
            HdMaterialNetworkSchema::Builder()
                .SetNodes(HdMaterialNetworkSchema(matDS).GetNodes())
                .Build());
    }
    // No glow input connection; try for param value instead
    const VtValue glowIV = srcMatNI.GetNodeParameterValue(
        terminalConn.second.upstreamNodeName, glowParam);
    if (glowIV.IsHolding<GfVec3f>()) {
        // glow param value exists; set as textureColor on
        // light terminal's upstream node
        shaderNI.SetNodeParameterValue(
            lightTC.second.upstreamNodeName,
            _tokens->textureColor,
            glowIV);
        // return the modified light shader. No need to copy any nodes.
        return shaderNI.Finish();
    }

    // No glow param value either; return unmodified
    return originalShaderDS;
}

HdContainerDataSourceHandle
_BuildLightDependenciesDataSource(
    const SdfPath& originPath,
    const HdContainerDataSourceHandle& originDS,
    const SdfPath& bindingSourcePath,
    const HdContainerDataSourceHandle& bindingSourceDS)
{
    // XXX: As with _BuildLightShaderDataSource above, bindingSource will
    // ordinarily be the same as origin, except in the (not yet supported)
    // case of geom subsets.

    // Data source locators

    // light
    static const HdLocatorDataSourceHandle lightDSL =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdLightSchema::GetDefaultLocator());
    
    // light.filters
    static const HdLocatorDataSourceHandle lightFiltersDSL =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdLightSchema::GetDefaultLocator().Append(HdTokens->filters));

    // material
    static const HdLocatorDataSourceHandle materialDSL =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdMaterialSchema::GetDefaultLocator());

    // material binding
    static const HdLocatorDataSourceHandle materialBindingsDSL =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
#if HD_API_VERSION >= 51
            HdMaterialBindingsSchema::GetDefaultLocator());
#else
            HdMaterialBindingSchema::GetDefaultLocator());
#endif

    // usdCollections
    static const HdLocatorDataSourceHandle usdCollectionsDSL =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdDataSourceLocator(_tokens->usdCollections));

    // visibility
    static const HdLocatorDataSourceHandle visibilityDSL =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdVisibilitySchema::GetDefaultLocator());
    
    // xform
    static const HdLocatorDataSourceHandle xformDSL =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdXformSchema::GetDefaultLocator());
    
    // instanced by
    static const HdLocatorDataSourceHandle instancedByDSL =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdInstancedBySchema::GetDefaultLocator());
    
    // Build dependencies data source
    
    // Read "-->" in comments as "depends on"

    std::vector<TfToken> names;
    std::vector<HdDataSourceBaseHandle> sources;

    const auto originPathDS =
        HdRetainedTypedSampledDataSource<SdfPath>::New(originPath);

    // meshLight.light --> origin.light
    names.push_back(_tokens->meshLight_dep_light);
    sources.push_back(HdDependencySchema::Builder()
        .SetDependedOnPrimPath(originPathDS)
        .SetDependedOnDataSourceLocator(lightDSL)
        .SetAffectedDataSourceLocator(lightDSL)
        .Build());

    // meshLight.material --> origin.material (the light shader)
    names.push_back(_tokens->meshLight_dep_material);
    sources.push_back(HdDependencySchema::Builder()
        .SetDependedOnPrimPath(originPathDS)
        .SetDependedOnDataSourceLocator(materialDSL)
        .SetAffectedDataSourceLocator(materialDSL)
        .Build());

    // meshLight.material --> bindingSource.materialBinding
    names.push_back(_tokens->meshLight_dep_material_materialBinding);
    sources.push_back(HdDependencySchema::Builder()
        .SetDependedOnPrimPath(
            HdRetainedTypedSampledDataSource<SdfPath>::New(bindingSourcePath))
        .SetDependedOnDataSourceLocator(materialBindingsDSL)
        .SetAffectedDataSourceLocator(materialDSL)
        .Build());

    // meshLight.usdCollections --> origin.usdCollections
    names.push_back(_tokens->meshLight_dep_usdCollections);
    sources.push_back(HdDependencySchema::Builder()
        .SetDependedOnPrimPath(originPathDS)
        .SetDependedOnDataSourceLocator(usdCollectionsDSL)
        .SetAffectedDataSourceLocator(usdCollectionsDSL)
        .Build());

    // meshLight.visibility --> origin.visibility
    names.push_back(_tokens->meshLight_dep_visibility);
    sources.push_back(HdDependencySchema::Builder()
        .SetDependedOnPrimPath(originPathDS)
        .SetDependedOnDataSourceLocator(visibilityDSL)
        .SetAffectedDataSourceLocator(visibilityDSL)
        .Build());
    
    // meshLight.xform --> origin.xform
    names.push_back(_tokens->meshLight_dep_xform);
    sources.push_back(HdDependencySchema::Builder()
        .SetDependedOnPrimPath(originPathDS)
        .SetDependedOnDataSourceLocator(xformDSL)
        .SetAffectedDataSourceLocator(xformDSL)
        .Build());
    
    // meshLight.instancedBy --> origin.instancedBy
    names.push_back(_tokens->meshLight_dep_instancedBy);
    sources.push_back(HdDependencySchema::Builder()
        .SetDependedOnPrimPath(originPathDS)
        .SetDependedOnDataSourceLocator(instancedByDSL)
        .SetAffectedDataSourceLocator(instancedByDSL)
        .Build());

    // meshLight.material --> <bindingSource.materialBinding>.material
    names.push_back(_tokens->meshLight_dep_material_boundMaterial);
    sources.push_back(HdDependencySchema::Builder()
        .SetDependedOnPrimPath(
#if HD_API_VERSION >= 51
            HdMaterialBindingsSchema::GetFromParent(bindingSourceDS)
                .GetMaterialBinding()
            .GetPath())
#else
            HdMaterialBindingSchema::GetFromParent(bindingSourceDS)
                .GetMaterialBinding())
#endif
        .SetDependedOnDataSourceLocator(materialDSL)
        .SetAffectedDataSourceLocator(materialDSL)
        .Build());
    
    // XXX: Light filter dependencies *should* look like this:
    //   meshLight.material --> origin.material,
    //   origin.material --> origin.light.filters,
    //   origin.material --> <each filter>
    // If they did, we would not need any direct dependencies on the
    // light filter prims here. But light filters are not yet Hydra 2.0 enabled,
    // so we will put those direct dependencies here. Delete this stuff when
    // lights and light filters are properly handling the origin.material -->
    // <each filter> dependencies. (Note that the meshLight.material -->
    // origin.light.filters dependency is covered by the meshLight.material -->
    // origin.light dependency.)

    static const std::string prefix = "meshLight_dep_material_filter_";
    for (const SdfPath& filterPath : _GetLightFilterPaths(originDS)) {
        names.push_back(TfToken(prefix + filterPath.GetAsString()));
        sources.push_back(HdDependencySchema::Builder()
            .SetDependedOnPrimPath(
                HdRetainedTypedSampledDataSource<SdfPath>::New(filterPath))
            .SetDependedOnDataSourceLocator(nullptr)
            .SetAffectedDataSourceLocator(materialDSL)
            .Build());
    }

    // And since these dependencies are dynamic, 
    // meshLight.__dependencies --> origin.light.filters
    static const TfToken filtersDepToken("meshLight_dep_dependencies_filters");
    names.push_back(filtersDepToken);
    sources.push_back(HdDependencySchema::Builder()
        .SetDependedOnPrimPath(originPathDS)
        .SetDependedOnDataSourceLocator(lightFiltersDSL)
        .SetAffectedDataSourceLocator(materialDSL)
        .Build());

    return HdRetainedContainerDataSource::New(
        names.size(), names.data(), sources.data());
}

HdContainerDataSourceHandle
_BuildLightDataSource(
    const SdfPath& originPath,
    const HdSceneIndexPrim& originPrim,
    const SdfPath& bindingSourcePath,
    const HdContainerDataSourceHandle& bindingSourceDS,
    const HdSceneIndexBaseRefPtr& inputSceneIndex)
{
    const TfToken materialSyncMode = _GetMaterialSyncMode(originPrim.dataSource);
    std::vector<TfToken> names;
    std::vector<HdDataSourceBaseHandle> sources;

    // revised light shader network with glow signal from bound material
    if (materialSyncMode == UsdLuxTokens->materialGlowTintsLight) {
        names.push_back(HdMaterialSchemaTokens->material);
        sources.push_back(HdRetainedContainerDataSource::New(
            _tokens->renderContext,
            _BuildLightShaderDataSource(
                originPath, originPrim,
                bindingSourceDS,
                inputSceneIndex)));
    }
      
    // Add a link to the source mesh
    names.push_back(HdLightSchemaTokens->light);
    sources.push_back(HdRetainedContainerDataSource::New(
        HdPrmanTokens->sourceGeom,
        HdRetainedTypedSampledDataSource<SdfPath>::New(
            originPath.AppendChild(_tokens->meshLightSourceName))));
    
    // Add dependencies
    names.push_back(HdDependenciesSchemaTokens->__dependencies);
    sources.push_back(
        _BuildLightDependenciesDataSource(
            originPath, originPrim.dataSource,
            bindingSourcePath, bindingSourceDS));
    
    // Knock out primvars
    names.push_back(HdPrimvarsSchemaTokens->primvars);
    sources.push_back(HdBlockDataSource::New());

    // Knock out model
    names.push_back(UsdImagingModelSchemaTokens->model);
    sources.push_back(HdBlockDataSource::New());

    // Knock out mesh
    names.push_back(HdMeshSchemaTokens->mesh);
    sources.push_back(HdBlockDataSource::New());

    // Knock out material binding
#if HD_API_VERSION >= 51
    names.push_back(HdMaterialBindingsSchema::GetSchemaToken());
#else
    names.push_back(HdMaterialBindingSchemaTokens->materialBinding);
#endif
    sources.push_back(HdBlockDataSource::New());

    // Knock out volume field binding
    names.push_back(HdVolumeFieldBindingSchemaTokens->volumeFieldBinding);
    sources.push_back(HdBlockDataSource::New());
      

    return HdOverlayContainerDataSource::New(
        HdRetainedContainerDataSource::New(
            names.size(), names.data(), sources.data()),
        originPrim.dataSource);
}

HdContainerDataSourceHandle
_BuildSourceDependenciesDataSource(
    const SdfPath originPath)
{
    // Data source locators

    // mesh
    static const HdLocatorDataSourceHandle meshDSL =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdMeshSchema::GetDefaultLocator());

    // primvars
    static const HdLocatorDataSourceHandle primvarsDSL =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdPrimvarsSchema::GetDefaultLocator());
    
    // material binding
    static const HdLocatorDataSourceHandle materialBindingsDSL =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
#if HD_API_VERSION >= 51
            HdMaterialBindingsSchema::GetDefaultLocator());
#else
            HdMaterialBindingSchema::GetDefaultLocator());
#endif
    
    // volume field binding
    static const HdLocatorDataSourceHandle volumeFieldBindingDSL =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdVolumeFieldBindingSchema::GetDefaultLocator());

    // Build dependencies data source
    
    // Read "-->" in comments as "depends on"

    std::vector<TfToken> names;
    std::vector<HdDataSourceBaseHandle> sources;

    const auto originPathDS = HdRetainedTypedSampledDataSource<SdfPath>
        ::New(originPath);
    
    // source.mesh --> origin.mesh
    names.push_back(_tokens->meshLight_dep_mesh);
    sources.push_back(HdDependencySchema::Builder()
        .SetDependedOnPrimPath(originPathDS)
        .SetDependedOnDataSourceLocator(meshDSL)
        .SetAffectedDataSourceLocator(meshDSL)
        .Build());

    // source.primvars --> origin.primvars
    names.push_back(_tokens->meshLight_dep_primvars);
    sources.push_back(HdDependencySchema::Builder()
        .SetDependedOnPrimPath(originPathDS)
        .SetDependedOnDataSourceLocator(primvarsDSL)
        .SetAffectedDataSourceLocator(primvarsDSL)
        .Build());

    // source.materialBinding --> origin.materialBinding (for displacement)
    names.push_back(_tokens->meshLight_dep_materialBinding);
    sources.push_back(HdDependencySchema::Builder()
        .SetDependedOnPrimPath(originPathDS)
        .SetDependedOnDataSourceLocator(materialBindingsDSL)
        .SetAffectedDataSourceLocator(materialBindingsDSL)
        .Build());
    
    // source.volumeFieldBinding --> origin.volumeFieldBinding
    names.push_back(_tokens->meshLight_dep_volumeFieldBinding);
    sources.push_back(HdDependencySchema::Builder()
        .SetDependedOnPrimPath(originPathDS)
        .SetDependedOnDataSourceLocator(volumeFieldBindingDSL)
        .SetAffectedDataSourceLocator(volumeFieldBindingDSL)
        .Build());

    return HdRetainedContainerDataSource::New(
        names.size(), names.data(), sources.data());
}

HdContainerDataSourceHandle
_BuildSourceDataSource(
    const SdfPath& originPath,
    const HdContainerDataSourceHandle& originDS)
{
    std::vector<TfToken> names;
    std::vector<HdDataSourceBaseHandle> sources;

    const auto originPathDS = HdRetainedTypedSampledDataSource<SdfPath>
        ::New(originPath);

    // Add dependencies
    names.push_back(HdDependenciesSchemaTokens->__dependencies);
    sources.push_back(_BuildSourceDependenciesDataSource(originPath));

    // Knock out material
    names.push_back(HdMaterialSchemaTokens->material);
    sources.push_back(HdBlockDataSource::New());

    // Knock out light
    names.push_back(HdLightSchemaTokens->light);
    sources.push_back(HdBlockDataSource::New());

    // Knock out usdCollections
    names.push_back(_tokens->usdCollections);
    sources.push_back(HdBlockDataSource::New());

    // Knock out xform
    names.push_back(HdXformSchemaTokens->xform);
    sources.push_back(HdBlockDataSource::New());

    return HdOverlayContainerDataSource::New(
        HdRetainedContainerDataSource::New(
            names.size(), names.data(), sources.data()),
        originDS);
}

HdContainerDataSourceHandle
_BuildMeshDependenciesDataSource(
    const SdfPath originPath)
{
    // Data source locators

    // mesh
    static const HdLocatorDataSourceHandle meshDSL =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdMeshSchema::GetDefaultLocator());

    // primvars
    static const HdLocatorDataSourceHandle primvarsDSL =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdPrimvarsSchema::GetDefaultLocator());
    
    // material binding
    static const HdLocatorDataSourceHandle materialBindingDSL =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
#if HD_API_VERSION >= 51
            HdMaterialBindingsSchema::GetDefaultLocator());
#else
            HdMaterialBindingSchema::GetDefaultLocator());
#endif

    // visibility
    static const HdLocatorDataSourceHandle visibilityDSL =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdVisibilitySchema::GetDefaultLocator());
    
    // xform
    static const HdLocatorDataSourceHandle xformDSL =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdXformSchema::GetDefaultLocator());

    // instanced by
    static const HdLocatorDataSourceHandle instancedByDSL =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdInstancedBySchema::GetDefaultLocator());
    
    // volume field binding
    static const HdLocatorDataSourceHandle volumeFieldBindingDSL =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdVolumeFieldBindingSchema::GetDefaultLocator());

    // Build dependencies data source
    
    // Read "-->" in comments as "depends on"

    std::vector<TfToken> names;
    std::vector<HdDataSourceBaseHandle> sources;

    const auto originPathDS = HdRetainedTypedSampledDataSource<SdfPath>
        ::New(originPath);
    
    // mesh.mesh --> origin.mesh
    names.push_back(_tokens->meshLight_dep_mesh);
    sources.push_back(HdDependencySchema::Builder()
        .SetDependedOnPrimPath(originPathDS)
        .SetDependedOnDataSourceLocator(meshDSL)
        .SetAffectedDataSourceLocator(meshDSL)
        .Build());

    // mesh.primvars --> origin.primvars
    names.push_back(_tokens->meshLight_dep_primvars);
    sources.push_back(HdDependencySchema::Builder()
        .SetDependedOnPrimPath(originPathDS)
        .SetDependedOnDataSourceLocator(primvarsDSL)
        .SetAffectedDataSourceLocator(primvarsDSL)
        .Build());

    // mesh.materialBinding --> origin.materialBinding
    names.push_back(_tokens->meshLight_dep_materialBinding);
    sources.push_back(HdDependencySchema::Builder()
        .SetDependedOnPrimPath(originPathDS)
        .SetDependedOnDataSourceLocator(materialBindingDSL)
        .SetAffectedDataSourceLocator(materialBindingDSL)
        .Build());
    
    // mesh.visibility --> origin.visibility
    names.push_back(_tokens->meshLight_dep_visibility);
    sources.push_back(HdDependencySchema::Builder()
        .SetDependedOnPrimPath(originPathDS)
        .SetDependedOnDataSourceLocator(visibilityDSL)
        .SetAffectedDataSourceLocator(visibilityDSL)
        .Build());

    // mesh.xform --> origin.xform
    names.push_back(_tokens->meshLight_dep_xform);
    sources.push_back(HdDependencySchema::Builder()
        .SetDependedOnPrimPath(originPathDS)
        .SetDependedOnDataSourceLocator(xformDSL)
        .SetAffectedDataSourceLocator(xformDSL)
        .Build());

    // mesh.instancedBy --> origin.instancedBy
    names.push_back(_tokens->meshLight_dep_instancedBy);
    sources.push_back(HdDependencySchema::Builder()
        .SetDependedOnPrimPath(originPathDS)
        .SetDependedOnDataSourceLocator(instancedByDSL)
        .SetAffectedDataSourceLocator(instancedByDSL)
        .Build());
    
    // source.volumeFieldBinding --> origin.volumeFieldBinding
    names.push_back(_tokens->meshLight_dep_volumeFieldBinding);
    sources.push_back(HdDependencySchema::Builder()
        .SetDependedOnPrimPath(originPathDS)
        .SetDependedOnDataSourceLocator(volumeFieldBindingDSL)
        .SetAffectedDataSourceLocator(volumeFieldBindingDSL)
        .Build());

    return HdRetainedContainerDataSource::New(
        names.size(), names.data(), sources.data());
}

} // anonymous namespace

/* static */
HdPrmanMeshLightResolvingSceneIndexRefPtr
HdPrmanMeshLightResolvingSceneIndex::New(
    const HdSceneIndexBaseRefPtr& inputSceneIndex)
{
    return TfCreateRefPtr(
        new HdPrmanMeshLightResolvingSceneIndex(
            inputSceneIndex));
}

HdPrmanMeshLightResolvingSceneIndex::HdPrmanMeshLightResolvingSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
    : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{ }

HdSceneIndexPrim 
HdPrmanMeshLightResolvingSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    // The origin prim -> no primType (should only be visible to HSB)
    if (_meshLights.count(primPath)) {
        return {
            TfToken(),
            _GetInputSceneIndex()->GetPrim(primPath).dataSource
        };
    }

    const SdfPath& parentPath = primPath.GetParentPath();

    if (_meshLights.count(parentPath) > 0) {
        const HdSceneIndexPrim& parentPrim = _GetInputSceneIndex()
            ->GetPrim(parentPath);

        // The stripped-down origin prim -> "mesh" or "volume"
        if (primPath.GetNameToken() == _tokens->meshLightMeshName
            && _meshLights.at(parentPath)) {
            return {
                parentPrim.primType,
                HdOverlayContainerDataSource::New(
                    HdRetainedContainerDataSource::New(
                        HdLightSchemaTokens->light, HdBlockDataSource::New(),
                        HdMaterialSchemaTokens->material, HdBlockDataSource::New(),
                        _tokens->usdCollections, HdBlockDataSource::New(),
                        HdDependenciesSchemaTokens->__dependencies,
                        _BuildMeshDependenciesDataSource(parentPath)),
                    parentPrim.dataSource)
            };
        }

        // The light prim -> "meshLight"
        if (primPath.GetNameToken() == _tokens->meshLightLightName) {
            return {
                HdPrimTypeTokens->meshLight,
                _BuildLightDataSource(
                    parentPath, parentPrim,
                    parentPath, parentPrim.dataSource, // materialBinding source
                    _GetInputSceneIndex())
            };
        }

        // The source mesh -> "meshLightSourceMesh" or "meshLightSourceVolume"
        if (primPath.GetNameToken() == _tokens->meshLightSourceName) {
            return {
                parentPrim.primType == HdPrimTypeTokens->volume
                  ? HdPrmanTokens->meshLightSourceVolume
                  : HdPrmanTokens->meshLightSourceMesh,
                _BuildSourceDataSource(parentPath, parentPrim.dataSource)
            };
        }
    }

    return _GetInputSceneIndex()->GetPrim(primPath);
}

SdfPathVector 
HdPrmanMeshLightResolvingSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    SdfPathVector paths = _GetInputSceneIndex()->GetChildPrimPaths(primPath);

    if (_meshLights.count(primPath)) {
        paths.push_back(primPath.AppendChild(_tokens->meshLightLightName));
        paths.push_back(primPath.AppendChild(_tokens->meshLightSourceName));
        if (_meshLights.at(primPath)) {
            paths.push_back(primPath.AppendChild(_tokens->meshLightMeshName));
        }
    }
    return paths;
}

void
HdPrmanMeshLightResolvingSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{    
    if (!_IsObserved()) {
        return;
    }
    
    TRACE_FUNCTION();

    HdSceneIndexObserver::AddedPrimEntries added;

    for (const auto& entry : entries) {
        if ((entry.primType == HdPrimTypeTokens->mesh) ||
            (entry.primType == HdPrimTypeTokens->volume)) {
            HdSceneIndexPrim prim = _GetInputSceneIndex()->
                GetPrim(entry.primPath);
            
            // The prim is a mesh light if light.isLight is true. But a mesh
            // light also needs a valid light shader network [material
            // resource], which it won't have when stage scene index is not
            // enabled.
            //
            // Mesh lights are not supported without stage scene index;
            // we should not insert the light, source, or stripped-down mesh
            // unless it is enabled. If it is disabled, we should instead
            // just forward the origin prim along unmodified at its original
            // path; downstream HdPrman will treat it as the mesh its prim type
            // declares it to be.

            if (_IsMeshLight(prim) && _HasValidMaterialNetwork(prim)) {
                const bool meshVisible = _GetMaterialSyncMode(prim.dataSource)
                    != UsdLuxTokens->noMaterialResponse;
                _meshLights.insert({ entry.primPath, meshVisible });

                // The light prim
                added.emplace_back(
                    entry.primPath.AppendChild(_tokens->meshLightLightName),
                    HdPrimTypeTokens->meshLight);
                
                // The source mesh (for the light prim)

                added.emplace_back(
                    entry.primPath.AppendChild(_tokens->meshLightSourceName),
                    entry.primType == HdPrimTypeTokens->volume
                      ? HdPrmanTokens->meshLightSourceVolume
                      : HdPrmanTokens->meshLightSourceMesh);

                // The stripped-down origin prim
                if (meshVisible) {
                    added.emplace_back(
                        entry.primPath.AppendChild(_tokens->meshLightMeshName),
                        entry.primType);
                }

                // skip fallback insertion
                continue;
            }
        }
        added.push_back(entry);
    }
    _SendPrimsAdded(added);
}

void 
HdPrmanMeshLightResolvingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{

    HdSceneIndexObserver::RemovedPrimEntries removed;

    for (const auto& entry : entries) {
        if (_meshLights.count(entry.primPath)) {

            // The light prim
            removed.emplace_back(
                entry.primPath.AppendChild(_tokens->meshLightLightName));

            // The source mesh (for the light prim)
            removed.emplace_back(
                entry.primPath.AppendChild(_tokens->meshLightSourceName));

            // The stripped-down origin prim
            if (_meshLights.at(entry.primPath)) {
                removed.emplace_back(
                    entry.primPath.AppendChild(_tokens->meshLightMeshName));
            }

            _meshLights.erase(entry.primPath);

            // skip fallback removal
            continue;
        }
        removed.push_back(entry);
    }

    _SendPrimsRemoved(removed);
}

void
HdPrmanMeshLightResolvingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    // Dependency Forwarding Scene Index will take care of most everything, but
    // we do still need to add/remove the stripped-down origin prim when
    // materialSyncMode changes from/to noMaterialResponse.

    static const HdDataSourceLocator materialSyncModeLocator = { 
        HdLightSchemaTokens->light,
        HdTokens->materialSyncMode
    };

    for (const auto& entry : entries) {
        if (_meshLights.count(entry.primPath)
            && entry.dirtyLocators.Contains(materialSyncModeLocator)) {
            const HdSceneIndexPrim& prim = _GetInputSceneIndex()
                ->GetPrim(entry.primPath);
            const bool visible = _GetMaterialSyncMode(prim.dataSource)
                != UsdLuxTokens->noMaterialResponse;
            if (visible && (!_meshLights.at(entry.primPath))) {
                // materialSyncMode is no longer noMaterialResponse; insert
                _meshLights[entry.primPath] = visible;
                _SendPrimsAdded({{ 
                    entry.primPath.AppendChild(_tokens->meshLightMeshName),
                    prim.primType
                }});
            } else if ((!visible) && _meshLights.at(entry.primPath)) {
                // materialSyncMode changed to noMaterialResponse; remove
                _meshLights[entry.primPath] = visible;
                _SendPrimsRemoved({{ 
                    entry.primPath.AppendChild(_tokens->meshLightMeshName)
                }});
            }
        }
    }
    _SendPrimsDirtied(entries);
}

HdPrmanMeshLightResolvingSceneIndex::~HdPrmanMeshLightResolvingSceneIndex()
    = default;

PXR_NAMESPACE_CLOSE_SCOPE
