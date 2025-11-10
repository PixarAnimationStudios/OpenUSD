//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/imaging/hdSt/dashDotLines.h"
#include "pxr/imaging/hdSt/dashDotLinesComputations.h"
#include "pxr/imaging/hdSt/dashDotLinesShaderKey.h"
#include "pxr/imaging/hdSt/dashDotLinesTopology.h"
#include "pxr/imaging/hdSt/bufferArrayRange.h"
#include "pxr/imaging/hdSt/computation.h"
#include "pxr/imaging/hdSt/drawItem.h"
#include "pxr/imaging/hdSt/extCompGpuComputation.h"
#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/imaging/hdSt/glslfxShader.h"
#include "pxr/imaging/hdSt/instancer.h"
#include "pxr/imaging/hdSt/material.h"
#include "pxr/imaging/hdSt/materialNetworkShader.h"
#include "pxr/imaging/hdSt/materialParam.h"
#include "pxr/imaging/hdSt/primUtils.h"
#include "pxr/imaging/hdSt/renderParam.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/textureBinder.h"
#include "pxr/imaging/hdSt/textureHandle.h"
#include "pxr/imaging/hdSt/textureObject.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/base/arch/hash.h"

#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec2i.h"

#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hf/diagnostic.h"
#include "pxr/base/vt/value.h"

#include "pxr/imaging/hio/glslfx.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/thisPlugin.h"

#include <tbb/concurrent_unordered_map.h>
#include <tbb/parallel_for.h>
#include <tuple>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

// The minimum count of dash-dot symbols in a period when it is faster to save the
// pattern in a texture than save it in an array of floats.
TF_DEFINE_ENV_SETTING(HD_SYMBOL_COUNT_USE_SAMPLE, 2,
    "Use Scene Index API for imaging scene input");

/// \brief A map from pattern hash to DashDotPattern.
tbb::concurrent_unordered_map<size_t, DashDotPattern> _dashDotPatternMap;
/// \brief A vector of DashDotPattern, used to store patterns that are not in the map.
std::vector<DashDotPattern> _dashDotPatternArray;

TfToken&
PatternToPathToken(DashDotPattern const& pattern)
{
    // Generate a hash for the pattern to use as a token.
    size_t hash = TfHash::Combine(
        pattern._pattern.size()
    );
    for (const GfVec2f& p : pattern._pattern) {
        hash = TfHash::Combine(hash, p[0], p[1]);
    }
    hash = TfHash::Combine(hash, pattern._period);

    int patternIndex = -1;
    // Try to add the pattern to the map.
    auto pair = _dashDotPatternMap.emplace(hash, pattern);
    if (!pair.second && (pair.first->second != pattern))
    {
        // If there is one different pattern with the same hash value in the map, 
        // add the pattern to a separate array.
        static tbb::spin_mutex mutex;
        tbb::spin_mutex::scoped_lock lock(mutex);
        // Find if the pattern already exists in the array.
        auto it = std::find_if(_dashDotPatternArray.begin(), _dashDotPatternArray.end(),
            [&pattern](const DashDotPattern& p) { return p == pattern; });
        if (it != _dashDotPatternArray.end()) {
            // If it exists, get the index.
            patternIndex = std::distance(_dashDotPatternArray.begin(), it);
        } else {
            // If it doesn't exist, add it to the array.
            _dashDotPatternArray.push_back(pattern);
            patternIndex = _dashDotPatternArray.size() - 1;
        }
    }
    // Generate a token based on the hash.
    TfToken patternToken = TfToken(TfStringPrintf("dashDotPattern_%zu_%d.dashdot", hash, patternIndex));
    return patternToken;
}

const DashDotPattern&
PathTokenToPattern(TfToken const& token)
{
    // Extract the hash from the token string.
    std::string const& tokenString = token.GetString();
    size_t hash = 0;
    int patternIndex = -1;
    if (sscanf(tokenString.c_str(), "dashDotPattern_%zu_%d.dashdot", &hash, &patternIndex) != EOF)
    {
        if (patternIndex >= 0) {
            if(_dashDotPatternArray.size() > patternIndex)
                return _dashDotPatternArray[patternIndex];
            else
                {
                TF_CODING_ERROR("Pattern index %d is out of bounds for token: %s", 
                    patternIndex, tokenString.c_str());
                return DashDotPattern();
            }
        }
        else {
            // Look up the pattern in the map using the hash.
            auto it = _dashDotPatternMap.find(hash);
            if (it != _dashDotPatternMap.end()) {
                return it->second;
            }
            else {
                TF_CODING_ERROR("No pattern found for token: %s", tokenString.c_str());
                return DashDotPattern();
            }
        }
    }
    else
    {
        TF_CODING_ERROR("No pattern found for token: %s", tokenString.c_str());
        return DashDotPattern();
    }
}

HdStMaterial* HdStDashDotLines::_fallbackMaterial = nullptr;

HdStDashDotLines::HdStDashDotLines(SdfPath const& id)
    : HdDashDotLines(id)
    , _topology()
    , _topologyId(0)
    , _customDirtyBitsInUse(0)
    , _refineLevel(0)
    , _displayOpacity(false)
    , _displayInOverlay(false)
    , _occludedSelectionShowsThrough(false)
    , _patternTexture(nullptr)
{
    /*NOTHING*/
}


HdStDashDotLines::~HdStDashDotLines() = default;

void
HdStDashDotLines::UpdateRenderTag(HdSceneDelegate *delegate,
                                 HdRenderParam *renderParam)
{
    HdStUpdateRenderTag(delegate, renderParam, this);
}


void
HdStDashDotLines::Sync(HdSceneDelegate *delegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits,
                      TfToken const   &reprToken)
{
    _UpdateVisibility(delegate, dirtyBits);

    bool updateMaterialTags = false;
    if (*dirtyBits & HdChangeTracker::DirtyMaterialId) {
        HdStSetMaterialId(delegate, renderParam, this);
        updateMaterialTags = true;
    }
    if (*dirtyBits & (HdChangeTracker::DirtyDisplayStyle|
                      HdChangeTracker::NewRepr)) {
        updateMaterialTags = true;
    }

    // Check if either the material or geometric shaders need updating for
    // draw items of all the reprs.
    bool updateMaterialNetworkShader = false;
    if (*dirtyBits & (HdChangeTracker::DirtyMaterialId |
                      HdChangeTracker::NewRepr)) {
        updateMaterialNetworkShader = true;
    }

    bool updateGeometricShader = false;
    if (*dirtyBits & (HdChangeTracker::DirtyDisplayStyle |
                      HdChangeTracker::DirtyMaterialId |
                      HdChangeTracker::DirtyTopology| // topological visibility
                      HdChangeTracker::NewRepr)) {
        updateGeometricShader = true;
    }

    bool displayOpacity = _displayOpacity;
    // The dash-dot patterns can be set to the shader via an array or via a
    // texture. Currently if the pattern contains only one dash or one dot,
    // we use the array, otherwise we use a texture.
    _UpdateRepr(delegate, renderParam, reprToken, dirtyBits);

    if (updateMaterialTags || 
        (GetMaterialId().IsEmpty() && displayOpacity != _displayOpacity)) { 
        _UpdateMaterialTagsForAllReprs(delegate, renderParam);
    }

    if (updateMaterialNetworkShader || updateGeometricShader) {
        _UpdateShadersForAllReprs(delegate, renderParam,
            updateMaterialNetworkShader, updateGeometricShader);
    }


    // This clears all the non-custom dirty bits. This ensures that the rprim
    // doesn't have pending dirty bits that add it to the dirty list every
    // frame.
    // XXX: GetInitialDirtyBitsMask sets certain dirty bits that aren't
    // reset (e.g. DirtyExtent, DirtyPrimID) that make this necessary.
    *dirtyBits &= ~ (HdChangeTracker::AllSceneDirtyBits | DirtyCamera);
}

void
HdStDashDotLines::Finalize(HdRenderParam *renderParam)
{
    HdStMarkGarbageCollectionNeeded(renderParam);

    HdStRenderParam * const stRenderParam =
        static_cast<HdStRenderParam*>(renderParam);

    // Decrement material tag counts for each draw item material tag
    for (auto const& reprPair : _reprs) {
        HdStDrawItem *drawItem = static_cast<HdStDrawItem*>(
            reprPair.second->GetDrawItem(0));
        stRenderParam->DecreaseMaterialTagCount(drawItem->GetMaterialTag());
    }
    stRenderParam->DecreaseRenderTagCount(GetRenderTag());
}

