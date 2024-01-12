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
#include "hdPrman/light.h"

#include "hdPrman/debugCodes.h"
#include "hdPrman/instancer.h"
#include "hdPrman/lightFilter.h"
#include "hdPrman/material.h"
#include "hdPrman/mesh.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/rixStrings.h"
#include "hdPrman/tokens.h"
#include "hdPrman/utils.h"

#include "pxr/base/arch/library.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hf/diagnostic.h"

#include "RiTypesHelper.h"
#include "RixShadingUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

HdPrmanLight::HdPrmanLight(SdfPath const& id, TfToken const& lightType)
    : HdLight(id)
    , _hdLightType(lightType)
{
    /* NOTHING */
}

HdPrmanLight::~HdPrmanLight() = default;

void
HdPrmanLight::Finalize(HdRenderParam *renderParam)
{
    HdPrman_RenderParam *param =
        static_cast<HdPrman_RenderParam*>(renderParam);
    riley::Riley* riley = param->AcquireRiley();
    if (!_lightLink.IsEmpty()) {
        param->DecrementLightLinkCount(_lightLink);
        _lightLink = TfToken();
    }
    if (!_lightFilterPaths.empty()) {
        _lightFilterPaths.clear();
    }
    if (!_lightFilterLinks.empty()) {
        for (const TfToken& filterLink : _lightFilterLinks) {
            param->DecrementLightFilterCount(filterLink);
        }
        _lightFilterLinks.clear();
    }

    // delete instances owned by the instancer.
    if (HdPrmanInstancer* instancer = param->GetInstancer(
        GetInstancerId())) {
        instancer->Depopulate(renderParam, GetId());
    }
    
    if (_instanceId != riley::LightInstanceId::InvalidId()) {
        TRACE_SCOPE("riley::DeleteLightInstance");
        riley->DeleteLightInstance(riley::GeometryPrototypeId::InvalidId(),
            _instanceId);
        _instanceId = riley::LightInstanceId::InvalidId();
    }
    if (_shaderId != riley::LightShaderId::InvalidId()) {
        TRACE_SCOPE("riley::DeleteLightShader");
        riley->DeleteLightShader(_shaderId);
        _shaderId = riley::LightShaderId::InvalidId();
    }
    _lightShaderType = RtUString();
    _geometryPrototypeId = riley::GeometryPrototypeId::InvalidId();
    _sourceGeomPath = SdfPath();
    _shadowLink = TfToken();
}

static bool
_PopulateNodesFromMaterialResource(HdSceneDelegate *sceneDelegate,
                                   const SdfPath &id,
                                   const TfToken &terminalName,
                                   std::vector<riley::ShadingNode> *result)
{
    VtValue hdMatVal = sceneDelegate->GetMaterialResource(id);
    if (!hdMatVal.IsHolding<HdMaterialNetworkMap>()) {
        TF_WARN("Could not get HdMaterialNetworkMap for '%s'", id.GetText());
        return false;
    }

    // Convert HdMaterial to HdMaterialNetwork2 form.
    const HdMaterialNetwork2 matNetwork2 = HdConvertToHdMaterialNetwork2(
            hdMatVal.UncheckedGet<HdMaterialNetworkMap>());

    SdfPath nodePath;
    for (auto const& terminal: matNetwork2.terminals) {
        if (terminal.first == terminalName) {
            nodePath = terminal.second.upstreamNode;
            break;
        }
    }

    if (nodePath.IsEmpty()) {
        TF_WARN("Could not find terminal '%s' in HdMaterialNetworkMap for '%s'",
                terminalName.GetText(), id.GetText());
        return false;
    }

    result->reserve(matNetwork2.nodes.size());
    if (!HdPrman_ConvertHdMaterialNetwork2ToRmanNodes(
            matNetwork2, nodePath, result)) {
        TF_WARN("Failed to convert HdMaterialNetwork to Renderman shading "
                "nodes for '%s'", id.GetText());
        return false;
    }

    return true;
}

static void
_AddLightFilterCombiner(std::vector<riley::ShadingNode>* lightFilterNodes)
{
    static RtUString combineMode("combineMode");
    static RtUString mult("mult");
    static RtUString max("max");
    static RtUString min("min");
    static RtUString screen("screen");

    riley::ShadingNode combiner = riley::ShadingNode {
        riley::ShadingNode::Type::k_LightFilter,
        RtUString("PxrCombinerLightFilter"),
        RtUString("terminal.Lightfilter"),
        RtParamList()
    };

    // Build a map of light filter handles grouped by mode.
    std::unordered_map<RtUString, std::vector<RtUString>> modeMap;

    for (const auto& lightFilterNode : *lightFilterNodes) {       
        RtUString mode;
        lightFilterNode.params.GetString(combineMode, mode);
        if (mode.Empty()) {
            modeMap[mult].push_back(lightFilterNode.handle);
        } 
        else {
            modeMap[mode].push_back(lightFilterNode.handle);
        }
    }

    // Set the combiner light filter reference array for each mode.
    for (const auto& entry : modeMap) {
        if (!entry.second.empty()) {
            combiner.params.SetLightFilterReferenceArray(
                entry.first, &entry.second[0], entry.second.size());
        }
    }

    lightFilterNodes->push_back(combiner);
}

