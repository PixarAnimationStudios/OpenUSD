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
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/hdSt/light.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxrUsdMayaGL/usdShapeRenderer.h"
#include "pxrUsdMayaGL/usdBatchRenderer.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(PXRUSDMAYARENDER_QUEUE_INFO,
            "Prints out batch renderer queuing info.");
}

/* static */
void
UsdBatchRenderer::Init()
{
    GlfGlewInit();
}

UsdBatchRenderer UsdBatchRenderer::_sGlobalRenderer;

UsdBatchRenderer::UsdBatchRenderer()
    : _renderIndex(nullptr)
	, _renderDelegate()
    , _taskDelegate()
    , _intersector()
	, _selTracker(new HdxSelectionTracker)
	, _lightingContext(GlfSimpleLightingContext::New())
	, _renderTimeStamp(0)
	, _selectTimeStamp(0)
{
	_renderIndex = HdRenderIndex::New(&_renderDelegate);
    if (!TF_VERIFY(_renderIndex != nullptr)) {
        return;
    }
    _taskDelegate = UsdTaskDelegateSharedPtr(
                          new UsdTaskDelegate(_renderIndex, SdfPath("/mayaTask")));
    _intersector = HdxIntersectorSharedPtr(new HdxIntersector(_renderIndex));
}

bool UsdBatchRenderer::updateRenderTimeStamp(unsigned long long timeStamp)
{
	if (timeStamp == _renderTimeStamp)
		return false;
	_renderTimeStamp = timeStamp;
	return true;
}

bool UsdBatchRenderer::updateSelectTimeStamp(unsigned long long timeStamp)
{
	if (timeStamp == _selectTimeStamp)
		return false;
	_selectTimeStamp = timeStamp;
	return true;
}

void
UsdBatchRenderer::InsertRenderQueue(
	UsdShapeRenderer * renderer,
	uint8_t refineLevel,
	TfTokenVector const & renderTags,
	const GfVec4f& overrideColor)
{
	if (! renderer)
		return;

	if (! renderer->IsPopulated())
		_populateQueue.insert(renderer);

	// Set RenderParams
	UsdRenderParams params;
	params.refineLevel = refineLevel;
	params.renderTags = renderTags;
	params.overrideColor = overrideColor;
	size_t paramKey = params.Hash();

	// Insert the corresponding render queue according to the renderParams 
	const SdfPath& sharedId = renderer->GetSdfPath();
	auto renderSetIter = _renderQueue.find(paramKey);
	if (renderSetIter == _renderQueue.end())
	{
		// If we had no _SdfPathSet for this particular RenderParam combination,
		// create a new one.
		_renderQueue[paramKey] = _RenderParamSet(params, _SdfPathSet({ sharedId }));
	}
	else
	{
		_SdfPathSet &renderPaths = renderSetIter->second.second;
		renderPaths.insert(sharedId);
	}
}

void
UsdBatchRenderer::RemoveRenderQueue(UsdShapeRenderer * renderer)
{
	if (! renderer)
		return;

	for (auto &renderSetIter : _renderQueue)
	{
		_SdfPathSet &renderPaths = renderSetIter.second.second;
		auto sdfIter = renderPaths.find(renderer->GetSdfPath());
		if (sdfIter != renderPaths.end())
		{
			renderPaths.erase(sdfIter);
			break;
		}
	}
}

void
UsdBatchRenderer::PopulateShapeRenderer()
{
	if (_populateQueue.empty())
		return;

	TF_DEBUG(PXRUSDMAYARENDER_QUEUE_INFO).Msg(
		"____________ POPULATE STAGE START ______________ (%zu)\n", _populateQueue.size());

	std::vector<UsdImagingDelegate*> delegates;
	UsdPrimVector rootPrims;
	std::vector<SdfPathVector> excludedPrimPaths;
	std::vector<SdfPathVector> invisedPrimPaths;

	for (UsdShapeRenderer *shapeRenderer : _populateQueue)
	{
		delegates.push_back(shapeRenderer->GetDelegate());
		rootPrims.push_back(shapeRenderer->GetRootPrim());
		excludedPrimPaths.push_back(shapeRenderer->GetExcludedPaths());
		invisedPrimPaths.push_back(SdfPathVector());

		shapeRenderer->Populated();
	}

	UsdImagingDelegate::Populate(delegates,
		rootPrims,
		excludedPrimPaths,
		invisedPrimPaths);

	// The queue will be cleared after populated.
	_populateQueue.clear();

	TF_DEBUG(PXRUSDMAYARENDER_QUEUE_INFO).Msg(
		"^^^^^^^^^^^^ POPULATE STAGE FINISH ^^^^^^^^^^^^^ (%zu)\n", _populateQueue.size());
}