void
HdStDashDotLines::_UpdateDrawItem(HdSceneDelegate *sceneDelegate,
                                 HdRenderParam *renderParam,
                                 HdStDrawItem *drawItem,
                                 HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();

    /* MATERIAL SHADER (may affect subsequent primvar population) */
    if ((*dirtyBits & HdChangeTracker::NewRepr) ||
        HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id)) {
            drawItem->SetMaterialNetworkShader(_GetMaterialNetworkShader(sceneDelegate));
    }

    // Reset value of _displayOpacity
    if (HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id)) {
        _displayOpacity = false;
    }

    /* INSTANCE PRIMVARS */
    _UpdateInstancer(sceneDelegate, dirtyBits);
    HdStUpdateInstancerData(sceneDelegate->GetRenderIndex(),
                            renderParam,
                            this,
                            drawItem,
                            &_sharedData,
                            *dirtyBits);

    _displayOpacity = _displayOpacity ||
            HdStIsInstancePrimvarExistentAndValid(
            sceneDelegate->GetRenderIndex(), this, HdTokens->displayOpacity);

    /* CONSTANT PRIMVARS, TRANSFORM, EXTENT AND PRIMID */
    if (HdStShouldPopulateConstantPrimvars(dirtyBits, id)) {
        HdPrimvarDescriptorVector constantPrimvars =
            HdStGetPrimvarDescriptors(this, drawItem, sceneDelegate,
                HdInterpolationConstant);

        _PopulateConstantPrimvars(sceneDelegate,
                                  renderParam,
                                  drawItem,
                                  dirtyBits,
                                  constantPrimvars);

        _displayOpacity = _displayOpacity ||
            HdStIsPrimvarExistentAndValid(this, sceneDelegate, 
            constantPrimvars, HdTokens->displayOpacity);
    }

    /* TOPOLOGY */
    // XXX: _PopulateTopology should be split into two phase
    //      for scene dirtybits and for repr dirtybits.
    if (*dirtyBits & (HdChangeTracker::DirtyTopology
                    | HdChangeTracker::DirtyDisplayStyle
                    | DirtyIndices)) {
        
        bool oldNeedUpdateEachFrame = _topology && _topology->GetScreenSpacePattern();

        _PopulateTopology(
            sceneDelegate, renderParam, drawItem, dirtyBits);

        // If needUpdateEachFrame is changed, we need to update this information in render index.
        // _topology will always be generated in _PopulateTopology, so here we don't check it.
        bool needUpdateEachFrameChanged = oldNeedUpdateEachFrame ^ _topology->GetScreenSpacePattern();
        if (needUpdateEachFrameChanged)
        {
            if(!oldNeedUpdateEachFrame)
            {
                *dirtyBits |= DirtyCamera;
                sceneDelegate->GetRenderIndex().UpdateScreenSpaceDashDotLines(true, this);
            }
            else
            {
                *dirtyBits &= ~DirtyCamera;
                sceneDelegate->GetRenderIndex().UpdateScreenSpaceDashDotLines(false, this);
            }
        }
    }

    /* PRIMVAR */
    bool dirtyPrimvar = HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id);
    bool dirtyTransform = (*dirtyBits & DirtyCamera);
    if (dirtyPrimvar || dirtyTransform) {
        // XXX: curves don't use refined vertex primvars, however,
        // the refined renderpass masks the dirtiness of non-refined vertex
        // primvars, so we need to see refined dirty for updating coarse
        // vertex primvars if there is only refined reprs being updated.
        // we'll fix the change tracking in order to address this craziness.
        // When primvar is dirty, we need to pull the value of dirty primvar.
        // When camera is dirty, we also need to pull the value of the 
        // accumulated length.
        _PopulateVertexPrimvars(
            sceneDelegate, renderParam, drawItem, dirtyBits);
        if (dirtyPrimvar)
        {
            _PopulateVaryingPrimvars(
                sceneDelegate, renderParam, drawItem, dirtyBits);
            _PopulateElementPrimvars(
                sceneDelegate, renderParam, drawItem, dirtyBits);
        }
    }

    // When we have multiple drawitems for the same prim we need to clean the
    // bits for all the data fields touched in this function, otherwise it
    // will try to extract topology (for instance) twice, and this won't
    // work with delegates that don't keep information around once extracted.
    *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;

    // Topology and VertexPrimvar may be null, if the curve has zero line
    // segments.
    TF_VERIFY(drawItem->GetConstantPrimvarRange());
}

static const char*
HdSt_PrimTypeToString(HdSt_GeometricShader::PrimitiveType type) {
    switch (type)
    {
    case HdSt_GeometricShader::PrimitiveType::PRIM_BASIS_CURVES_LINES:
        return "lines";
    case HdSt_GeometricShader::PrimitiveType::PRIM_DASH_DOT_LINES:
        return "dash-dot lines";
    default:
        TF_WARN("Unknown type");
        return "unknown";
    }
}

static size_t
_GetTextureHandleHash(
    HdStTextureHandleSharedPtr const& textureHandle)
{
    const HdSamplerParameters& samplerParams =
        textureHandle->GetSamplerParameters();

    return TfHash::Combine(
        textureHandle->GetTextureObject()->GetTextureIdentifier(),
        samplerParams.wrapS,
        samplerParams.wrapT,
        samplerParams.wrapR,
        samplerParams.minFilter,
        samplerParams.magFilter,
        samplerParams.borderColor,
        samplerParams.enableCompare,
        samplerParams.compareFunction,
        samplerParams.maxAnisotropy);
}

void
HdStDashDotLines::_UpdateDrawItemGeometricShader(
        HdSceneDelegate *sceneDelegate,
        HdRenderParam *renderParam,
        HdStDrawItem *drawItem)
{
    if (!TF_VERIFY(_topology)) return;

    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();
    
    HdStResourceRegistrySharedPtr resourceRegistry =
        std::static_pointer_cast<HdStResourceRegistry>(
            renderIndex.GetResourceRegistry());
    
    TfToken shapeDetail = _topology->GetShapeDetail();
    bool screenSpacePattern = _topology->GetScreenSpacePattern();

    bool hasAuthoredTopologicalVisiblity =
        (bool) drawItem->GetTopologyVisibilityRange();

    float width = 1.0f;
    VtValue value = sceneDelegate->Get(GetId(), HdTokens->widths);
    if (!value.IsEmpty())
    {
        if (value.CanCast<VtFloatArray>())
        {
            VtFloatArray widths = value.Get<VtFloatArray>();
            if (widths.size() > 0)
            {
                // Use the first width value.
                width = widths[0];
            }
        }
        else if (value.CanCast<float>())
        {
            width = value.Get<float>();
        }
    }
    if (width != 1.0f)
    {
        HgiCapabilities const* capabilities =
            resourceRegistry->GetHgi()->GetCapabilities();
        bool wideLinesSupport = capabilities->IsSet(
            HgiDeviceCapabilitiesBitsWideLines);
        if (wideLinesSupport)
        {
            const float* lineWidthRange = capabilities->GetWideLineWidthRange();
            if (width < lineWidthRange[0] || width > lineWidthRange[1])
            {
                TF_WARN("HdStDashDotLines - the line width %f is out of the range"
                    "that this device can support. The range is between %f and %f."
                    "The line width will be clamped to this range.",
                    width, lineWidthRange[0], lineWidthRange[1]);
            }
        }
        else
        {
            TF_WARN("HdStDashDotLines - current device doesn't support line width"
                "other than 1.0. The line width will be set to 1.0.");
            width = 1.0f;
        }
    }
    HdSt_DashDotLinesShaderKey shaderKey(shapeDetail, screenSpacePattern, width,
        _patternTexture != nullptr, hasAuthoredTopologicalVisiblity);

    TF_DEBUG(HD_RPRIM_UPDATED).
            Msg("HdStDashDotLines(%s) - Shader Key PrimType: %s\n ",
                GetId().GetText(), HdSt_PrimTypeToString(shaderKey.primType));

    // If there is the texture for sampling the pixel information, we need to set
    // the texture to the geometric shader, and also add a parameter for the texture.
    HdStShaderCode::NamedTextureHandleVector textures;
    HdSt_MaterialParamVector params;
    if (_patternTexture)
    {
        textures.push_back(
            { HdStTokens->dashDotTexturePattern,
              HdStTextureType::Uv,
              { _patternTexture },
              _GetTextureHandleHash(_patternTexture) });

        params.push_back({
            HdSt_MaterialParam::ParamTypeTexture,
            HdStTokens->dashDotTexturePattern,
            VtValue(GfVec4f(0.0, 0.0, 0.0, 1.0)) });
    }

    HdSt_GeometricShaderSharedPtr geomShader =
        HdSt_GeometricShader::Create(shaderKey, textures, params, resourceRegistry);

    TF_VERIFY(geomShader);

    if (geomShader != drawItem->GetGeometricShader())
    {
        drawItem->SetGeometricShader(geomShader);

        // If the gometric shader changes, we need to do a deep validation of
        // batches, so they can be rebuilt if necessary.
        HdStMarkDrawBatchesDirty(renderParam);

        TF_DEBUG(HD_RPRIM_UPDATED).Msg(
            "%s: Marking all batches dirty to trigger deep validation because"
            " the geometric shader was updated.\n", GetId().GetText());
    }
}

HdDirtyBits
HdStDashDotLines::_PropagateDirtyBits(HdDirtyBits bits) const
{
    // propagate scene-based dirtyBits into rprim-custom dirtyBits
    if (bits & HdChangeTracker::DirtyTopology) {
        bits |= _customDirtyBitsInUse &
            (DirtyIndices|HdChangeTracker::DirtyPrimvar);
    }

    return bits;
}

void
HdStDashDotLines::_InitRepr(TfToken const &reprToken, HdDirtyBits *dirtyBits)
{
    _ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(),
                                            _ReprComparator(reprToken));
    bool isNew = it == _reprs.end();
    if (isNew) {
        // add new repr
        _reprs.emplace_back(reprToken, std::make_shared<HdRepr>());
        HdReprSharedPtr &repr = _reprs.back().second;

        *dirtyBits |= HdChangeTracker::NewRepr;

        HdRepr::DrawItemUniquePtr drawItem =
            std::make_unique<HdStDrawItem>(&_sharedData);
        HdDrawingCoord *drawingCoord = drawItem->GetDrawingCoord();
        repr->AddDrawItem(std::move(drawItem));
        if (!(_customDirtyBitsInUse & DirtyIndices)) {
            _customDirtyBitsInUse |= DirtyIndices;
            *dirtyBits |= DirtyIndices;
        }
        if (!(_customDirtyBitsInUse & DirtyCamera)) {
            _customDirtyBitsInUse |= DirtyCamera;
            // DirtyCamera is not set at first. It is only set when a new frame
            // starts.
        }
        // Set up drawing coord instance primvars.
        drawingCoord->SetInstancePrimvarBaseIndex(
            HdStDashDotLines::InstancePrimvar);
    }
}

void
HdStDashDotLines::_UpdateRepr(HdSceneDelegate *sceneDelegate,
                              HdRenderParam *renderParam,
                              TfToken const &reprToken,
                              HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdReprSharedPtr const &curRepr = _GetRepr(reprToken);
    if (!curRepr) {
        return;
    }

    // Filter custom dirty bits to only those in use.
    *dirtyBits &= (_customDirtyBitsInUse |
                   HdChangeTracker::AllSceneDirtyBits |
                   HdChangeTracker::NewRepr);

    if (TfDebug::IsEnabled(HD_RPRIM_UPDATED)) {
        TfDebug::Helper().Msg(
            "HdStDashDotLines::_UpdateRepr for %s : Repr = %s\n",
            GetId().GetText(), reprToken.GetText());
        HdChangeTracker::DumpDirtyBits(*dirtyBits);
    }
    HdStDrawItem* drawItem = static_cast<HdStDrawItem*>(
        curRepr->GetDrawItem(0));

    if (HdChangeTracker::IsDirty(*dirtyBits)) {
        _UpdateDrawItem(sceneDelegate, renderParam,
            drawItem, dirtyBits);
    }

    *dirtyBits &= ~HdChangeTracker::NewRepr;
}