static void
_PopulateLightFilterNodes(
        const SdfPath &lightId,
        const SdfPathVector &lightFilterPaths,
        HdSceneDelegate *sceneDelegate,
        HdRenderParam *renderParam,
        riley::Riley *riley,
        std::vector<riley::ShadingNode> *lightFilterNodes,
        std::vector<riley::CoordinateSystemId> *coordsysIds,
        std::vector<TfToken> *lightFilterLinks)
{
    HdPrman_RenderParam * const param =
        static_cast<HdPrman_RenderParam*>(renderParam);

    if (lightFilterPaths.empty()) {
        return;
    }

    int maxFilters = lightFilterPaths.size();
    if (maxFilters > 1) {
        maxFilters += 1;  // extra for the combiner filter
    }
    lightFilterNodes->reserve(maxFilters);
    
    for (const auto& filterPath : lightFilterPaths) {
        TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
            .Msg("HdPrman: Light <%s> filter \"%s\" path \"%s\"\n",
                lightId.GetText(), filterPath.GetName().c_str(),
                filterPath.GetText());

        if (!sceneDelegate->GetVisible(filterPath)) {
            // XXX -- need to get a dependency analysis working here
            // Invis of a filter works but does not cause the light
            // to re-sync so one has to tweak the light to see the
            // effect of the invised filter
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("  filter invisible\n");
            continue;
        }

        if (!_PopulateNodesFromMaterialResource(
                sceneDelegate, filterPath,
                HdMaterialTerminalTokens->lightFilter, 
                lightFilterNodes)) {
            continue;
        }

        riley::ShadingNode *filter = &lightFilterNodes->back();
        RtUString filterPathAsString = RtUString(filterPath.GetText());

        // To ensure that multiple light filters within a light get
        // unique names, use the full filter path for the handle.
        filter->handle = filterPathAsString;

        // Only certain light filters require a coordsys, but we do not
        // know which, here, so we provide it in all cases.
        //
        // TODO: We should be able to look up the SdrShaderNode entry
        // and query it for the existence of this parameter.
        filter->params.SetString(RtUString("coordsys"), filterPathAsString);

        // Light filter linking
        VtValue val = sceneDelegate->GetLightParamValue(filterPath,
                                        HdTokens->lightFilterLink);
        TfToken lightFilterLink = TfToken();
        if (val.IsHolding<TfToken>()) {
            lightFilterLink = val.UncheckedGet<TfToken>();
        }
        if (!lightFilterLink.IsEmpty()) {
            param->IncrementLightFilterCount(lightFilterLink);
            lightFilterLinks->push_back(lightFilterLink);
            // For light filters to link geometry, the light filters must
            // be assigned a grouping membership, and the
            // geometry must subscribe to that grouping.
            filter->params.SetString(RtUString("linkingGroups"),
                                RtUString(lightFilterLink.GetText()));
            TF_DEBUG(HDPRMAN_LIGHT_LINKING)
                .Msg("HdPrman: Light filter <%s> linkingGroups \"%s\"\n",
                        filterPath.GetText(), lightFilterLink.GetText());
        }

        // Look up light filter ID
        if (HdSprim *sprim = sceneDelegate->GetRenderIndex().GetSprim(
            HdPrimTypeTokens->lightFilter, filterPath)) {
            if (HdPrmanLightFilter *lightFilter =
                dynamic_cast<HdPrmanLightFilter*>(sprim)) {
                lightFilter->SyncToRiley(sceneDelegate, riley);
                coordsysIds->push_back(lightFilter->GetCoordSysId());
            }
        } else {
            TF_WARN("Did not find expected light filter <%s>",
                filterPath.GetText());
        }
    }

    // Multiple filters requires a PxrCombinerLightFilter to combine results.
    if (lightFilterNodes->size() > 1) {
        _AddLightFilterCombiner(lightFilterNodes);
    }
}