void
UsdBatchRenderer::RenderBatches(
	TfToken drawRepr,
	HdCullStyle cullStyle,
	const GfMatrix4d& viewMatrix,
	const GfMatrix4d& projectionMatrix,
	const GfVec4d& viewport)
{
	if (_renderQueue.empty())
		return;

	TF_DEBUG(PXRUSDMAYARENDER_QUEUE_INFO).Msg(
		"____________ RENDER STAGE START ______________ (%zu)\n", _renderQueue.size());

	_taskDelegate->SetCameraState(viewMatrix, projectionMatrix, viewport);

	glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT);
	// hydra orients all geometry during topological processing so that
	// front faces have ccw winding. We disable culling because culling
	// is handled by fragment shader discard.
	glFrontFace(GL_CCW); // < State is pushed via GL_POLYGON_BIT
	glDisable(GL_CULL_FACE);

	// note: to get benefit of alpha-to-coverage, the target framebuffer
	// has to be a MSAA buffer.
	glDisable(GL_BLEND);
	glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// render task setup
	HdTaskSharedPtrVector tasks = _taskDelegate->GetSetupTasks(_lightingContext); // lighting etc

	for (const auto &renderSetIter : _renderQueue)
	{
		const UsdRenderParams &params = renderSetIter.second.first;
		const _SdfPathSet &renderPaths = renderSetIter.second.second;
		if (renderPaths.empty())
			continue;

		TF_DEBUG(PXRUSDMAYARENDER_QUEUE_INFO).Msg(
			"*** renderQueue, batch %zx, size %zu\n",
			renderSetIter.first, renderPaths.size());

		SdfPathVector roots(renderPaths.begin(), renderPaths.end());
		tasks.push_back(
			_taskDelegate->GetRenderTask(
				renderSetIter.first,
				params.renderTags,
				drawRepr,
				params.overrideColor,
				cullStyle,
				roots));
	}

	VtValue selectionValue(_selTracker);
	_hdEngine.SetTaskContextData(HdxTokens->selectionState, selectionValue);
	_hdEngine.Execute(*_renderIndex, tasks);

	glPopAttrib(); // GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT

	TF_DEBUG(PXRUSDMAYARENDER_QUEUE_INFO).Msg(
		"^^^^^^^^^^^^ RENDER STAGE FINISH ^^^^^^^^^^^^^ (%zu)\n", _renderQueue.size());
}