void
HdStDashDotLines::_UpdateShadersForAllReprs(HdSceneDelegate *sceneDelegate,
                                            HdRenderParam *renderParam,
                                            bool updateMaterialNetworkShader,
                                            bool updateGeometricShader)
{
    TF_DEBUG(HD_RPRIM_UPDATED). Msg(
        "(%s) - Updating geometric and material shaders for draw "
        "items of all reprs.\n", GetId().GetText());

    HdSt_MaterialNetworkShaderSharedPtr materialNetworkShader;
    if (updateMaterialNetworkShader) {
        // If the curve style is dashdot, we will use the fallback dashdot material.
        materialNetworkShader = _GetMaterialNetworkShader(sceneDelegate);
    }

    const bool materialIsFinal = GetDisplayStyle(sceneDelegate).materialIsFinal;
    bool materialIsFinalChanged = false;
    for (auto const& reprPair : _reprs) {
        HdStDrawItem* drawItem = static_cast<HdStDrawItem*>(
            reprPair.second->GetDrawItem(0));
        if (materialIsFinal != drawItem->GetMaterialIsFinal()) {
            materialIsFinalChanged = true;
        }
        drawItem->SetMaterialIsFinal(materialIsFinal);

        if (updateMaterialNetworkShader) {
            drawItem->SetMaterialNetworkShader(materialNetworkShader);
        }
        if (updateGeometricShader) {
            _UpdateDrawItemGeometricShader(
                sceneDelegate, renderParam, drawItem);
        }
    }

    if (materialIsFinalChanged) {
        HdStMarkDrawBatchesDirty(renderParam);
        TF_DEBUG(HD_RPRIM_UPDATED).Msg(
            "%s: Marking all batches dirty to trigger deep validation because "
            "the materialIsFinal was updated.\n", GetId().GetText());
    }
}

void
HdStDashDotLines::_UpdateMaterialTagsForAllReprs(HdSceneDelegate *sceneDelegate,
                                                 HdRenderParam *renderParam)
{
    TF_DEBUG(HD_RPRIM_UPDATED). Msg(
        "(%s) - Updating material tags for draw items of all reprs.\n", 
        GetId().GetText());

    for (auto const& reprPair : _reprs) {
        HdStDrawItem* drawItem = static_cast<HdStDrawItem*>(
            reprPair.second->GetDrawItem(0));

        HdStSetMaterialTag(renderParam, drawItem, HdStMaterialTagTokens->translucent);
    }
}

void
HdStDashDotLines::_PopulateTopology(HdSceneDelegate *sceneDelegate,
                                    HdRenderParam *renderParam,
                                    HdStDrawItem *drawItem,
                                    HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();
    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        std::static_pointer_cast<HdStResourceRegistry>(
        sceneDelegate->GetRenderIndex().GetResourceRegistry());
    HdChangeTracker &changeTracker =
        sceneDelegate->GetRenderIndex().GetChangeTracker();

    if (*dirtyBits & HdChangeTracker::DirtyDisplayStyle) {
        HdDisplayStyle ds = GetDisplayStyle(sceneDelegate);
        _refineLevel = ds.refineLevel;
        _displayInOverlay = ds.displayInOverlay;
        _occludedSelectionShowsThrough = ds.occludedSelectionShowsThrough;
    }

    // XXX: is it safe to get topology even if it's not dirty?
    bool dirtyTopology = HdChangeTracker::IsTopologyDirty(*dirtyBits, id);

    if (dirtyTopology || HdChangeTracker::IsDisplayStyleDirty(*dirtyBits, id)) {

        const HdDashDotLinesTopology &srcTopology =
                                          GetDashDotLinesTopology(sceneDelegate);

        // Topological visibility (of points, curves) comes in as DirtyTopology.
        // We encode this information in a separate BAR.
        if (dirtyTopology) {
            // The points primvar is permitted to be larger than the number of
            // CVs implied by the topology.  So here we allow for
            // invisiblePoints being larger as well.
            size_t minInvisiblePointsCapacity = srcTopology.GetNumPoints();

            HdStProcessTopologyVisibility(
                srcTopology.GetInvisibleCurves(),
                srcTopology.GetNumCurves(),
                srcTopology.GetInvisiblePoints(),
                minInvisiblePointsCapacity,
                &_sharedData,
                drawItem,
                renderParam,
                &changeTracker,
                resourceRegistry,
                id);
        }

        // compute id.
        _topologyId = srcTopology.ComputeHash();
        bool refined = (_refineLevel>0);
        _topologyId = ArchHash64((const char*)&refined, sizeof(refined),
            _topologyId);

        // ask the registry if there is a sharable dashDotLines topology
        HdInstance<HdSt_DashDotLinesTopologySharedPtr> topologyInstance =
            resourceRegistry->RegisterDashDotLinesTopology(_topologyId);

        if (topologyInstance.IsFirstInstance()) {
            // if this is the first instance, create a new stream topology
            // representation and use that.
            HdSt_DashDotLinesTopologySharedPtr topology =
                                     HdSt_DashDotLinesTopology::New(srcTopology);

            topologyInstance.SetValue(topology);
        }

        _topology = topologyInstance.GetValue();
        TF_VERIFY(_topology);

        // hash collision check
        if (TfDebug::IsEnabled(HD_SAFE_MODE)) {
            TF_VERIFY(srcTopology == *_topology);
        }
    }

    // bail out if the index bar is already synced
    TfToken indexToken;
    if ((*dirtyBits & DirtyIndices) == 0) return;
    *dirtyBits &= ~DirtyIndices;
    indexToken = HdTokens->indices;

    {
        HdInstance<HdBufferArrayRangeSharedPtr> rangeInstance =
            resourceRegistry->RegisterDashDotLinesIndexRange(
                                                _topologyId, indexToken);

        if(rangeInstance.IsFirstInstance()) {
            HdBufferSourceSharedPtrVector sources;
            HdBufferSpecVector bufferSpecs;

            sources.push_back(_topology->GetIndexBuilderComputation(
                !_SupportsRefinement(_refineLevel)));

            HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);

            HdBufferArrayUsageHint usageHint =
                HdBufferArrayUsageHintBitsIndex |
                HdBufferArrayUsageHintBitsStorage;
            // Set up the usage hints to mark topology as varying if
            // there is a previously set range.
            if (drawItem->GetTopologyRange()) {
                usageHint |= HdBufferArrayUsageHintBitsSizeVarying;
            }

            // allocate new range
            HdBufferArrayRangeSharedPtr range
                = resourceRegistry->AllocateNonUniformBufferArrayRange(
                    HdTokens->topology, bufferSpecs, usageHint);

            // add sources to update queue
            resourceRegistry->AddSources(range, std::move(sources));
            rangeInstance.SetValue(range);
        }

        HdBufferArrayRangeSharedPtr const& newRange = rangeInstance.GetValue();

        HdStUpdateDrawItemBAR(
            newRange,
            drawItem->GetDrawingCoord()->GetTopologyIndex(),
            &_sharedData,
            renderParam,
            &changeTracker);
    }
}

namespace {

template <typename T> 
void 
AddVertexOrVaryingPrimvarSource(const SdfPath &id, const TfToken &name, 
    HdInterpolation interpolation, const VtValue &value, 
    HdSt_DashDotLinesTopologySharedPtr topology,
    HdBufferSourceSharedPtrVector *sources, T fallbackValue) {
    VtArray<T> array = value.Get<VtArray<T>>();
    // Empty primvar arrays are ignored, except for points
    if (!array.empty() || name == HdTokens->points) {
        sources->push_back(
            std::make_shared<HdSt_DashDotLinesPrimvarInterpolaterComputation<T>>(
                topology, array, id, name, interpolation, fallbackValue, 
                HdGetValueTupleType(VtValue(array)).type));
    }
}

void ProcessVertexOrVaryingPrimvar(
    const SdfPath &id, const TfToken &name, HdInterpolation interpolation, 
    const VtValue &value, HdSt_DashDotLinesTopologySharedPtr topology,
    HdBufferSourceSharedPtrVector *sources) {
    if (value.IsHolding<VtHalfArray>()) {
        AddVertexOrVaryingPrimvarSource<GfHalf>(
            id, name, interpolation, value, topology, sources, 1);
    } else if (value.IsHolding<VtFloatArray>()) {
        AddVertexOrVaryingPrimvarSource<float>(
            id, name, interpolation, value, topology, sources, 1);
    } else if (value.IsHolding<VtVec2fArray>()) {
        AddVertexOrVaryingPrimvarSource<GfVec2f>(
            id, name, interpolation, value, topology, sources, GfVec2f(1, 0));
    } else if (value.IsHolding<VtVec3fArray>()) {
        AddVertexOrVaryingPrimvarSource<GfVec3f>(
            id, name, interpolation, value, topology, sources, GfVec3f(1, 0, 0));
    } else if (value.IsHolding<VtVec4fArray>()) {
        AddVertexOrVaryingPrimvarSource<GfVec4f>(
            id, name, interpolation, value, topology, sources, GfVec4f(1, 0, 0, 1)); 
     } else if (value.IsHolding<VtDoubleArray>()) {
        AddVertexOrVaryingPrimvarSource<double>(
            id, name, interpolation, value, topology, sources, 1);
    } else if (value.IsHolding<VtVec2dArray>()) {
        AddVertexOrVaryingPrimvarSource<GfVec2d>(
            id, name, interpolation, value, topology, sources, GfVec2d(1, 0));
    } else if (value.IsHolding<VtVec3dArray>()) {
        AddVertexOrVaryingPrimvarSource<GfVec3d>(
            id, name, interpolation, value, topology, sources, GfVec3d(1, 0, 0));
    } else if (value.IsHolding<VtVec4dArray>()) {
        AddVertexOrVaryingPrimvarSource<GfVec4d>(
            id, name, interpolation, value, topology, sources, GfVec4d(1, 0, 0, 1));
    } else if (value.IsHolding<VtIntArray>()) {
        AddVertexOrVaryingPrimvarSource<int>(
            id, name, interpolation, value, topology, sources, 1); 
    } else if (value.IsHolding<VtVec2iArray>()) {
        AddVertexOrVaryingPrimvarSource<GfVec2i>(
            id, name, interpolation, value, topology, sources, GfVec2i(1, 0)); 
    } else if (value.IsHolding<VtVec3iArray>()) {
        AddVertexOrVaryingPrimvarSource<GfVec3i>(
            id, name, interpolation, value, topology, sources, GfVec3i(1, 0, 0)); 
    } else if (value.IsHolding<VtVec4iArray>()) {
        AddVertexOrVaryingPrimvarSource<GfVec4i>(
            id, name, interpolation, value, topology, sources, GfVec4i(1, 0, 0, 1)); 
    } else if (value.IsHolding<VtVec4iArray>()) {
        AddVertexOrVaryingPrimvarSource<GfVec4i>(
            id, name, interpolation, value, topology, sources, GfVec4i(1, 0, 0, 1)); 
    } else if (value.IsHolding<VtVec4iArray>()) {
        AddVertexOrVaryingPrimvarSource<GfVec4i>(
            id, name, interpolation, value, topology, sources, GfVec4i(1, 0, 0, 1)); 
    } else if (value.IsHolding<VtArray<int16_t>>()) {
        AddVertexOrVaryingPrimvarSource<int16_t>(
            id, name, interpolation, value, topology, sources, 1);
    } else if (value.IsHolding<VtArray<int32_t>>()) {
        AddVertexOrVaryingPrimvarSource<int32_t>(
            id, name, interpolation, value, topology, sources, 1);
    } else if (value.IsHolding<VtArray<uint16_t>>()) {
        AddVertexOrVaryingPrimvarSource<uint16_t>(
            id, name, interpolation, value, topology, sources, 1); 
    } else if (value.IsHolding<VtArray<uint32_t>>()) {
        AddVertexOrVaryingPrimvarSource<uint32_t>(
            id, name, interpolation, value, topology, sources, 1); 
    } else {
        TF_WARN("HdStDashDotLines(%s) - Type of vertex or varying primvar %s"
                " not yet fully supported", id.GetText(), name.GetText());
        sources->push_back(std::make_shared<HdVtBufferSource>(name, value));
    }
}
} // anonymous namespace

