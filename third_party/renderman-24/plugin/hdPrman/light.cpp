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
#include "hdPrman/material.h"
#include "hdPrman/mesh.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/debugCodes.h"
#include "hdPrman/rixStrings.h"
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

#include "hdPrman/lightFilterUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (meshLight)
    ((meshLightSourceMesh,          "sourceMesh"))
);

HdPrmanLight::HdPrmanLight(SdfPath const& id, TfToken const& lightType)
    : HdLight(id)
    , _hdLightType(lightType)
    , _shaderId(riley::LightShaderId::InvalidId())
    , _instanceId(riley::LightInstanceId::InvalidId())
    // Note: _groupPrototypeId isn't used yet. I.e., it's always invalid. 
    , _groupPrototypeId(riley::GeometryPrototypeId::InvalidId())
    , _geometryPrototypeId(riley::GeometryPrototypeId::InvalidId())
    , _instanceMaterialId(riley::MaterialId::InvalidId())
{
    /* NOTHING */
}

HdPrmanLight::~HdPrmanLight() = default;

void
HdPrmanLight::Finalize(HdRenderParam *renderParam)
{
    HdPrman_RenderParam *param =
        static_cast<HdPrman_RenderParam*>(renderParam);
    _ResetLight(param, true);
}

void
HdPrmanLight::_ResetLight(HdPrman_RenderParam *renderParam, bool clearFilterPaths)
{
    riley::Riley *riley = renderParam->AcquireRiley();

    if (!_lightLink.IsEmpty()) {
        renderParam->DecrementLightLinkCount(_lightLink);
        _lightLink = TfToken();
    }
    if (clearFilterPaths && !_lightFilterPaths.empty()) {
        _lightFilterPaths.clear();
    }
    if (!_lightFilterLinks.empty()) {
        for (const TfToken &filterLink: _lightFilterLinks)
            renderParam->DecrementLightFilterCount(filterLink);
        _lightFilterLinks.clear();
    }

    if (_instanceId != riley::LightInstanceId::InvalidId()) {
        riley->DeleteLightInstance(_groupPrototypeId, _instanceId);
        _instanceId = riley::LightInstanceId::InvalidId();
    }
    if (_shaderId != riley::LightShaderId::InvalidId()) {
        riley->DeleteLightShader(_shaderId);
        _shaderId = riley::LightShaderId::InvalidId();
    }

    _geometryPrototypeId = riley::GeometryPrototypeId::InvalidId();
    _instanceMaterialId = riley::MaterialId::InvalidId();
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
        combiner.params.SetLightFilterReferenceArray(
            entry.first, &entry.second[0], entry.second.size());
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

        HdPrmanLightFilterGenerateCoordSysAndLinks(
            &lightFilterNodes->back(),
            filterPath,
            coordsysIds,
            lightFilterLinks,
            sceneDelegate,
            param,
            riley);
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
    // Change tracking
    HdDirtyBits bits = *dirtyBits;

    static const RtUString us_PxrDomeLight("PxrDomeLight");
    static const RtUString us_PxrRectLight("PxrRectLight");
    static const RtUString us_PxrDiskLight("PxrDiskLight");
    static const RtUString us_PxrCylinderLight("PxrCylinderLight");
    static const RtUString us_PxrSphereLight("PxrSphereLight");
    static const RtUString us_PxrDistantLight("PxrDistantLight");
    static const RtUString us_PxrMeshLight("PxrMeshLight");
    static const RtUString us_shadowSubset("shadowSubset");
    static const RtUString us_default("default");

    SdfPath id = GetId();

    // XXX -- Only update mesh lights if *lighting* bits are dirty.
    // i.e., ignore mesh/transform/primvar/etc. changes.
    // This helps to prevent a Prman crash resulting from simultaneous 
    // edits to both the sprim and rprim pieces of a mesh light.
    // https://jira.pixar.com/browse/RMAN-20136
    
    bool dirtyLightingBits = false;
    if (_hdLightType == _tokens->meshLight) {

        // Make sure we have required resources. If we don't, we need to sync.
        if (_geometryPrototypeId == riley::GeometryPrototypeId::InvalidId() ||
                _instanceMaterialId  == riley::MaterialId::InvalidId()) {
            dirtyLightingBits = true;
        }

        // Are there dirty resource bits?
        if (*dirtyBits & HdLight::DirtyResource) {
            dirtyLightingBits = true;
        }

        // Are there dirty transform bits?
        if (*dirtyBits & HdLight::DirtyTransform) {
            dirtyLightingBits = true;
        }

        // Has source mesh changed? Is the rprim still there?
        const VtValue sourceMesh = sceneDelegate->GetLightParamValue(id, 
            _tokens->meshLightSourceMesh);
        if (sourceMesh.IsHolding<SdfPath>()) {
            const SdfPath sourceMeshPath = sourceMesh.UncheckedGet<SdfPath>();
            if (sourceMeshPath != _sourceMeshPath) {
                dirtyLightingBits = true;
                _sourceMeshPath = sourceMeshPath;
            } else {
                const HdRprim *rprim = 
                    sceneDelegate->GetRenderIndex().GetRprim(_sourceMeshPath);
                if (!rprim) {
                    dirtyLightingBits = true;
                }
            }
        }

        // Has linking changed?
        TfToken lightLink;
        VtValue val =
            sceneDelegate->GetLightParamValue(id, HdTokens->lightLink);
        if (val.IsHolding<TfToken>()) {
            lightLink = val.UncheckedGet<TfToken>();
        } else {
            dirtyLightingBits = true;                        
        }
        if (_lightLink != lightLink) {
            dirtyLightingBits = true;                        
        }

        // Have filters changed?
        // XXX -- We don't actually support filters yet.
        SdfPathVector lightFilterPaths;
        val = sceneDelegate->GetLightParamValue(id, HdTokens->filters);
        if (val.IsHolding<SdfPathVector>()) {
            SdfPathVector lightFilterPaths = val.UncheckedGet<SdfPathVector>();
        } else {
            dirtyLightingBits = true;                                    
        }
        if (_lightFilterPaths != lightFilterPaths) {
            dirtyLightingBits = true;
        }                        

        if (!dirtyLightingBits) {
            // No dirty lighting bits. We can skip this update.
            *dirtyBits = HdChangeTracker::Clean;
            return;
        }
    }

    if (dirtyLightingBits) {
        *dirtyBits = GetInitialDirtyBitsMask();

    }

    HdPrman_RenderParam * const param =
            static_cast<HdPrman_RenderParam*>(renderParam);

    riley::Riley *const riley = param->AcquireRiley();

    HdChangeTracker& changeTracker = 
        sceneDelegate->GetRenderIndex().GetChangeTracker();

    // Remove old dependencies before clearing the light.
    bool clearFilterPaths = false;
    if (bits & DirtyParams) {
        if (!_lightFilterPaths.empty()) {
            for (SdfPath const & filterPath : _lightFilterPaths) {
                changeTracker.RemoveSprimSprimDependency(filterPath, id);
            }
        }
        clearFilterPaths = true;
    }
    
    // For simplicity just re-create the light.  In the future we may
    // want to consider adding a path to use the Modify() API in Riley.
    _ResetLight(param, clearFilterPaths);

    // Early mesh light case test. See if the geometry prototype exists (yet).
    // Sprims get created before rprims, but for mesh lights we *need* the
    // source mesh rprim, since that's our geometry prototype. If it hasn't
    // yet been created, we return early. In that case, the light is still 
    // marked as "dirty", so this will run again.
    if (_hdLightType == _tokens->meshLight) {
        const HdRprim *rprim = 
                sceneDelegate->GetRenderIndex().GetRprim(_sourceMeshPath);

        if (!rprim) {
            // No prim. It may not have been created yet. Leave the light 
            // "dirty" and return.
            return;            
        }

        auto mesh = static_cast<const HdPrman_Mesh*>(rprim);
        std::vector<riley::GeometryPrototypeId> prototypeIds = 
                mesh->GetPrototypeIds();

        if (prototypeIds.empty()) {
            TF_WARN("Could not find prototype for mesh at '%s'."
                    " Skipping '%s'.", 
                    _sourceMeshPath.GetText(), 
                    id.GetText());
            // Light stays dirty.
            return;
        }

        // Find geometry prototype id.
        for (const auto &protoTypeId: prototypeIds) {
            if (protoTypeId != riley::GeometryPrototypeId::InvalidId()) {
                _geometryPrototypeId = protoTypeId;
                break;
            }
        }

        // Find instance material id.
        const SdfPath materialPath = sceneDelegate->GetMaterialId(
                _sourceMeshPath);
        if (materialPath == SdfPath()) {
            // Leave the light "dirty" and return.
            return;
        } 
        const HdSprim *sprim = 
                sceneDelegate->GetRenderIndex().GetSprim(
                        HdSprimTypeTokens->material,
                        materialPath);
        if (!sprim) {
            // No prim. It may not have been created yet. Leave the light 
            // "dirty" and return.            
            return;
        }
        auto hdPrmanMaterial = static_cast<const HdPrmanMaterial*>(sprim);
        _instanceMaterialId = hdPrmanMaterial->GetMaterialId();

        if (_instanceMaterialId == riley::MaterialId::InvalidId()) {
            TF_WARN("Could not find material for mesh at '%s'."
                    " Skipping '%s'.", 
                    _sourceMeshPath.GetText(), 
                    id.GetText());
            // Stay dirty. Return.
            return;
        }

        // Note: If we've returned early, we'll need to revisit this light 
        // once the other prims have been processed. This will continue 
        // until we succeed (and "bits" is marked "clean", which happens 
        // below). In the meshlight case, we're synthesizing the 
        // dependencies, so we know we'll succeed quickly. However, misuse 
        // of this code might result in a loop.
    }

    std::vector<riley::ShadingNode> lightNodes;

    _PopulateNodesFromMaterialResource(
        sceneDelegate, id, HdMaterialTerminalTokens->light, &lightNodes);

    if (lightNodes.empty() || lightNodes.back().name.Empty()) {
        TF_WARN("Could not populate shading nodes for light '%s'. Skipping.",
                id.GetText());
        *dirtyBits = HdChangeTracker::Clean;
        return;
    }

    TF_DEBUG(HDPRMAN_LIGHT_LIST)
        .Msg("HdPrman: Light <%s> lightType \"%s\"\n",
             id.GetText(), _hdLightType.GetText());

    // The terminal light node will be updated with other parameters that 
    // aren't direct inputs of the material resource.
    riley::ShadingNode &lightNode = lightNodes.back();

    // Attributes.
    RtParamList attrs = param->ConvertAttributes(sceneDelegate, id, false);

    // Check if the dome light should be camera visible
    if (lightNode.name == us_PxrDomeLight) {
        const bool domeLightCamVis = sceneDelegate->GetRenderIndex().
            GetRenderDelegate()->GetRenderSetting<bool>(
                HdRenderSettingsTokens->domeLightCameraVisibility,
                true);
        if (!domeLightCamVis) {
            attrs.SetInteger(RixStr.k_visibility_camera, 0);
        }
    }

    // Light linking
    {
        VtValue val =
            sceneDelegate->GetLightParamValue(id, HdTokens->lightLink);
        if (val.IsHolding<TfToken>()) {
            _lightLink = val.UncheckedGet<TfToken>();
        }

        if (!_lightLink.IsEmpty()) {
            param->IncrementLightLinkCount(_lightLink);
            // For lights to link geometry, the lights must
            // be assigned a grouping membership, and the
            // geometry must subscribe to that grouping.
            attrs.SetString(RixStr.k_grouping_membership,
                            RtUString(_lightLink.GetText()));
            TF_DEBUG(HDPRMAN_LIGHT_LINKING)
                .Msg("HdPrman: Light <%s> grouping membership \"%s\"\n",
                     id.GetText(), _lightLink.GetText());
        } else {
            // Default light group
            attrs.SetString(RixStr.k_grouping_membership, us_default);
            TF_DEBUG(HDPRMAN_LIGHT_LINKING)
                .Msg("HdPrman: Light <%s> grouping membership \"default\"\n",
                     id.GetText());
        }
    }

    // Shadow linking
    {
        VtValue shadowLinkVal =
            sceneDelegate->GetLightParamValue(id, HdTokens->shadowLink);
        if (shadowLinkVal.IsHolding<TfToken>()) {
            TfToken shadowLink = shadowLinkVal.UncheckedGet<TfToken>();
            if (!shadowLink.IsEmpty()) {
                lightNode.params.SetString(us_shadowSubset,
                                  RtUString(shadowLink.GetText()));
                TF_DEBUG(HDPRMAN_LIGHT_LINKING)
                    .Msg("HdPrman: Light <%s> shadowSubset \"%s\"\n",
                         id.GetText(), shadowLink.GetText());
            }
        }
    }

    // filters
    // Re-gather filter paths and add dependencies if necessary.
    if (clearFilterPaths) {
        VtValue val = sceneDelegate->GetLightParamValue(id, HdTokens->filters);
        if (val.IsHolding<SdfPathVector>()) {
            _lightFilterPaths = val.UncheckedGet<SdfPathVector>();
            for (SdfPath &filterPath: _lightFilterPaths) {
                changeTracker.AddSprimSprimDependency(filterPath, id);
            }
        }
    }

    std::vector<riley::ShadingNode> filterNodes;
    std::vector<riley::CoordinateSystemId> coordsysIds;
    _PopulateLightFilterNodes(
        id, _lightFilterPaths, sceneDelegate, renderParam, riley,
        &filterNodes, &coordsysIds, &_lightFilterLinks);

    // TODO: portals

    // Create the light shader.
    _shaderId = riley->CreateLightShader(
        riley::UserId(stats::AddDataLocation(id.GetText()).GetValue()),
        {static_cast<uint32_t>(lightNodes.size()), lightNodes.data()},
        {static_cast<uint32_t>(filterNodes.size()), filterNodes.data()});

    // Sample transform
    HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES> xf;
    sceneDelegate->SampleTransform(id, &xf);
    
    TfSmallVector<RtMatrix4x4, HDPRMAN_MAX_TIME_SAMPLES> 
        xf_rt_values(xf.count);

    GfMatrix4d geomMat(1.0);

    // Some lights have parameters that scale the size of the light.
    GfVec3d geomScale(1.0f);

    // Type-specific parameters
    if (lightNode.name == us_PxrRectLight) {
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
    } else if (lightNode.name == us_PxrDiskLight) {
        // radius (XY only, default 0.5)
        VtValue radius = sceneDelegate->GetLightParamValue(id, 
            HdLightTokens->radius);
        if (radius.IsHolding<float>()) {
            geomScale[0] *= radius.UncheckedGet<float>() / 0.5;
            geomScale[1] *= radius.UncheckedGet<float>() / 0.5;
        }
    } else if (lightNode.name == us_PxrCylinderLight) {
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
    } else if (lightNode.name == us_PxrSphereLight) {
        // radius (XYZ, default 0.5)
        VtValue radius = sceneDelegate->GetLightParamValue(id, 
            HdLightTokens->radius);
        if (radius.IsHolding<float>()) {
            geomScale *= radius.UncheckedGet<float>() / 0.5;
        }
    } else if (lightNode.name == us_PxrMeshLight) {
        // Our mesh light geom should not be visible, and should be one-sided, 
        // to match the existing Katana behavior.
        attrs.SetInteger(RixStr.k_visibility_camera, 0);
        attrs.SetInteger(RixStr.k_visibility_transmission, 0);
        attrs.SetInteger(RixStr.k_visibility_indirect, 0);
        // Note: In Xpu, this may be "sides", not "Sides".
        attrs.SetInteger(RixStr.k_Sides, 1);
    }

    geomMat.SetScale(geomScale);

    // Adjust orientation to make prman match the USD spec.
    // TODO: Add another orientMat for PxrEnvDayLight when supported.
    GfMatrix4d orientMat(1.0);
    if (lightNode.name == us_PxrDomeLight) {
        // Transform Dome to match OpenEXR spec for environment maps
        // Rotate -90 X, Rotate 90 Y
        orientMat = GfMatrix4d(0.0, 0.0, -1.0, 0.0, 
                               -1.0, 0.0, 0.0, 0.0, 
                               0.0, 1.0, 0.0, 0.0, 
                               0.0, 0.0, 0.0, 1.0);
    } else if (lightNode.name != us_PxrMeshLight) {
        // Transform lights to match correct orientation
        // Scale -1 Z, Rotate 180 Z
        orientMat = GfMatrix4d(-1.0, 0.0, 0.0, 0.0, 
                               0.0, -1.0, 0.0, 0.0, 
                               0.0, 0.0, -1.0, 0.0, 
                               0.0, 0.0, 0.0, 1.0);
    }
    geomMat = orientMat * geomMat;

    for (size_t i=0; i < xf.count; ++i) {
        xf_rt_values[i] = HdPrman_GfMatrixToRtMatrix(geomMat * xf.values[i]);
    }
    const riley::Transform xform = {
        unsigned(xf.count), xf_rt_values.data(), xf.times.data()};

    // Instance attributes.
    attrs.SetInteger(RixStr.k_lighting_mute, !sceneDelegate->GetVisible(id));

    // Coordsys.
    riley::CoordinateSystemList const coordsysList = {
                             unsigned(coordsysIds.size()), coordsysIds.data()};

    // Light instance.
    _instanceId = riley->CreateLightInstance(
        riley::UserId(stats::AddDataLocation(id.GetText()).GetValue()),
        _groupPrototypeId,
        _geometryPrototypeId, // No geo id, unless this is a mesh light.
        _instanceMaterialId, // No material id, unless this is a mesh light.
        _shaderId,
        coordsysList,
        xform,
        attrs);

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

bool
HdPrmanLight::IsValid() const
{
    return _instanceId != riley::LightInstanceId::InvalidId();
}

PXR_NAMESPACE_CLOSE_SCOPE