void
UsdBatchRenderer::RenderSelects(
    unsigned int pickResolution,
    bool singleSelection,
	const GfMatrix4d &viewMatrix,
	const GfMatrix4d &projectionMatrix,
	TfToken drawRepr,
	HdCullStyle cullStyle)
{
    // Guard against user clicking in viewer before renderer is setup
    if( ! _renderIndex || _renderQueue.empty())
        return;

    TF_DEBUG(PXRUSDMAYARENDER_QUEUE_INFO).Msg(
        "____________ SELECTION STAGE START ______________ (singleSelect = %d)\n",
        singleSelection );

	_selectResults.clear();

    _intersector->SetResolution(GfVec2i(pickResolution, pickResolution));
        
    HdxIntersector::Params qparams;
    qparams.viewMatrix = viewMatrix;
    qparams.projectionMatrix = projectionMatrix;
    qparams.alphaThreshold = 0.1;
    
    for( const auto &renderSetIter : _renderQueue)
    {
        const UsdRenderParams &renderParams = renderSetIter.second.first;
        const _SdfPathSet &renderPaths = renderSetIter.second.second;
        SdfPathVector roots(renderPaths.begin(), renderPaths.end());
            
        TF_DEBUG(PXRUSDMAYARENDER_QUEUE_INFO).Msg(
                "--- pickQueue, batch %zx, size %zu\n",
                renderSetIter.first, renderPaths.size());
            
        TfToken colName = HdTokens->geometry;
        HdRprimCollection rprims(colName, drawRepr);
        rprims.SetRootPaths(roots);
		rprims.SetRenderTags(renderParams.renderTags);

        qparams.cullStyle = cullStyle;
		qparams.renderTags = renderParams.renderTags;
            
        HdxIntersector::Result result;
        HdxIntersector::HitVector hits;

        if( ! _intersector->Query(qparams, rprims, &_hdEngine, &result) )
            continue;
            
        if( singleSelection )
        {
            hits.resize(1);
            if( ! result.ResolveNearest(&hits.front()) )
                continue;
        }
        else if( ! result.ResolveAll(&hits) )
        {
            continue;
        }

		// Support multiply selection.
		std::unordered_map<size_t, HdxIntersector::Hit> hitMap;
        for (const HdxIntersector::Hit& hit : hits) {
			size_t hashKey = hit.delegateId.GetHash();
			boost::hash_combine(hashKey, hit.objectId.GetHash());
			boost::hash_combine(hashKey, hit.instanceIndex);
            auto itIfExists =
				hitMap.insert(
                    std::pair<size_t, HdxIntersector::Hit>(hashKey, hit));
                
            const bool &inserted = itIfExists.second;
            if( inserted )
                continue;
                                
            HdxIntersector::Hit& existingHit = itIfExists.first->second;
            if( hit.ndcDepth < existingHit.ndcDepth )
                existingHit = hit;
        }
		for (const auto &hit: hitMap)
		{
			_selectResults.insert(std::pair<SdfPath, HdxIntersector::Hit>(hit.second.delegateId, hit.second));
		}
    }
        
    if( singleSelection && _selectResults.size()>1 )
    {
        TF_DEBUG(PXRUSDMAYARENDER_QUEUE_INFO).Msg(
                "!!! multiple singleSel hits found: %zu\n",
                _selectResults.size());
            
        auto minIt=_selectResults.begin();
        for( auto curIt=minIt; curIt!=_selectResults.end(); curIt++ )
        {
            const HdxIntersector::Hit& curHit = curIt->second;
            const HdxIntersector::Hit& minHit = minIt->second;
            if( curHit.ndcDepth < minHit.ndcDepth )
                minIt = curIt;
        }
            
        if( minIt!=_selectResults.begin() )
            _selectResults.erase(_selectResults.begin(),minIt);
        minIt++;
        if( minIt!=_selectResults.end() )
            _selectResults.erase(minIt,_selectResults.end());
    }
        
    if( TfDebug::IsEnabled(PXRUSDMAYARENDER_QUEUE_INFO) )
    {
        for ( const auto &selectPair : _selectResults)
        {
            const HdxIntersector::Hit& hit = selectPair.second;
            std::cout << "NEW HIT: " << std::endl;
			std::cout << "\tdelegateId: " << hit.delegateId << std::endl;
			std::cout << "\tobjectId: " << hit.objectId << std::endl;
			std::cout << "\tndcDepth: " << hit.ndcDepth << std::endl;
        }
    }
    
    TF_DEBUG(PXRUSDMAYARENDER_QUEUE_INFO).Msg(
        "^^^^^^^^^^^^ SELECTION STAGE FINISH ^^^^^^^^^^^^^\n");
}


UsdBatchRenderer::HitInfoPair
UsdBatchRenderer::GetHitInfo(const SdfPath& sharedId) const
{
	return _selectResults.equal_range(sharedId);
}

void
UsdBatchRenderer::SetLightings(GlfSimpleLightVector const & lights)
{
	_lightingContext->SetLights(lights);
	_lightingContext->SetUseLighting(! lights.empty());

	{
		// Default material for objects
		GlfSimpleMaterial material;
		material.SetAmbient(GfVec4f(0.0f, 0.0f, 0.0f, 1.0f));
		material.SetSpecular(GfVec4f(0.0f, 0.0f, 0.0f, 1.0f));
		material.SetEmission(GfVec4f(0.0f, 0.0f, 0.0f, 1.0f));
		// clamp to 0.0001, since pow(0,0) is undefined in GLSL.
		GLfloat shininess = 0.0001f;
		material.SetShininess(shininess);
		_lightingContext->SetMaterial(material);

		_lightingContext->SetSceneAmbient(GfVec4f(0.0f, 0.0f, 0.0f, 1.0f));
	}
}

PXR_NAMESPACE_CLOSE_SCOPE