static GfVec2d NDCToScreen(GfVec2d NDC, GfVec2d screenDim)
{
    double const* ndcData = NDC.data();
    double const* screenDimData = screenDim.data();
    return GfVec2d(ndcData[0] * screenDimData[0] * 0.5 + screenDimData[0] * 0.5,
        ndcData[1] * screenDimData[1] * 0.5 + screenDimData[1] * 0.5);
}

template <typename T> 
static void _CalculateAccumulatedLength(HdSceneDelegate* sceneDelegate,
    const VtValue& value,
    const VtIntArray& curveVertexCounts,
    SdfPath const& id,
    bool screenSpacePattern,
    VtFloatArray& accumulatedLengths)
{
    VtValue castValue = VtValue::Cast<T>(value);
    const T& points = castValue.UncheckedGet<T>();

    // The count of points.
    size_t pointCount = points.size();
    // The count of curves.
    size_t curveCount = curveVertexCounts.size();

    VtVec2dArray screenPoints;
    if (screenSpacePattern)
    {
        HdRenderIndex& renderIndex = sceneDelegate->GetRenderIndex();
        const GfMatrix4d& wvpMatrix = renderIndex.GetCurrentWVPMatrix();
        const GfVec4f& viewport = renderIndex.GetCurrentViewport();

        GfMatrix4d transform;
        // For screen space length, we need to get the transform for each point.
        transform = sceneDelegate->GetTransform(id);
        transform = transform * wvpMatrix;

        // For screen spaced calculation, we need to convert the position to the screen
        // space position first.
        screenPoints.resize(pointCount);
        tbb::parallel_for(tbb::blocked_range<size_t>(0, pointCount),
            [&](const tbb::blocked_range<size_t>& r) {
                for (std::size_t i = r.begin(); i != r.end(); ++i) {
                    GfVec4d NDCPos = GfVec4d(points[i][0], points[i][1], points[i][2], 1.0) * transform;
                    NDCPos /= NDCPos[3];
                    screenPoints[i] = NDCToScreen(GfVec2d(NDCPos[0], NDCPos[1]),
                        GfVec2d(viewport[2], viewport[3]));
                }
            });
    }

    // Initialize the maximum vertex Index of the first curve.
    int currentCurveMaxVertexIndex = pointCount - 1;
    if (curveCount > 0)
    {
        currentCurveMaxVertexIndex = curveVertexCounts[0] - 1;
    }
    // Initialize the index of the first curve.
    int currentCurveIndex = 0;

    // Calculate the accumulated length.
    accumulatedLengths.reserve(pointCount);
    // Initialize the accumulatedLength.
    float accumulatedLength = 0.0f;
    accumulatedLengths.emplace_back(accumulatedLength);
    for (size_t pointIndex = 1; pointIndex < pointCount; ++pointIndex)
    {
        if (pointIndex > currentCurveMaxVertexIndex)
        {
            // Move to next curve.
            ++currentCurveIndex;
            if (currentCurveIndex < curveCount)
            {
                currentCurveMaxVertexIndex += curveVertexCounts[currentCurveIndex];
                // Reset accumulatedLength.
                accumulatedLength = 0;
                accumulatedLengths.emplace_back(accumulatedLength);
            }
            else
                break;
        }
        else
        {
            // Calculate the length from last point to current point, and accumulate it to the
            // accumulated length.
            if (screenSpacePattern)
            {
                // ScreenSpace calciulation, so we use the screen projected points.
                accumulatedLength += (screenPoints[pointIndex] - screenPoints[pointIndex - 1]).GetLength();
            }
            else
            {
                // World space calculation, we can directly calculate the length.
                accumulatedLength += (points[pointIndex] - points[pointIndex - 1]).GetLength();
            }
            accumulatedLengths.emplace_back(accumulatedLength);
        }
    }
}

template <typename T> 
static void _CalculateVertexInfo(const VtValue& value,
    const VtIntArray& curveVertexCounts,
    SdfPath const& id,
    HdSt_DashDotLinesTopologySharedPtr const& topology,
    HdBufferSourceSharedPtrVector *sources)
{
    VtValue castValue = VtValue::Cast<T>(value);
    const T& points = castValue.UncheckedGet<T>();

    T styleCurvePoints;
    T styleCurveAdjPoints1;
    T styleCurveAdjPoints2;
    T styleCurveAdjPoints3;
    VtFloatArray styleCurveExtrude;

    // The count of orginal points.
    size_t pointCount = points.size();
    // The count of curves.
    size_t curveCount = curveVertexCounts.size();

    // Initialize the maximum vertex Index of the first curve.
    int currentCurveMaxVertexIndex = -1;
    int currentCurveMinVertexIndex = -1;
    // Initialize the index of the first curve.
    int currentCurveIndex = 0;

    // Calculate the vertex information.
    // For each line segment, we will add four points: the previous adjacent point, the first point,
    // the second point, and the next adjacent point. At each point, we will record all these four 
    // points in Point and AdjPoint1-3.
    // If the line segment is a start segment of a curve, the previous adjacent point will be the
    // first point. If the line segment is the end segment of a curve, the next adjacent point will
    // be the second point.
    // We use extrude to identify the role of the point: 0.0 for previous adjacent point, 1.0 for
    // first point, 2.0 for second point, and 3.0 for next adjacent point.
    // Example: A curve which has 4 points, 1,2,3,4. Then the vertex information will be like below:
    // First line segment:
    // Points:      Pos1,     Pos1,     Pos1,     Pos1
    // AdjPoint1:   Pos1,     Pos1,     Pos1,     Pos1
    // AdjPoint2:   Pos2,     Pos2,     Pos2,     Pos2
    // AdjPoint3:   Pos3,     Pos3,     Pos3,     Pos3
    // Extrude:     0.0,      1.0,      2.0,      3.0
    // Second line segment:
    // Points:      Pos1,     Pos1,     Pos1,     Pos1
    // AdjPoint1:   Pos2,     Pos2,     Pos2,     Pos2
    // AdjPoint2:   Pos3,     Pos3,     Pos3,     Pos3
    // AdjPoint3:   Pos4,     Pos4,     Pos4,     Pos4
    // Extrude:     0.0,      1.0,      2.0,      3.0
    // Third line segment:
    // Points:      Pos2,     Pos2,     Pos2,     Pos2
    // AdjPoint1:   Pos3,     Pos3,     Pos3,     Pos3
    // AdjPoint2:   Pos4,     Pos4,     Pos4,     Pos4
    // AdjPoint3:   Pos4,     Pos4,     Pos4,     Pos4
    // Extrude:     0.0,      1.0,      2.0,      3.0
    int totalPointCount = 4 * (pointCount - curveCount);
    styleCurvePoints.reserve(totalPointCount);
    styleCurveAdjPoints1.reserve(totalPointCount);
    styleCurveAdjPoints2.reserve(totalPointCount);
    styleCurveAdjPoints3.reserve(totalPointCount);
    styleCurveExtrude.reserve(totalPointCount);
    for (int pointIndex = 0; pointIndex < pointCount; ++pointIndex)
    {
        int previousIndex, firstIndex, secondIndex, nextIndex;
        if (pointIndex > currentCurveMaxVertexIndex)
        {
            // This is the first point of the current curve.
            // Reset the currentCurveMinVertexIndex and currentCurveMaxVertexIndex.
            currentCurveMinVertexIndex = currentCurveMaxVertexIndex + 1;
            if (curveCount > 0)
            {
                currentCurveMaxVertexIndex += curveVertexCounts[currentCurveIndex];
                ++currentCurveIndex;
            }
            else
                currentCurveMaxVertexIndex = pointCount - 1;

            previousIndex = firstIndex = pointIndex;
        }
        else
        {
            previousIndex = pointIndex - 1;
            firstIndex = pointIndex;
        }

        if (pointIndex == currentCurveMaxVertexIndex)
        {
            continue;
        }
        else if (pointIndex + 1 == currentCurveMaxVertexIndex)
        {
            secondIndex = nextIndex = pointIndex + 1;
        }
        else
        {
            secondIndex = pointIndex + 1;
            nextIndex = pointIndex + 2;
        }

        styleCurvePoints.emplace_back(points[previousIndex]);
        styleCurvePoints.emplace_back(points[previousIndex]);
        styleCurvePoints.emplace_back(points[previousIndex]);
        styleCurvePoints.emplace_back(points[previousIndex]);
        styleCurveAdjPoints1.emplace_back(points[firstIndex]);
        styleCurveAdjPoints1.emplace_back(points[firstIndex]);
        styleCurveAdjPoints1.emplace_back(points[firstIndex]);
        styleCurveAdjPoints1.emplace_back(points[firstIndex]);
        styleCurveAdjPoints2.emplace_back(points[secondIndex]);
        styleCurveAdjPoints2.emplace_back(points[secondIndex]);
        styleCurveAdjPoints2.emplace_back(points[secondIndex]);
        styleCurveAdjPoints2.emplace_back(points[secondIndex]);
        styleCurveAdjPoints3.emplace_back(points[nextIndex]);
        styleCurveAdjPoints3.emplace_back(points[nextIndex]);
        styleCurveAdjPoints3.emplace_back(points[nextIndex]);
        styleCurveAdjPoints3.emplace_back(points[nextIndex]);
        styleCurveExtrude.emplace_back(0.0);
        styleCurveExtrude.emplace_back(1.0);
        styleCurveExtrude.emplace_back(2.0);
        styleCurveExtrude.emplace_back(3.0);
    }

    // Add the points source.
    ProcessVertexOrVaryingPrimvar(id, HdTokens->points,
        HdInterpolationVertex, VtValue(styleCurvePoints), topology, sources);

    // Add the first adjacent information source.
    ProcessVertexOrVaryingPrimvar(id, HdStTokens->adjPoints1,
        HdInterpolationVertex, VtValue(styleCurveAdjPoints1), topology, sources);

    // Add the second adjacent information source.
    ProcessVertexOrVaryingPrimvar(id, HdStTokens->adjPoints2,
        HdInterpolationVertex, VtValue(styleCurveAdjPoints2), topology, sources);

    // Add the third adjacent information source.
    ProcessVertexOrVaryingPrimvar(id, HdStTokens->adjPoints3,
        HdInterpolationVertex, VtValue(styleCurveAdjPoints3), topology, sources);

    // Add the extrude information source.
    ProcessVertexOrVaryingPrimvar(id, HdStTokens->extrude,
        HdInterpolationVertex, VtValue(styleCurveExtrude), topology, sources);
}