/* virtual */
void
HdPrmanLight::Sync(HdSceneDelegate *sceneDelegate,
                   HdRenderParam   *renderParam,
                   HdDirtyBits     *dirtyBits)
{
    HD_TRACE_FUNCTION();

    static const RtUString us_PxrDomeLight("PxrDomeLight");
    static const RtUString us_PxrRectLight("PxrRectLight");
    static const RtUString us_PxrDiskLight("PxrDiskLight");
    static const RtUString us_PxrCylinderLight("PxrCylinderLight");
    static const RtUString us_PxrSphereLight("PxrSphereLight");
    static const RtUString us_PxrDistantLight("PxrDistantLight");
    static const RtUString us_PxrMeshLight("PxrMeshLight");
    static const RtUString us_PxrPortalLight("PxrPortalLight");
    static const RtUString us_shadowSubset("shadowSubset");
    static const riley::MaterialId invalidMaterial;

    const SdfPath& id = GetId();

    HdPrman_RenderParam* param =static_cast<HdPrman_RenderParam*>(renderParam);
    riley::Riley* riley = param->AcquireRiley();
    HdRenderIndex& renderIndex = sceneDelegate->GetRenderIndex();
    HdChangeTracker& changeTracker = renderIndex.GetChangeTracker();

    // Light shader nodes will go here, whether we calculate them early during
    // change tracking or later during shader update.
    std::vector<riley::ShadingNode> lightNodes;
    // Any coordinate system ID's used will go here.
    std::vector<riley::CoordinateSystemId> coordSysIds;

    // Update instance bindings
    // XXX: This relies on DirtyInstancer having the same value for lights as
    // it does for rprims. It is the only flag that _UpdateInstancer cares about
    _UpdateInstancer(sceneDelegate, dirtyBits);

    const SdfPath& instancerId = GetInstancerId();
    const bool isHdInstance = !instancerId.IsEmpty();
    SdfPath primPath = sceneDelegate->GetScenePrimPath(id, 0, nullptr);

    // XXX: The following dirtiness detection, and all of the state we have to
    // maintain to do it, is necessary so that we can use
    // the riley api for updating existing light shaders/instances, which is
    // in turn needed so we can split ownership of shaders and instances between
    // HdPrmanLight and HdPrmanInstancer when lights are instanced. The existing
    // DirtyBits mechanism for lights is inadequate for tracking changes to the
    // light shader separately from changes to the light instance.
    // Note that we do not update our saved state while checking for dirtiness.
    // We'll do that later, when we're actually updating the light in Riley.

    // The source geom of a geom light has changed in a way that requires us to
    // destroy and recreate the light instance rather than use
    // riley->ModifyLightInstance(). Specifically, the riley geometry prototype
    // id has changed. We don't expect this ever to happen, since a gprim never
    // changes its prototype id, but it could happen if a scene index downstream
    // of the mesh light resolving scene index were to alter the source geometry
    // binding on the mesh light.
    bool dirtySourceGeom = false;

    // Something changed that invalidates the riley light shader. This could be
    // DirtyResource, dirty shadow params (which may not be accurately signalled
    // by DirtyShadowParams), or dirty light filters.
    bool dirtyLightShader = false;

    // Something changed that invalidates the riley light instance(s) in a way
    // that we can update with ModifyLightInstance. This could be the transform,
    // the coordinate system list, or certain attributes.
    bool dirtyLightInstance = false;

    if (_hdLightType == HdSprimTypeTokens->meshLight) {
        // Has source geom changed? Is the rprim still there?
        const VtValue sourceGeom = sceneDelegate->GetLightParamValue(id, 
            HdPrmanTokens->sourceGeom);
        if (sourceGeom.IsHolding<SdfPath>()) {
            const SdfPath sourceGeomPath = sourceGeom.UncheckedGet<SdfPath>();
            if (sourceGeomPath != _sourceGeomPath) {
                // source geom path has changed; assume new prototype id
                dirtySourceGeom = true;
            }
            const HdRprim *rprim = 
                sceneDelegate->GetRenderIndex().GetRprim(sourceGeomPath);
            if (rprim) {

                // XXX: Temporary workaround for RMAN-20136
                // Check if the source mesh is scheduled for a prototype
                // update. If it is, we need to postpone sync in order to avoid
                // an issue in Prman with simultaneous light shader and
                // geometry prototype modifications on mesh lights.
                // See https://jira.pixar.com/browse/RMAN-20136. Delaying here
                // has the effect of adding an extra call to riley->Render()
                // between the geometry prototype update and the light shader
                // update, which has a noticeable effect on performance, so we
                // will also check to see whether the light shader already
                // exists. If it does not, it should be safe to make it even
                // when we have a dirty source mesh geometry.
                HdDirtyBits sourceDirtyBits = changeTracker
                    .GetRprimDirtyBits(sourceGeomPath);
                if (_shaderId != riley::LightShaderId::InvalidId() && (
                        sourceDirtyBits & (
                        HdChangeTracker::DirtyPoints |
                        HdChangeTracker::DirtyNormals |
                        HdChangeTracker::DirtyWidths |
                        HdChangeTracker::DirtyTopology))) {
                    TF_DEBUG(HDPRMAN_MESHLIGHT).Msg("Source geom <%s> for geom "
                        "light <%s> has dirty geometry; postponing sync.\n",
                        sourceGeomPath.GetText(), id.GetText());
                    // Note that we cannot just resync the source mesh here.
                    // We have to separate updates to the geometry protoype from
                    // updates to the light shader by a call to Render, and the
                    // only way to achieve that is to return while the light is
                    // still dirty. This will be a challenging bug to work
                    // around in HdPrman 2.0.
                    return;
                }
                // XXX: End of RMAN-20136 workaround

                // check if the prototype id exists, or has changed
                auto gprim = dynamic_cast<const HdPrman_GprimBase*>(rprim);
                std::vector<riley::GeometryPrototypeId> prototypeIds = 
                    gprim->GetPrototypeIds();
                if (prototypeIds.empty()) {
                    // XXX: This is our least-ugly workaround for sync ordering.
                    // We do not expect the source geometry's prototype id to
                    // change under any normal circumstances, so this only needs
                    // to happen during the initial sync phase. We reach out to
                    // the unsynced source geometry and sync it. This is not a
                    // general solution to sync ordering. It should not be
                    // replicated elsewhere!
                    TF_DEBUG(HDPRMAN_MESHLIGHT).Msg("Attempting to sync source "
                        "geometry <%s> for geom light <%s>\n",
                        sourceGeomPath.GetText(), id.GetText());
                    const_cast<HdRprim*>(rprim)->Sync(
                        sceneDelegate, renderParam, &sourceDirtyBits, TfToken());
                    prototypeIds = gprim->GetPrototypeIds();
                }
                if (prototypeIds.empty()) {
                    // sync failed to produce riley prototype ids; ignore light
                    TF_DEBUG(HDPRMAN_MESHLIGHT).Msg("Source geometry <%s> for "
                        "geom light <%s> still has not been created in riley; "
                        "the light will be ignored.\n",
                        sourceGeomPath.GetText(), id.GetText());
                    *dirtyBits = DirtyBits::Clean;
                    return;
                } else if (prototypeIds.size() > 1) {
                    // XXX: Geom subsets are not yet supported on geom lights;
                    // the mesh light resolving scene index should strip them
                    // out of the source geom. If we hit this, something odd
                    // is going on.
                    TF_DEBUG(HDPRMAN_MESHLIGHT).Msg("Source geom <%s> for "
                        "geom light <%s> has more than one geometry "
                        "prototype id; only one is expected, and only the "
                        "first will be used.\n", sourceGeomPath.GetText(),
                        id.GetText());
                }
                if (prototypeIds[0] != _geometryPrototypeId) {
                    // source geom prototype id has changed
                    // XXX: Again, we do not expect this. The source geometry
                    // won't change its prototype id because of how gprim sync
                    // behaves, and a given geom light's source geometry path
                    // should never change because of how the mesh light
                    // resolving scene index behaves. The checks are here to
                    // protect against prman crashes in the event that a scene
                    // index between the mesh light resolving scene index and
                    // here messes with things.
                    dirtySourceGeom = true;
                }
            } else {
                // cannot find the source geom in the render index; ignore light
                TF_DEBUG(HDPRMAN_MESHLIGHT).Msg("Source geom <%s> for geom "
                    "light <%s> could not be found; the light will be "
                    "ignored.\n", sourceGeomPath.GetText(), id.GetText());
                *dirtyBits = DirtyBits::Clean;
                return;
            }
        } else {
            // light.sourceGeom was empty, which would indicate a breakdown in
            // the mesh light resolving scene index.
            TF_DEBUG(HDPRMAN_MESHLIGHT).Msg("Geom light <%s> has no source "
                "geometry; this light will be ignored since source geometry is "
                "required.", id.GetText());
            *dirtyBits = DirtyBits::Clean;
            return;
        }
    }

    if ((*dirtyBits & DirtyResource)
        || _shaderId == riley::LightShaderId::InvalidId()) {
        // The light shader has changed
        dirtyLightShader = true;
        if (_hdLightType == HdSprimTypeTokens->pluginLight) {
            // The material resource [light shader] of a plugin light will
            // change when the id [name] of the specific shader the light uses
            // has changed. Which shader the light is using can affect how
            // input parameters are interpreted to affect the light's transform,
            // which is instance-invalidating. We keep the name as state so we
            // can detect when we must also invalidate the instance. But we also
            // want to avoid calling _PopulateNodesFromMaterialResource more
            // than necessary.
            if (_lightShaderType.Empty()) {
                // Empty state means we've never seen the shader at all, so the
                // instance will also be dirty.
                dirtyLightInstance = true;
            } else {
                // Early call to _PopulateNodesFromMaterialResource
                _PopulateNodesFromMaterialResource(
                    sceneDelegate, id,
                    HdMaterialTerminalTokens->light, &lightNodes);
                if ((!lightNodes.empty())
                    && lightNodes.back().name != _lightShaderType) {
                    dirtyLightInstance = true;
                }
            }
        }
    }

    if (*dirtyBits & (DirtyTransform | DirtyInstancer)) {
        // If the transform has changed or the instancer is dirty, the light
        // instance (or instances, in the latter case) needs to be refreshed.
        dirtyLightInstance = true;
    }
    
    if (*dirtyBits & (DirtyParams | DirtyShadowParams | DirtyCollection)) {
        // Light linking changes are subsumed under changes to the light api,
        // which are in turn signalled as :
        //   (DirtyParams | DirtyShadowParams | DirtyCollection)
        // We can store lightLink locally and compare against that to see if
        // light links specifically have changed. Light links affect the light's
        // instance attributes, and do not invalidate the shader!
        VtValue val = sceneDelegate->GetLightParamValue(id, HdTokens->lightLink);
        if (val.IsHolding<TfToken>()) {
            TfToken lightLink = val.UncheckedGet<TfToken>();
            if (lightLink != _lightLink) {
                // lightLink has changed
                dirtyLightInstance = true;
            }
        } else if (!_lightLink.IsEmpty()) {
            // lightLink was not empty before, but is now
            dirtyLightInstance = true;
        }

        // Light filter changes are also subsumed under changes to the light
        // api and signalled the same as light filters. Again, compare against a
        // local copy to see if they've really changed. Changes to light filters
        // affect both the shader and the instance, the latter due to potential
        // changes in the relevant coordinate systems.
        val = sceneDelegate->GetLightParamValue(id, HdTokens->filters);
        if (val.IsHolding<SdfPathVector>()) {
            SdfPathVector lightFilterPaths = val.UncheckedGet<SdfPathVector>();
            if (lightFilterPaths != _lightFilterPaths) {
                // light filter paths have changed
                dirtyLightShader = true;
                dirtyLightInstance = true;
            } else {
                // TODO: check if the filters themselves have changed?
            }
        } else if (!_lightFilterPaths.empty()) {
            // light filter paths were not empty before, but are now
            dirtyLightShader = true;
            dirtyLightInstance = true;
        }

        // DirtyShadowParams may be set even if the shadow params did not
        // change, due to a lack of granularity in the dirty bits translator.
        // So we will manually check the shadow params against a local copy
        // to see if they really changed. Changes to the shadow params
        // invalidate the light shader.
        val = sceneDelegate->GetLightParamValue(id, HdTokens->shadowLink);
        if (val.IsHolding<TfToken>()) {
            TfToken shadowLink = val.UncheckedGet<TfToken>();
            if (shadowLink != _shadowLink) {
                // shadowLink has changed
                dirtyLightShader = true;
            }
        } else if (!_shadowLink.IsEmpty()) {
            dirtyLightShader = true;
        }

        // DirtyParams will always dirty the instance
        if (*dirtyBits & DirtyParams) {
            dirtyLightInstance = true;
        }
    }

    // finally, dirtySourceGeom implies dirtyLightInstance;
    dirtyLightInstance |= dirtySourceGeom;

    // Now that we know what's actually dirty (the shader and/or the instance),
    // we can proceed with a modify-aware approach.

    if (dirtyLightShader) {
        // prepare and create or modify the light shader(s).

        // Only call _PopulateNodesFromMaterialResource if we did not call it
        // above during dirty checking.
        if (lightNodes.empty()) {
            _PopulateNodesFromMaterialResource(
                sceneDelegate, id,
                HdMaterialTerminalTokens->light, &lightNodes);
        }

        if (lightNodes.empty() || lightNodes.back().name.Empty()) {
            TF_WARN("Could not populate shading nodes for light <%s>. "
            "The light will be ignored.", id.GetText());
            *dirtyBits = DirtyBits::Clean;
            return;
        }

        _lightShaderType = lightNodes.back().name;

        TF_DEBUG(HDPRMAN_LIGHT_LIST).Msg("HdPrman: Light <%s> lightType '%s', "
            "shader '%s'\n",
            id.GetText(), _hdLightType.GetText(), _lightShaderType.CStr());
        // The terminal light node will be updated with other parameters that
        // aren't direct inputs of the material resource.
        riley::ShadingNode& lightNode = lightNodes.back();

        // Shadow linking
        VtValue shadowLinkVal =
            sceneDelegate->GetLightParamValue(id, HdTokens->shadowLink);
        if (shadowLinkVal.IsHolding<TfToken>()) {
            _shadowLink = shadowLinkVal.UncheckedGet<TfToken>();
            if (!_shadowLink.IsEmpty()) {
                lightNode.params.SetString(us_shadowSubset,
                    RtUString(_shadowLink.GetText()));
                TF_DEBUG(HDPRMAN_LIGHT_LINKING).Msg("HdPrman: Light <%s> "
                    "shadowSubset '%s'\n", id.GetText(),
                    _shadowLink.GetText());
            }
        }

        SdfPathVector filters;
        VtValue filtersVal = sceneDelegate->GetLightParamValue(
            id, HdTokens->filters);
        if (filtersVal.IsHolding<SdfPathVector>()) {
            filters = filtersVal.UncheckedGet<SdfPathVector>();
        }
        if (filters != _lightFilterPaths) {
            // clear and recreate dependencies
            for (const SdfPath& filterPath : _lightFilterPaths) {
                changeTracker.RemoveSprimSprimDependency(filterPath, id);
            }
            for (const SdfPath& filterPath : filters) {
                changeTracker.AddSprimSprimDependency(filterPath, id);
            }
            _lightFilterPaths = filters;
        }
        // Light filter counts get incremented when we call
        // _PopulateLightFilterNodes, so we don't get the opportunity
        // to really compare them against state. State here exists so
        // we can decrement the old filter counts before building
        // the filter network.
        for (const TfToken& filterLink : _lightFilterLinks) {
            param->DecrementLightFilterCount(filterLink);
        }
        std::vector<riley::ShadingNode> filterNodes;

        // _PopulateLightFilterNodes also gives us the coordinate systems.
        // We store them so we can have them on later calls where only the
        // light instance is dirty. Note above that dirty light filters mean
        // dirty shader *and* dirty instance; the coordinate systems are why,
        // and are the only piece of derived state that needs to be shared by
        // both the shader and instance update branches.
        _PopulateLightFilterNodes(id, filters, sceneDelegate, renderParam,
            riley, &filterNodes, &coordSysIds, &_lightFilterLinks);

        const riley::ShadingNetwork light {
            static_cast<uint32_t>(lightNodes.size()),
            lightNodes.data()
        };

        const riley::ShadingNetwork filter {
            static_cast<uint32_t>(filterNodes.size()),
            filterNodes.data()
        };

        // TODO: portals
        
        if (_shaderId == riley::LightShaderId::InvalidId()) {
            const riley::UserId userId(
                stats::AddDataLocation(id.GetText()).GetValue());
            TRACE_SCOPE("riley::CreateLightShader");
            _shaderId = riley->CreateLightShader(userId, light, filter);
        } else {
            TRACE_SCOPE("riley::ModifyLightShader");
            riley->ModifyLightShader(_shaderId, &light, &filter);
        }
    }

    if (dirtyLightInstance) {
    
        riley::MaterialId materialId;
        if (_hdLightType == HdSprimTypeTokens->meshLight) {
            // Checks that these exist have already been done above!

            const SdfPath sourceGeomPath = sceneDelegate->GetLightParamValue(
                id, HdPrmanTokens->sourceGeom).UncheckedGet<SdfPath>();
            const HdRprim* rPrim = sceneDelegate->GetRenderIndex()
                .GetRprim(sourceGeomPath);
            const auto* gprim = dynamic_cast<const HdPrman_GprimBase*>(rPrim);
            
            _geometryPrototypeId = gprim->GetPrototypeIds()[0];
            _sourceGeomPath = sourceGeomPath;
        }

        RtParamList attrs = param->ConvertAttributes(sceneDelegate, id, false);
        if (_lightShaderType == us_PxrDomeLight) {
            const bool domeLightCamVis = sceneDelegate->GetRenderIndex()
                .GetRenderDelegate()->GetRenderSetting<bool>(
                    HdRenderSettingsTokens->domeLightCameraVisibility, true);
            if (!domeLightCamVis) {
                attrs.SetInteger(RixStr.k_visibility_camera, 0);
            }
        }

        TfToken lightLink;
        VtValue lightLinkVal = sceneDelegate->GetLightParamValue(
            id, HdTokens->lightLink);
        if (lightLinkVal.IsHolding<TfToken>()) {
            lightLink = lightLinkVal.UncheckedGet<TfToken>();
        }
        if (lightLink != _lightLink) {
            if (!_lightLink.IsEmpty()) {
                param->DecrementLightLinkCount(_lightLink);
            }
            if (!lightLink.IsEmpty()) {
                param->IncrementLightLinkCount(lightLink);
            }
            _lightLink = lightLink;
        }
        if (!_lightLink.IsEmpty()) {
            // For lights to link geometry, the lights must be assigned a
            // grouping membership and the geometry must subscribe to that
            // grouping.
            attrs.SetString(RixStr.k_grouping_membership,
                RtUString(_lightLink.GetText()));
            TF_DEBUG(HDPRMAN_LIGHT_LINKING).Msg("HdPrman: Light <%s> grouping "
                "membership '%s'\n", id.GetText(), _lightLink.GetText());
        } else {
            // Default light group
            attrs.SetString(RixStr.k_grouping_membership, RtUString("default"));
            TF_DEBUG(HDPRMAN_LIGHT_LINKING).Msg("HdPrman: Light <%s> grouping "
                "membership 'default'\n", id.GetText());
        }

        // Convert coordinate system ids to list
        const riley::CoordinateSystemList coordSysList = {
            unsigned(coordSysIds.size()), coordSysIds.data()
        };

        // Sample transform
        HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES> xf;
        sceneDelegate->SampleTransform(id, &xf);

        GfMatrix4d geomMat(1.0);

        // Some lights have parameters that scale the size of the light.
        GfVec3d geomScale(1.0f);

        // Type-specific parameters
        if (_lightShaderType == us_PxrRectLight ||
            _lightShaderType == us_PxrPortalLight) {
            // width
            VtValue width = sceneDelegate->GetLightParamValue(id,
                HdLightTokens->width);
            if (width.IsHolding<float>()) {
                geomScale[0] = width.UncheckedGet<float>();
            }
            // height
            VtValue height = sceneDelegate->GetLightParamValue(id,
                HdLightTokens->height);
            if (height.IsHolding<float>()) {
                geomScale[1] = height.UncheckedGet<float>();
            }
        } else if (_lightShaderType == us_PxrDiskLight) {
            // radius (XY only, default 0.5)
            VtValue radius = sceneDelegate->GetLightParamValue(id,
                HdLightTokens->radius);
            if (radius.IsHolding<float>()) {
                geomScale[0] *= radius.UncheckedGet<float>() / 0.5;
                geomScale[1] *= radius.UncheckedGet<float>() / 0.5;
            }
        } else if (_lightShaderType == us_PxrCylinderLight) {
            // radius (YZ only, default 0.5)
            VtValue radius = sceneDelegate->GetLightParamValue(id,
                HdLightTokens->radius);
            if (radius.IsHolding<float>()) {
                geomScale[1] *= radius.UncheckedGet<float>() / 0.5;
                geomScale[2] *= radius.UncheckedGet<float>() / 0.5;
            }
            // length (X-axis)
            VtValue length = sceneDelegate->GetLightParamValue(id,
                HdLightTokens->length);
            if (length.IsHolding<float>()) {
                geomScale[0] *= length.UncheckedGet<float>();
            }
        } else if (_lightShaderType == us_PxrSphereLight) {
            // radius (XYZ, default 0.5)
            VtValue radius = sceneDelegate->GetLightParamValue(id, 
                HdLightTokens->radius);
            if (radius.IsHolding<float>()) {
                geomScale *= radius.UncheckedGet<float>() / 0.5;
            }
        } else if (_lightShaderType == us_PxrMeshLight) {
            // Our mesh light geom should not be visible, and should be one-sided, 
            // to match the existing Katana behavior.
            // XXX: these may not be effective for volumes, either at all or
            // for certain path tracers. Volume light support is still incomplete.
            // XXX: These will overwrite and ignore what may be authored on the
            // mesh light, which may not be desirable.
            attrs.SetInteger(RixStr.k_visibility_camera, 0);
            attrs.SetInteger(RixStr.k_visibility_transmission, 0);
            attrs.SetInteger(RixStr.k_visibility_indirect, 0);
            // XXX: In Xpu, this may be "sides", not "Sides".
            attrs.SetInteger(RixStr.k_Sides, 1);
        }

        geomMat.SetScale(geomScale);

        // Adjust orientation to make prman match the USD spec.
        // TODO: Add another orientMat for PxrEnvDayLight when supported.
        GfMatrix4d orientMat(1.0);
        if (_lightShaderType == us_PxrDomeLight) {
            // Transform Dome to match OpenEXR spec for environment maps
            // Rotate -90 X, Rotate 90 Y
            orientMat = GfMatrix4d(0.0, 0.0, -1.0, 0.0, 
                                -1.0, 0.0, 0.0, 0.0, 
                                0.0, 1.0, 0.0, 0.0, 
                                0.0, 0.0, 0.0, 1.0);
            // Apply domeOffset if present
            VtValue domeOffset = sceneDelegate->GetLightParamValue(id,
                HdLightTokens->domeOffset);
            if (domeOffset.IsHolding<GfMatrix4d>()) {
                orientMat = orientMat * domeOffset.UncheckedGet<GfMatrix4d>();
            }
        } else if (_lightShaderType != us_PxrMeshLight) {
            // Transform lights to match correct orientation
            // Scale -1 Z, Rotate 180 Z
            orientMat = GfMatrix4d(-1.0, 0.0, 0.0, 0.0, 
                                0.0, -1.0, 0.0, 0.0, 
                                0.0, 0.0, -1.0, 0.0, 
                                0.0, 0.0, 0.0, 1.0);
        }
        geomMat = orientMat * geomMat;
        for (size_t i = 0; i < xf.count; ++i) {
            xf.values[i] = geomMat * xf.values[i];
        }

        attrs.SetInteger(RixStr.k_lighting_mute, !sceneDelegate->GetVisible(id));

        if (!isHdInstance) {
            // Singleton case. Create the light instance.

            // convert xform for riley
            TfSmallVector<RtMatrix4x4, HDPRMAN_MAX_TIME_SAMPLES> 
                xf_rt_values(xf.count);
            for (size_t i = 0; i < xf.count; ++i) {
                xf_rt_values[i] = HdPrman_Utils::GfMatrixToRtMatrix(xf.values[i]);
            }
            const riley::Transform xform = {
                unsigned(xf.count), xf_rt_values.data(), xf.times.data()
            };

            if (dirtySourceGeom
                && _instanceId != riley::LightInstanceId::InvalidId()) {
                riley->DeleteLightInstance(
                    riley::GeometryPrototypeId::InvalidId(),
                    _instanceId);
                _instanceId = riley::LightInstanceId::InvalidId();
            }

            // XXX: Temporary workaround for RMAN-20704
            // Destroy the light instance so it will be recreated instead
            // of being updated, since ModifyLightInstance may crash.
            if (_instanceId != riley::LightInstanceId::InvalidId()) {
                riley->DeleteLightInstance(
                    riley::GeometryPrototypeId::InvalidId(), _instanceId);
                _instanceId = riley::LightInstanceId::InvalidId();
            }
            // XXX: End of RMAN-20704 workaround

            if (_instanceId == riley::LightInstanceId::InvalidId()) {
                const riley::UserId userId(stats::AddDataLocation(id.GetText())
                    .GetValue());
                TRACE_SCOPE("riley::CreateLightInstance");
                _instanceId = riley->CreateLightInstance(
                    userId,
                    riley::GeometryPrototypeId::InvalidId(),
                    _geometryPrototypeId,
                    invalidMaterial,
                    _shaderId,
                    coordSysList,
                    xform,
                    attrs);
            } else {
                TRACE_SCOPE("riley::ModifyLightInstance");
                riley->ModifyLightInstance(
                    riley::GeometryPrototypeId::InvalidId(),
                    _instanceId,
                    &invalidMaterial,
                    &_shaderId,
                    &coordSysList,
                    &xform,
                    &attrs);
            }
        } else {
            // This light is a prototype of a hydra instancer. The light shader
            // has already been synced above, and any riley geometry prototypes
            // (if this is a mesh light) have already been synced as prototype-
            // only by gprim.h. We need to tell the HdPrmanInstancer to sync
            // riley light instances.
            HdInstancer::_SyncInstancerAndParents(renderIndex, instancerId);
            HdPrmanInstancer *instancer = static_cast<HdPrmanInstancer*>(
                renderIndex.GetInstancer(instancerId));
            if (instancer) {

                // if for some reason the source geometry id has changed, we
                // first have to depopulate the old light instances from the
                // parent instancer.
                if (dirtySourceGeom) {
                    instancer->Depopulate(renderParam, id);
                }
            
                // XXX: The dirtybits we have are not useful to the instancer.
                // we should translate them, but to do so accurately would
                // require a lot more state. So we will set DirtyTransform
                // as a token value to signal to the instancer to update the
                // instances.
                HdDirtyBits instanceDirtyBits(
                    HdChangeTracker::DirtyTransform |
                    (*dirtyBits & HdChangeTracker::DirtyInstancer));
                instancer->Populate(
                    renderParam,
                    &instanceDirtyBits,
                    id,
                    { _geometryPrototypeId },
                    coordSysList,
                    attrs, xf,
                    { invalidMaterial },
                    { primPath },
                    _shaderId);
            }
        }
    }

    if (dirtyBits) {
        *dirtyBits = HdChangeTracker::Clean;
    }
}

/* virtual */
HdDirtyBits
HdPrmanLight::GetInitialDirtyBitsMask() const
{
    return HdLight::AllDirty;
}

PXR_NAMESPACE_CLOSE_SCOPE
