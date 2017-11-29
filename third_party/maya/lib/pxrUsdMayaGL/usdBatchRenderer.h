//
// Copyright 2017 Autodesk
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
///
/// \file usdBatchRenderer.h
///

#ifndef PXRUSDMAYAGL_USDBATCHRENDERER_H
#define PXRUSDMAYAGL_USDBATCHRENDERER_H

#include "pxrUsdMayaGL/usdTaskDelegate.h"
#include "pxr/base/arch/hash.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/imaging/glf/simpleLightingContext.h"
#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdx/intersector.h"
#include "pxr/imaging/hdx/selectionTracker.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEBUG_CODES(
    PXRUSDMAYARENDER_QUEUE_INFO
);

class UsdShapeRenderer;
typedef boost::shared_ptr<class HdxIntersector> HdxIntersectorSharedPtr;

/// \brief This is a singleton object that shapes can hold to get consistent usd
/// batch drawing in Maya in VP2.
///
/// Typical usage is as follows:
///
/// At every refresh, a \c UsdShapeRenderer should be inserted into \c UsdBatchRender
/// that will build the populate queue and the render queue according to \c UsdRenderParams.
///
/// In the draw stage, \c RenderBatches(...) should be called for each draw request to
/// complete the render.
/// In the selection stage, \c RenderSelects(...) should be called before \c GetHitInfo
/// to get the selection intersection info.
/// Note both render and selection should be called only once at the same time stamp.
/// 
class UsdBatchRenderer : private boost::noncopyable
{
	/// \brief The render params is used to distinguish the each render queue.
	struct UsdRenderParams
	{
		// USD Params
		//
		uint8_t refineLevel = 0;
		
		// Geometry Params
        //
		TfTokenVector renderTags;

		// Color Params
		//
		GfVec4f overrideColor = { .0f, .0f, .0f, .0f };

		/// \brief Helper function to find a batch key for the render params
		size_t Hash() const
		{
			size_t hash = (refineLevel << 1);
			boost::hash_combine(hash, overrideColor);
			TF_FOR_ALL(rtIt, renderTags) {
				boost::hash_combine(hash, rtIt->Hash());
			}

			return hash;
		}
	};
public:
    /// \brief Init the BatchRenderer class before using it.  This should be
    /// called at least once and it is OK to call it multiple times.  This
    /// handles things like initializing Gl/Glew.
	PXRUSDMAYAGL_API
    static void Init();
    
	PXRUSDMAYAGL_API
    static UsdBatchRenderer& GetGlobalRenderer() { return _sGlobalRenderer; }
    /// \brief Push the shape renderer into the populate queue
    /// and insert the corresponding render queue according to the render params  
    /// that is set by refineLevel, geometryCol and overrideColor.
	PXRUSDMAYAGL_API
	void InsertRenderQueue(UsdShapeRenderer * renderer, uint8_t refineLevel, TfTokenVector const & renderTags, const GfVec4f& overrideColor);
	/// \brief Deregister the shape renderer from the render queue.
	PXRUSDMAYAGL_API
	void RemoveRenderQueue(UsdShapeRenderer * renderer);
	/// \brief Populate the shapeRenderer queue into the imaging delegate
	/// The queue will be cleared after populated.
	PXRUSDMAYAGL_API
	void PopulateShapeRenderer();

	/// \brief Call to render all queued batches.
	PXRUSDMAYAGL_API
	void RenderBatches(
		TfToken drawRepr,
		HdCullStyle cullStyle,
		const GfMatrix4d& viewMatrix,
		const GfMatrix4d& projectionMatrix,
		const GfVec4d& viewport);

	/// \brief Render the selects buffer for all queued batches.
	/// Tests an object for intersection with a given view.
	PXRUSDMAYAGL_API
	void RenderSelects(
		unsigned int pickResolution,
		bool singleSelection,
		const GfMatrix4d &viewMatrix,
		const GfMatrix4d &projectionMatrix,
		TfToken drawRepr,
		HdCullStyle cullStyle);

	/// \brief Require the Hit info from the select result map.
	/// 
	/// \returns Hydra Hit info for instance associated with \p sharedId
	///
	typedef std::unordered_multimap<SdfPath, HdxIntersector::Hit, SdfPath::Hash> HitBatch;
	typedef std::pair<HitBatch::const_iterator, HitBatch::const_iterator> HitInfoPair;
	PXRUSDMAYAGL_API
	HitInfoPair GetHitInfo(const SdfPath& sharedId) const;

	PXRUSDMAYAGL_API
	HdRenderIndex* GetRenderIndex() const { return _renderIndex; }

	/// \brief update the current time stamp
	/// and return whether is the same time stamp.
	PXRUSDMAYAGL_API
	bool updateRenderTimeStamp(unsigned long long timeStamp);
	PXRUSDMAYAGL_API
	bool updateSelectTimeStamp(unsigned long long timeStamp);

	/// \brief For prim selection stage.
	PXRUSDMAYAGL_API
	void SetSelectionEnable(bool enable) { _taskDelegate->SetSelectionEnable(enable); }
	PXRUSDMAYAGL_API
	void SetSelectionColor(GfVec4f const& color) { _taskDelegate->SetSelectionColor(color); }
	PXRUSDMAYAGL_API
	void SetSelection(HdxSelectionSharedPtr const &selection) { _selTracker->SetSelection(selection); }
	PXRUSDMAYAGL_API
	HdxSelectionSharedPtr GetSelection() const { return _selTracker->GetSelectionMap(); }

	/// \brief Set lighting and shadow params into context.
	PXRUSDMAYAGL_API
	void SetLightings(GlfSimpleLightVector const & lights);
private:

	/// \brief Construct a new, unique BatchRenderer. In almost all cases,
	/// this should not be used -- use \c GlobalBatchRenderer() instead.
	UsdBatchRenderer();

private:
    /// \brief container of all delegates to be populated at next display
    /// refresh.
    std::unordered_set<UsdShapeRenderer *> _populateQueue;
        
	/// \brief Cache of \c SdfPath objects to be rendered
	typedef std::unordered_set<SdfPath, SdfPath::Hash> _SdfPathSet;

	/// \brief Associative pair of \c RenderParams and \c SdfPath objects to be
	/// rendered with said params
	typedef std::pair<UsdRenderParams, _SdfPathSet> _RenderParamSet;

	/// \brief Lookup table to to find \c _RenderParamSet given a param hash key.
	typedef std::unordered_map<size_t, _RenderParamSet> _RendererQueueMap;

	/// \brief container of all batched render calls to be made at next display
	/// refresh.
	_RendererQueueMap _renderQueue;

    /// \brief a cache of all selection results gathered since the last display
    /// refresh.
    HitBatch _selectResults;
    
    /// \brief Master \c UsdImagingGL renderer used to render batches.

    HdEngine _hdEngine;
    HdRenderIndex * _renderIndex;
	HdStRenderDelegate _renderDelegate;
	UsdTaskDelegateSharedPtr _taskDelegate;
    HdxIntersectorSharedPtr _intersector;
	HdxSelectionTrackerSharedPtr _selTracker;
	GlfSimpleLightingContextRefPtr _lightingContext;

	/// \brief a time stamp used to draw only once for batch render.
	unsigned long long _renderTimeStamp;
	/// \brief a time stamp used to draw only once to render select buffer.
	unsigned long long _selectTimeStamp;

    /// \brief Sole global batch renderer used by default.
    static UsdBatchRenderer _sGlobalRenderer;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYAGL_USDBATCHRENDERER_H