bool HdStDashDotLines::_HandleAdjVertexInfo(const VtValue& points,
    const VtIntArray& curveVertexCounts,
    HdBufferSourceSharedPtrVector *sources)
{
    using TypeList = std::tuple<
        VtVec3fArray,
        VtVec3dArray,
        VtVec3hArray,
        VtVec2fArray,
        VtVec2dArray,
        VtVec2hArray,
        VtVec4fArray,
        VtVec4dArray,
        VtVec4hArray
    >;

    SdfPath const& id = GetId();
    bool handled = false;

    // Calculate the vertex information.
    auto checkAndCalculate = [&](auto typeList) {
        std::apply([&](auto... type) {
            ((points.IsHolding<decltype(type)>() ?
                (_CalculateVertexInfo<decltype(type)>(points, curveVertexCounts, 
                    id, _topology, sources), handled = true) : false) || ...);
            }, typeList);
        };

    checkAndCalculate(TypeList{});

    if (!handled) {
        TF_CODING_ERROR("Incorrect type for points.");
        return false;
    }
    return true;
}

template <typename ELEM>
static bool _AssignArrayValues(const VtIntArray& curveVertexCounts,
    const VtArray<ELEM>& inputArray, VtArray<ELEM>& outputArray)
{
    // The count of curves.
    size_t curveCount = curveVertexCounts.size();            
    // The count of orginal values.
    size_t inputCount = inputArray.size();
    if(inputCount < curveCount * 2)
    {
        TF_CODING_ERROR("The count of vertex primvar values doesn't match the \
             curveVertexCounts property. Each vertex should have a matching \
             value for a vertex primvar.");
        return false;
    }
    // If there is no curveVertexCounts, there is only one curve. So the first and the
    // last vertex will generate 2 new vertices each, and the middle vertex will generate
    // 4 new vertices each. Totally there will be 4 + (inputCount - 2) * 4 new vertices.
    // If there is curveVertexCounts, for each curve, the start and end vertex will
    // generate 2 new vertices each, and the middle vertices will generate 4 new vertices
    // each, so there will be totally curveCount * 4 + (inputCount - curveCount * 2) * 4
    // new vertices.
    size_t outputCount = (curveCount == 0) ? 4 + (inputCount - 2) * 4 :
        curveCount * 4 + (inputCount - curveCount * 2) * 4;
    outputArray.reserve(outputCount);

    // Initialize the index of the first curve.
    size_t currentCurveIndex = 0;
    // Initialize the minimum vertex Index of the next curve. This is used to indicate if
    // a curve is finished.
    size_t nextCurveMinVertexIndex = 0;
    for (size_t intputIndex = 0; intputIndex < inputCount; ++intputIndex)
    {
        if (intputIndex == nextCurveMinVertexIndex)
        {
            // This is the first value of a new curve.
            // Reset the nextCurveMinVertexIndex.
            if (curveCount > 0)
            {
                nextCurveMinVertexIndex += curveVertexCounts[currentCurveIndex];
                ++currentCurveIndex;
                if (currentCurveIndex > curveCount)
                {
                    TF_CODING_ERROR("The count of vertex primvar values doesn't match \
                                the curveVertexCounts property. Each vertex should \
                                have a matching value for a vertex primvar.");
                    break;
                }
            }
            else
                nextCurveMinVertexIndex = inputCount;

            // The first vertex will be duplicated with two instances. So the vertex
            // primvar will also be duplicated.
            outputArray.emplace_back(inputArray[intputIndex]);
            outputArray.emplace_back(inputArray[intputIndex]);
        }
        else if (intputIndex == nextCurveMinVertexIndex - 1)
        {
            // This is the last value of the current curve.
            // The last vertex will be duplicated with two instances. So the vertex
            // primvar will also be duplicated.
            outputArray.emplace_back(inputArray[intputIndex]);
            outputArray.emplace_back(inputArray[intputIndex]);
        }
        else
        {
            // The middle vertex will be duplicated with four instances. So the vertex
            // primvar will also be duplicated.
            outputArray.emplace_back(inputArray[intputIndex]);
            outputArray.emplace_back(inputArray[intputIndex]);
            outputArray.emplace_back(inputArray[intputIndex]);
            outputArray.emplace_back(inputArray[intputIndex]);
        }
    }
    if (currentCurveIndex != curveCount || nextCurveMinVertexIndex != inputCount)
    {
        TF_CODING_ERROR("The count of vertex primvar values doesn't match the \
                                curveVertexCounts property. Each vertex should \
                                have a matching value for a vertex primvar.");
        return false;
    }
    return true;
}

static VtValue _AssignValues(VtValue& values,
    const VtIntArray& curveVertexCounts)
{
    if(!values.IsArrayValued())
        return values;
    else
    {
        // We will handle float3 primvars such as color and normal, and float primvars
        // such as width. We will not handle the other types of primvars.
        if (values.IsHolding<VtVec3fArray>())
        {
            const VtVec3fArray& float3Array = values.UncheckedGet<VtVec3fArray>();
            VtVec3fArray newFloat3Array;
            if(_AssignArrayValues(curveVertexCounts, float3Array, newFloat3Array))
                return VtValue(newFloat3Array);
            else
                return values;
        }
        else if (values.IsHolding<VtFloatArray>())
        {
            const VtFloatArray& floatArray = values.UncheckedGet<VtFloatArray>();
            VtFloatArray newFloatArray;
            if(_AssignArrayValues(curveVertexCounts, floatArray, newFloatArray))
                return VtValue(newFloatArray);
            else
                return values;
        }
        else
        {
            TF_CODING_ERROR("We don't support this type of vertex primvars, for a DSashDotLines.");
            return values;
        }
    }
}

bool 
HdStDashDotLines::_HandleAccumulatedWidth(
    HdSceneDelegate* sceneDelegate, 
    const VtValue& points,
    const VtIntArray& curveVertexCounts,
    HdBufferSourceSharedPtrVector *sources)
{
    // First calculate the accumulate length for each line segment.
    VtFloatArray accumulatedLengths;
    
    using TypeList2D = std::tuple<
        VtVec2fArray,
        VtVec2dArray,
        VtVec2hArray
    >;

    using TypeList3D = std::tuple<
        VtVec3fArray,
        VtVec3dArray,
        VtVec3hArray,
        VtVec4fArray,
        VtVec4dArray,
        VtVec4hArray
    >;

    SdfPath const& id = GetId();
    bool screenSpacePattern = _topology->GetScreenSpacePattern();
    bool handled = false;

    // Calculate the accumulated length.
    auto checkAndCalculate = [&](auto typeList, bool screenSpacePatternValue) {
        std::apply([&](auto... type) {
            ((points.IsHolding<decltype(type)>() ?
                (_CalculateAccumulatedLength<decltype(type)>(sceneDelegate, points, curveVertexCounts, 
                    id, screenSpacePatternValue, accumulatedLengths), handled = true) : false) || ...);
            }, typeList);
        };

    // First check if the type of the points is 3D or 4D type. This is because it is more common.
    checkAndCalculate(TypeList3D{}, screenSpacePattern);
    if (!handled) {
        // Then check if the type of the points is 2D type.
        // If the points is in 2D space, screenSpacePattern is ignored. We don't need to do projection.
        checkAndCalculate(TypeList2D{}, false);
    }

    if (!handled) {
        TF_CODING_ERROR("Incorrect type for points.");
        return false;
    }

    if (_topology->NeedAdjacentPoints())
    {
        // If the DashDotLines requires adjacent points information, it means the rendering requires the 
        // line segment be rendered as tw triangles. In this case, each line segment will have four points
        // and each point need the accumulated length at the start of the line and at the end of the line.
        // So we need to generated an array of float2 from the array of accumulated lengths.
        VtVec2fArray accumulatedLengthStartAndEnd;
        accumulatedLengthStartAndEnd.reserve((accumulatedLengths.size() - curveVertexCounts.size()) * 4);
        GfVec2f startAndEnd;
        // First initialize the length of the line segment at the start.
        startAndEnd[0] = accumulatedLengths[0];
        size_t curveCountIndex = 0;
        int currentCurveCount = curveVertexCounts[curveCountIndex];
        std::for_each(accumulatedLengths.begin() + 1, accumulatedLengths.end(),
            [&](float& length) {
                --currentCurveCount;
                if (currentCurveCount == 0)
                {
                    // We will start another curve. So reinitialize the length of the line segment at the start.
                    ++curveCountIndex;
                    currentCurveCount = curveVertexCounts[curveCountIndex];
                    startAndEnd[0] = length;
                }
                else
                {
                    // Set the length of the line segment at the end.
                    startAndEnd[1] = length;
                    // Fill the array of float2.
                    accumulatedLengthStartAndEnd.emplace_back(startAndEnd);
                    accumulatedLengthStartAndEnd.emplace_back(startAndEnd);
                    accumulatedLengthStartAndEnd.emplace_back(startAndEnd);
                    accumulatedLengthStartAndEnd.emplace_back(startAndEnd);
                    // Then initialize the length of the next line segment at the start.
                    startAndEnd[0] = length;
                }
            });
        // Add the accumulated length source.
        ProcessVertexOrVaryingPrimvar(GetId(), HdStTokens->accumulatedLength,
            HdInterpolationVertex, VtValue(accumulatedLengthStartAndEnd), _topology, sources);
    }
    else
    {
        // Add the accumulated length source.
        ProcessVertexOrVaryingPrimvar(GetId(), HdStTokens->accumulatedLength,
            HdInterpolationVertex, VtValue(accumulatedLengths), _topology, sources);
    }
    return true;
}

HdSt_MaterialNetworkShaderSharedPtr
HdStDashDotLines::_GetMaterialNetworkShader(
    HdSceneDelegate* delegate)
{
    // Resolve the prim's material or use the fallback material.
    SdfPath const& materialId = GetMaterialId();
    HdRenderIndex& renderIndex = delegate->GetRenderIndex();
    HdStMaterial const* material = static_cast<HdStMaterial const*>(
        renderIndex.GetSprim(HdPrimTypeTokens->material, materialId));
    if (material == nullptr) {
        return _GetDashDotMaterialNetworkShader(delegate);
    }
    else
        return material->GetMaterialNetworkShader();
}

void
HdStDashDotLines::_PopulateConstantPrimvars(
    HdSceneDelegate* delegate,
    HdRenderParam* renderParam,
    HdStDrawItem* drawItem,
    HdDirtyBits* dirtyBits,
    HdPrimvarDescriptorVector const& constantPrimvars)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();
    SdfPath const& instancerId = GetInstancerId();

    HdRenderIndex& renderIndex = delegate->GetRenderIndex();
    HdStResourceRegistrySharedPtr const& hdStResourceRegistry =
        std::static_pointer_cast<HdStResourceRegistry>(
            renderIndex.GetResourceRegistry());

    // Update uniforms
    HdBufferSourceSharedPtrVector sources;
    if (HdChangeTracker::IsTransformDirty(*dirtyBits, id)) {
        const GfMatrix4d transform = delegate->GetTransform(id);
        _sharedData.bounds.SetMatrix(transform); // for CPU frustum culling

        HgiCapabilities const* capabilities =
            hdStResourceRegistry->GetHgi()->GetCapabilities();
        bool const doublesSupported = capabilities->IsSet(
            HgiDeviceCapabilitiesBitsShaderDoublePrecision);

        sources.push_back(
            std::make_shared<HdVtBufferSource>(
                HdTokens->transform, transform, doublesSupported));

        sources.push_back(
            std::make_shared<HdVtBufferSource>(
                HdTokens->transformInverse, transform.GetInverse(),
                doublesSupported));

        bool leftHanded = transform.IsLeftHanded();

        // If this is a prototype (has instancer),
        // also push the instancer transform separately.
        if (!instancerId.IsEmpty()) {
            // Gather all instancer transforms in the instancing hierarchy
            const VtMatrix4dArray rootTransforms =
                GetInstancerTransforms(delegate);
            VtMatrix4dArray rootInverseTransforms(rootTransforms.size());
            for (size_t i = 0; i < rootTransforms.size(); ++i) {
                rootInverseTransforms[i] = rootTransforms[i].GetInverse();
                // Flip the handedness if necessary
                leftHanded ^= rootTransforms[i].IsLeftHanded();
            }

            sources.push_back(
                std::make_shared<HdVtBufferSource>(
                    HdInstancerTokens->instancerTransform,
                    rootTransforms,
                    rootTransforms.size(),
                    doublesSupported));
            sources.push_back(
                std::make_shared<HdVtBufferSource>(
                    HdInstancerTokens->instancerTransformInverse,
                    rootInverseTransforms,
                    rootInverseTransforms.size(),
                    doublesSupported));

            // XXX: It might be worth to consider to have isFlipped
            // for non-instanced prims as well. It can improve
            // the drawing performance on older-GPUs by reducing
            // fragment shader cost, although it needs more GPU memory.

            // Set as int (GLSL needs 32-bit align for bool)
            sources.push_back(
                std::make_shared<HdVtBufferSource>(
                    HdTokens->isFlipped,
                    VtValue(int(leftHanded))));
        }
    }
    if (HdChangeTracker::IsExtentDirty(*dirtyBits, id)) {
        // Note: If the scene description doesn't provide the extents, we use
        // the default constructed GfRange3d which is [FLT_MAX, -FLT_MAX],
        // which disables frustum culling for the prim.
        _sharedData.bounds.SetRange(GetExtent(delegate));

        GfVec3d const& localMin = drawItem->GetBounds().GetBox().GetMin();
        HdBufferSourceSharedPtr sourceMin = std::make_shared<HdVtBufferSource>(
            HdTokens->bboxLocalMin,
            VtValue(GfVec4f(
                localMin[0],
                localMin[1],
                localMin[2],
                1.0f)));
        sources.push_back(sourceMin);

        GfVec3d const& localMax = drawItem->GetBounds().GetBox().GetMax();
        HdBufferSourceSharedPtr sourceMax = std::make_shared<HdVtBufferSource>(
            HdTokens->bboxLocalMax,
            VtValue(GfVec4f(
                localMax[0],
                localMax[1],
                localMax[2],
                1.0f)));
        sources.push_back(sourceMax);
    }

    if (HdChangeTracker::IsPrimIdDirty(*dirtyBits, id)) {
        int32_t primId = GetPrimId();
        HdBufferSourceSharedPtr source = std::make_shared<HdVtBufferSource>(
            HdTokens->primID,
            VtValue(primId));
        sources.push_back(source);
    }

    if (HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id)) {
        sources.reserve(sources.size() + constantPrimvars.size());
        for (const HdPrimvarDescriptor& pv : constantPrimvars) {
            if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, pv.name)) {
                VtValue value = delegate->Get(id, pv.name);
                if (pv.name == HdTokens->startCapType || pv.name == HdTokens->endCapType)
                {
                    TfToken capType = value.Get<TfToken>();
                    int capTypeValue =
                        (capType == HdTokens->square) ? 1 : ((capType == HdTokens->triangle) ? 2 : 0);
                    HdBufferSourceSharedPtr source = std::make_shared<HdVtBufferSource>(pv.name, VtValue(capTypeValue));
                    sources.push_back(source);
                }
                else if (pv.name == HdTokens->pattern)
                {
                    _patternTexture = nullptr;
                    // The dash-dot patterns can be set to the shader via an array or via a
                    // texture. Currently if the pattern contains only one dash or one dot,
                    // we use the array, otherwise we use a texture.
                    VtVec2fArray pattern = value.Get<VtVec2fArray>();
                    int patternPartCount = pattern.size();
                    if (patternPartCount > 0)
                    {
                        if(pattern[0][0] != 0.0f)
                        {
                            TF_WARN("The first symbol of the pattern should start from 0.0, but got %f. "
                                    "This may cause unexpected results.", pattern[0][0]);
                        }
                        // When the pattern contains less symbols than the SYMBOL_COUNT_USE_SAMPLE,
                        // we will store the pattern in the array, and the shader will calculate the
                        // pattern based on the array.
                        // Otherwise, we will store the pattern in a texture, and the shader will
                        // sample the texture to get the pattern.
                        static int symbolCountUseSample = TfGetEnvSetting(HD_SYMBOL_COUNT_USE_SAMPLE);
                        if (patternPartCount < symbolCountUseSample)
                        {
                            HdBufferSourceSharedPtr source1 = std::make_shared<HdVtBufferSource>(pv.name, value, value.GetArraySize());
                            HdBufferSourceSharedPtr source2 = std::make_shared<HdVtBufferSource>(HdTokens->patternPartCount, VtValue(patternPartCount));
                            sources.push_back(source1);
                            sources.push_back(source2);
                        }
                        else
                        {
                            VtValue periodValue = delegate->Get(id, HdTokens->patternPeriod);
                            float period = periodValue.Get<float>();
                            _patternTexture = hdStResourceRegistry->AllocateTextureHandle(
                                HdStTextureIdentifier(PatternToPathToken({ pattern, period })),
                                HdStTextureType::Uv,
                                {
                                    HdWrapClamp,
                                    HdWrapClamp,
                                    HdWrapNoOpinion,
                                    HdMinFilterNearest,
                                    HdMagFilterNearest,
                                    HdBorderColorTransparentBlack,
                                    false,
                                    HdCmpFuncNever,
                                    16
                                },
                                0,
                                HdStShaderCodePtr());
                            HdBufferSourceSharedPtr source = std::make_shared<HdVtBufferSource>(HdTokens->patternPartCount, VtValue(patternPartCount));
                            sources.push_back(source);
                        }
                    }
                }
                // XXX Storm doesn't support string or token primvars yet
                else if (value.IsHolding<std::string>() ||
                    value.IsHolding<VtStringArray>() ||
                    value.IsHolding<TfToken>() ||
                    value.IsHolding<VtTokenArray>()) {
                    continue;
                }
                else if (value.IsArrayValued() && value.GetArraySize() == 0) {
                    // A value holding an empty array does not count as an
                    // empty value. Catch that case here.
                    //
                    // Do nothing in this case.
                }
                else if (!value.IsEmpty()) {
                    // Given that this is a constant primvar, if it is
                    // holding VtArray then use that as a single array
                    // value rather than as one value per element.
                    HdBufferSourceSharedPtr source =
                        std::make_shared<HdVtBufferSource>(pv.name, value,
                            value.IsArrayValued() ? value.GetArraySize() : 1);

                    // Skip buffer source if tuple type is invalid.
                    if (!TF_VERIFY(
                        source->GetTupleType().type != HdTypeInvalid)) {
                        continue;
                    }
                    if (!TF_VERIFY(source->GetTupleType().count > 0)) {
                        continue;
                    }

                    sources.push_back(source);
                }
            }
        }
    }

    HdBufferArrayRangeSharedPtr const& bar =
        drawItem->GetConstantPrimvarRange();

    if (HdStCanSkipBARAllocationOrUpdate(sources, bar, *dirtyBits)) {
        return;
    }

    HdBufferSpecVector bufferSpecs;
    HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);

    // XXX: This should be based off the DirtyPrimvarDesc bit.
    bool hasDirtyPrimvarDesc = (*dirtyBits & HdChangeTracker::DirtyPrimvar);
    HdBufferSpecVector removedSpecs;
    if (hasDirtyPrimvarDesc) {
        static TfTokenVector internallyGeneratedPrimvars =
        {
            HdTokens->transform,
            HdTokens->transformInverse,
            HdInstancerTokens->instancerTransform,
            HdInstancerTokens->instancerTransformInverse,
            HdTokens->isFlipped,
            HdTokens->bboxLocalMin,
            HdTokens->bboxLocalMax,
            HdTokens->primID
        };
        removedSpecs = HdStGetRemovedOrReplacedPrimvarBufferSpecs(bar,
            constantPrimvars, internallyGeneratedPrimvars, bufferSpecs, id);
    }

    HdBufferArrayRangeSharedPtr range =
        hdStResourceRegistry->UpdateShaderStorageBufferArrayRange(
            HdTokens->primvar, bar, bufferSpecs, removedSpecs,
            HdBufferArrayUsageHintBitsStorage);

    HdStUpdateDrawItemBAR(
        range,
        drawItem->GetDrawingCoord()->GetConstantPrimvarIndex(),
        &_sharedData,
        renderParam,
        &(renderIndex.GetChangeTracker()));

    TF_VERIFY(drawItem->GetConstantPrimvarRange()->IsValid());

    if (!sources.empty()) {
        hdStResourceRegistry->AddSources(
            drawItem->GetConstantPrimvarRange(), std::move(sources));
    }
}

void
HdStDashDotLines::_PopulateVertexPrimvars(HdSceneDelegate *sceneDelegate,
                                          HdRenderParam *renderParam,
                                          HdStDrawItem *drawItem,
                                          HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();
    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        std::static_pointer_cast<HdStResourceRegistry>(
        sceneDelegate->GetRenderIndex().GetResourceRegistry());

    // The "points" attribute is expected to be in this list.
    HdPrimvarDescriptorVector primvars =
        HdStGetPrimvarDescriptors(this, drawItem, sceneDelegate,
                                  HdInterpolationVertex);
    
    HdExtComputationPrimvarDescriptorVector compPrimvars =
        sceneDelegate->GetExtComputationPrimvarDescriptors(id,
            HdInterpolationVertex);

    HdBufferSourceSharedPtrVector sources;
    HdBufferSourceSharedPtrVector reserveOnlySources;
    HdBufferSourceSharedPtrVector separateComputationSources;
    HdStComputationComputeQueuePairVector computations;
    sources.reserve(primvars.size());

    HdSt_GetExtComputationPrimvarsComputations(
        id,
        sceneDelegate,
        compPrimvars,
        *dirtyBits,
        &sources,
        &reserveOnlySources,
        &separateComputationSources,
        &computations);

    // accumulatedLengthCalculated is to prevent we calculate accumulated length twice.
    bool accumulatedLengthCalculated = false;
    for (HdPrimvarDescriptor const& primvar: primvars) {
        if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, primvar.name))
        {
            // TODO: We don't need to pull primvar metadata every time a value
            // changes, but we need support from the delegate.
            // If the name of primvar is "points", it will be specially handled. When the 
            // curve doesn't have style, we simply add the source for the "points". When
            // the curve has style, we will add the source for the "points" together with
            // the source for adjoint points, and we also need to add extrude and 
            // accumulated length which will be used in the shader.
            if (primvar.name == HdTokens->points)
            {
                // Having a null topology is possible, but shouldn't happen when there
                // are points
                if (!_topology)
                {
                    TF_CODING_ERROR("No topology set for DashDotLines %s",
                        id.GetName().c_str());
                    break;
                }
                // Get the original points value.
                VtValue value = GetPrimvar(sceneDelegate, HdTokens->points);
                if (!value.IsEmpty()) {
                    // Handle points when the curve has a style.
                    VtIntArray curveVertexCounts = _topology->GetCurveVertexCounts();

                    if (_topology->NeedAdjacentPoints())
                    {
                        // Add adjacent points information to source.
                        _HandleAdjVertexInfo(value, curveVertexCounts, &sources);
                    }
                    else
                    {
                        // Add the points source.
                        ProcessVertexOrVaryingPrimvar(id, HdTokens->points,
                            HdInterpolationVertex, value, _topology, &sources);
                    }

                    if (!accumulatedLengthCalculated)
                    {
                        // If accumulatedLength is not calculated, we need to calculate it and add the source.
                        _HandleAccumulatedWidth(sceneDelegate, value, curveVertexCounts, &sources);
                        accumulatedLengthCalculated = true;
                    }
                }
                else
                    continue;
            }
            else
            {
                //assert name not in range.bufferArray.GetResources()
                VtValue value = GetPrimvar(sceneDelegate, primvar.name);
                if (!value.IsEmpty()) {
                    if (_topology->NeedAdjacentPoints())
                    {
                        // If the curve needs adjacent points information for each point, we need to 
                        // expand the vertex primivars so that each final vertex will have a corresponding 
                        // value.
                    VtIntArray curveVertexCounts = _topology->GetCurveVertexCounts();
                    value = _AssignValues(value, curveVertexCounts);
                    }

                    ProcessVertexOrVaryingPrimvar(id, primvar.name,
                        HdInterpolationVertex, value, _topology, &sources);

                    if (primvar.name == HdTokens->displayOpacity) {
                        _displayOpacity = true;
                    }
                }
            }
        }
        else
        {
            // If the camera is dirty, it means the curve requires screen space accumulated length.
            // And if accumulated length is not calculated, we will calculate the length.
            if (!accumulatedLengthCalculated && (*dirtyBits & HdDashDotLines::DirtyCamera) != 0)
            {
                // Should have topology.
                if (!_topology)
                {
                    TF_CODING_ERROR("No topology set for DashDotLines %s",
                        id.GetName().c_str());
                    break;
                }

                // If the primvar is accumulated length, we will calculated the length here.
                // First get the position for all points.
                VtValue value = GetPrimvar(sceneDelegate, HdTokens->points);
                if (value.IsEmpty())
                    continue;
                // If the points is in 2D space, we don't need to recalculate the accumulated length
                // when camera changes.
                if (value.IsHolding<VtVec2fArray>() || value.IsHolding<VtVec2dArray>() || value.IsHolding<VtVec2hArray>() )
                    continue;

                // Then get the curve information.
                VtIntArray curveVertexCounts = _topology->GetCurveVertexCounts();

                // Calculate the accumulatedLengths.
                _HandleAccumulatedWidth(sceneDelegate, value, curveVertexCounts, &sources);
                accumulatedLengthCalculated = true;
            }
        }
    }

    HdBufferArrayRangeSharedPtr const& bar = drawItem->GetVertexPrimvarRange();

    if (HdStCanSkipBARAllocationOrUpdate(sources, computations, bar,
            *dirtyBits)) {
        return;
    }

    // XXX: This should be based off the DirtyPrimvarDesc bit.
    bool hasDirtyPrimvarDesc = (*dirtyBits & HdChangeTracker::DirtyPrimvar);
    HdBufferSpecVector removedSpecs;
    if (hasDirtyPrimvarDesc) {
        static TfTokenVector internallyGeneratedPrimvars =
        {
            HdStTokens->adjPoints1,
            HdStTokens->adjPoints2,
            HdStTokens->adjPoints3,
            HdStTokens->accumulatedLength,
            HdStTokens->extrude
        };
        removedSpecs = HdStGetRemovedPrimvarBufferSpecs(bar, primvars, 
            compPrimvars, internallyGeneratedPrimvars, id);
    }

    HdBufferSpecVector bufferSpecs;
    HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);
    HdBufferSpec::GetBufferSpecs(reserveOnlySources, &bufferSpecs);
    HdStGetBufferSpecsFromCompuations(computations, &bufferSpecs);

    HdBufferArrayRangeSharedPtr range =
        resourceRegistry->UpdateNonUniformBufferArrayRange(
            HdTokens->primvar, bar, bufferSpecs, removedSpecs,
            HdBufferArrayUsageHintBitsVertex);

    HdStUpdateDrawItemBAR(
        range,
        drawItem->GetDrawingCoord()->GetVertexPrimvarIndex(),
        &_sharedData,
        renderParam,
        &(sceneDelegate->GetRenderIndex().GetChangeTracker()));

    if (!sources.empty() || !computations.empty()) {
        // If sources or computations are to be queued against the resulting
        // BAR, we expect it to be valid.
        if (!TF_VERIFY(drawItem->GetVertexPrimvarRange()->IsValid())) {
            return;
        }
    }

    // add sources to update queue
    if (!sources.empty()) {
        resourceRegistry->AddSources(drawItem->GetVertexPrimvarRange(),
                                     std::move(sources));
    }
    // add gpu computations to queue.
    for (auto const& compQueuePair : computations) {
        HdStComputationSharedPtr const& comp = compQueuePair.first;
        HdStComputeQueue queue = compQueuePair.second;
        resourceRegistry->AddComputation(
            drawItem->GetVertexPrimvarRange(), comp, queue);
    }
    if (!separateComputationSources.empty()) {
        TF_FOR_ALL(it, separateComputationSources) {
            resourceRegistry->AddSource(*it);
        }
    }
}

void
HdStDashDotLines::_PopulateVaryingPrimvars(HdSceneDelegate *sceneDelegate,
                                           HdRenderParam *renderParam,
                                           HdStDrawItem *drawItem,
                                           HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();
    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        std::static_pointer_cast<HdStResourceRegistry>(
        sceneDelegate->GetRenderIndex().GetResourceRegistry());

    // Gather varying primvars
    HdPrimvarDescriptorVector primvars = 
        HdStGetPrimvarDescriptors(this, drawItem, sceneDelegate,
                                  HdInterpolationVarying);


    HdBufferSourceSharedPtrVector sources;
    sources.reserve(primvars.size());

    for (HdPrimvarDescriptor const& primvar: primvars) {
        if (!HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, primvar.name)) {
            continue;
        }

        // TODO: We don't need to pull primvar metadata every time a value
        // changes, but we need support from the delegate.

        //assert name not in range.bufferArray.GetResources()
        VtValue value = GetPrimvar(sceneDelegate, primvar.name);
        if (!value.IsEmpty()) {
            ProcessVertexOrVaryingPrimvar(id, primvar.name, 
                HdInterpolationVarying, value, _topology, &sources);

            if (primvar.name == HdTokens->displayOpacity) {
                _displayOpacity = true;
            }
        }
    }
 
    HdBufferArrayRangeSharedPtr const& bar = drawItem->GetVaryingPrimvarRange();

    if (HdStCanSkipBARAllocationOrUpdate(sources, bar, *dirtyBits)) {
        return;
    }

    // XXX: This should be based off the DirtyPrimvarDesc bit.
    bool hasDirtyPrimvarDesc = (*dirtyBits & HdChangeTracker::DirtyPrimvar);
    HdBufferSpecVector removedSpecs;
    if (hasDirtyPrimvarDesc) {
        TfTokenVector internallyGeneratedPrimvars; // none
        removedSpecs = HdStGetRemovedPrimvarBufferSpecs(bar, primvars, 
            internallyGeneratedPrimvars, id);
    }

    HdBufferSpecVector bufferSpecs;
    HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);

    HdBufferArrayRangeSharedPtr range =
        resourceRegistry->UpdateNonUniformBufferArrayRange(
            HdTokens->primvar, bar, bufferSpecs, removedSpecs,
            HdBufferArrayUsageHintBitsStorage);

    HdStUpdateDrawItemBAR(
        range,
        drawItem->GetDrawingCoord()->GetVaryingPrimvarIndex(),
        &_sharedData,
        renderParam,
        &(sceneDelegate->GetRenderIndex().GetChangeTracker()));

    // add sources to update queue
    if (!sources.empty()) {
        if (!TF_VERIFY(drawItem->GetVaryingPrimvarRange()->IsValid())) {
            return;
        }
        resourceRegistry->AddSources(drawItem->GetVaryingPrimvarRange(),
                                     std::move(sources));
    }
}

void
HdStDashDotLines::_PopulateElementPrimvars(HdSceneDelegate *sceneDelegate,
                                           HdRenderParam *renderParam,
                                           HdStDrawItem *drawItem,
                                           HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();
    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();
    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        std::static_pointer_cast<HdStResourceRegistry>(
        renderIndex.GetResourceRegistry());

    HdPrimvarDescriptorVector uniformPrimvars =
        HdStGetPrimvarDescriptors(this, drawItem, sceneDelegate,
                                  HdInterpolationUniform);

    HdBufferSourceSharedPtrVector sources;
    sources.reserve(uniformPrimvars.size());

    const size_t numCurves = _topology ? _topology->GetNumCurves() : 0;

    for (HdPrimvarDescriptor const& primvar: uniformPrimvars) {
        if (!HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, primvar.name))
            continue;

        VtValue value = GetPrimvar(sceneDelegate, primvar.name);
        if (!value.IsEmpty()) {
            HdBufferSourceSharedPtr source =
                std::make_shared<HdVtBufferSource>(primvar.name, value);

            // verify primvar length
            if (source->GetNumElements() != numCurves) {
                HF_VALIDATION_WARN(id,
                    "# of curves mismatch (%d != %d) for uniform primvar %s",
                    (int)source->GetNumElements(), (int)numCurves, 
                    primvar.name.GetText());
                continue;
            }
           
            sources.push_back(source);

            if (primvar.name == HdTokens->displayOpacity) {
                 _displayOpacity = true;
            }
        }
    }

    HdBufferArrayRangeSharedPtr const& bar = drawItem->GetElementPrimvarRange();
    
    if (HdStCanSkipBARAllocationOrUpdate(sources, bar, *dirtyBits)) {
        return;
    }

    // XXX: This should be based off the DirtyPrimvarDesc bit.
    bool hasDirtyPrimvarDesc = (*dirtyBits & HdChangeTracker::DirtyPrimvar);
    HdBufferSpecVector removedSpecs;
    if (hasDirtyPrimvarDesc) {
        TfTokenVector internallyGeneratedPrimvars; // none
        removedSpecs = HdStGetRemovedPrimvarBufferSpecs(bar, uniformPrimvars, 
            internallyGeneratedPrimvars, id);
    }

    HdBufferSpecVector bufferSpecs;
    HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);
    
    HdBufferArrayRangeSharedPtr range =
        resourceRegistry->UpdateNonUniformBufferArrayRange(
            HdTokens->primvar, bar, bufferSpecs, removedSpecs,
            HdBufferArrayUsageHintBitsStorage);

    HdStUpdateDrawItemBAR(
        range,
        drawItem->GetDrawingCoord()->GetElementPrimvarIndex(),
        &_sharedData,
        renderParam,
        &(sceneDelegate->GetRenderIndex().GetChangeTracker()));


    if (!sources.empty()) {
        // If sources are to be queued against the resulting BAR, we expect it 
        // to be valid.
        if (!TF_VERIFY(drawItem->GetElementPrimvarRange()->IsValid())) {
            return;
        }
        resourceRegistry->AddSources(drawItem->GetElementPrimvarRange(),
                                     std::move(sources));
    }
}

static bool 
HdSt_HasResource(HdStDrawItem* drawItem, const TfToken& resourceToken){
    // Check for authored resource, we could leverage dirtyBits here as an
    // optimization, however the BAR is the ground truth, so until there is a
    // known performance issue, we just check them explicitly.
    bool hasAuthoredResouce = false;

    typedef HdBufferArrayRangeSharedPtr HdBarPtr;
    if (HdBarPtr const& bar = drawItem->GetConstantPrimvarRange()){
        HdStBufferArrayRangeSharedPtr bar_ =
            std::static_pointer_cast<HdStBufferArrayRange> (bar);
        hasAuthoredResouce |= bool(bar_->GetResource(resourceToken));
    }
    if (HdBarPtr const& bar = drawItem->GetVertexPrimvarRange()) {
        HdStBufferArrayRangeSharedPtr bar_ =
            std::static_pointer_cast<HdStBufferArrayRange> (bar);
        hasAuthoredResouce |= bool(bar_->GetResource(resourceToken));
    }
    if (HdBarPtr const& bar = drawItem->GetVaryingPrimvarRange()){
        HdStBufferArrayRangeSharedPtr bar_ =
            std::static_pointer_cast<HdStBufferArrayRange> (bar);

        hasAuthoredResouce |= bool(bar_->GetResource(resourceToken));
    }
    if (HdBarPtr const& bar = drawItem->GetElementPrimvarRange()){
        HdStBufferArrayRangeSharedPtr bar_ =
            std::static_pointer_cast<HdStBufferArrayRange> (bar);

        hasAuthoredResouce |= bool(bar_->GetResource(resourceToken));
    }
    int instanceNumLevels = drawItem->GetInstancePrimvarNumLevels();
    for (int i = 0; i < instanceNumLevels; ++i) {
        if (HdBarPtr const& bar = drawItem->GetInstancePrimvarRange(i)) {
            HdStBufferArrayRangeSharedPtr bar_ =
                std::static_pointer_cast<HdStBufferArrayRange> (bar);

            hasAuthoredResouce |= bool(bar_->GetResource(resourceToken));
        }
    }
    return hasAuthoredResouce;  
}

bool
HdStDashDotLines::_SupportsRefinement(int refineLevel)
{
    if(!_topology) {
        TF_CODING_ERROR("Calling _SupportsRefinement before topology is set");
        return false;
    }

    return refineLevel > 0;
}

bool 
HdStDashDotLines::_SupportsUserWidths(HdStDrawItem* drawItem){
    return HdSt_HasResource(drawItem, HdTokens->widths);
}
bool 
HdStDashDotLines::_SupportsUserNormals(HdStDrawItem* drawItem){
    return HdSt_HasResource(drawItem, HdTokens->normals);
}

HdDirtyBits
HdStDashDotLines::GetInitialDirtyBitsMask() const
{
    HdDirtyBits mask = HdChangeTracker::Clean
        | HdChangeTracker::InitRepr
        | HdChangeTracker::DirtyExtent
        | HdChangeTracker::DirtyNormals
        | HdChangeTracker::DirtyPoints
        | HdChangeTracker::DirtyPrimID
        | HdChangeTracker::DirtyPrimvar
        | HdChangeTracker::DirtyDisplayStyle
        | HdChangeTracker::DirtyRepr
        | HdChangeTracker::DirtyMaterialId
        | HdChangeTracker::DirtyTopology
        | HdChangeTracker::DirtyTransform 
        | HdChangeTracker::DirtyVisibility 
        | HdChangeTracker::DirtyWidths
        | HdChangeTracker::DirtyComputationPrimvarDesc
        | HdChangeTracker::DirtyInstancer
        ;

    return mask;
}

/*override*/
TfTokenVector const &
HdStDashDotLines::GetBuiltinPrimvarNames() const
{
    // screenSpaceWidths toggles the interpretation of widths to be in
    // screen-space pixels.  We expect this to be useful for implementing guides
    // or other UI elements drawn with DashDotLines.  The pointsSizeScale primvar
    // similarly is intended to give clients a way to emphasize or supress
    // certain  points by scaling their default size.

    // minScreenSpaceWidth gives a minimum screen space width in pixels for
    // DashDotLines when rendered as tubes or camera-facing ribbons. We expect
    // this to be useful for preventing thin curves such as hair from 
    // undesirably aliasing when their screen space width would otherwise dip
    // below one pixel.

    // pointSizeScale, screenSpaceWidths, and minScreenSpaceWidths are
    // explicitly claimed here as "builtin" primvar names because they are 
    // consumed in the low-level baisCurves.glslfx rather than declared as 
    // inputs in any material shader's metadata.  Mentioning them here means
    // they will always survive primvar filtering.

    auto _ComputePrimvarNames = [this](){
        TfTokenVector primvarNames =
            this->HdDashDotLines::GetBuiltinPrimvarNames();
        primvarNames.push_back(HdStTokens->pointSizeScale);
        primvarNames.push_back(HdStTokens->screenSpaceWidths);
        primvarNames.push_back(HdStTokens->minScreenSpaceWidths);
        return primvarNames;
    };
    static TfTokenVector primvarNames = _ComputePrimvarNames();
    return primvarNames;
}

static TfToken
_GetShaderPath(char const * shader)
{
    static PlugPluginPtr plugin = PLUG_THIS_PLUGIN;
    const std::string path =
        PlugFindPluginResource(plugin, TfStringCatPaths("shaders", shader));
    TF_VERIFY(!path.empty(), "Could not find shader: %s\n", shader);

    return TfToken(path);
}

void
HdStDashDotLines::_CreateDashDotFallbackMaterialPrim()
{
    // Create the fallback material network for the dashdot styled lines.
    if(_fallbackMaterial)
        return;

    HioGlslfxSharedPtr glslfx =
        std::make_shared<HioGlslfx>(_GetShaderPath("dashDotFallbackMaterialNetwork.glslfx"));

    HdSt_MaterialNetworkShaderSharedPtr fallbackShaderCode =
        std::make_shared<HdStGLSLFXShader>(glslfx);

    _fallbackMaterial = new HdStMaterial(SdfPath::EmptyPath());
    _fallbackMaterial->SetMaterialNetworkShader(fallbackShaderCode);
}

HdSt_MaterialNetworkShaderSharedPtr
HdStDashDotLines::_GetDashDotMaterialNetworkShader(
    HdSceneDelegate * delegate)
{
    // Use the fallback material if the material is not set.
    HdRenderIndex &renderIndex = delegate->GetRenderIndex();
    HdStMaterial const * material = static_cast<HdStMaterial const *>(
            renderIndex.GetSprim(HdPrimTypeTokens->material, GetMaterialId()));
    if (material == nullptr) {
        static std::once_flag once;
        std::call_once(once, []() {
                _CreateDashDotFallbackMaterialPrim();
            });

        return _fallbackMaterial->GetMaterialNetworkShader();
    }
    else
        return material->GetMaterialNetworkShader();
}

PXR_NAMESPACE_CLOSE_SCOPE
