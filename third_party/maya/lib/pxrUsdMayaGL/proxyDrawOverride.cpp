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
#include "pxrUsdMayaGL/proxyDrawOverride.h"
#include "pxrUsdMayaGL/usdBatchRenderer.h"

#include <maya/MSelectionContext.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MBoundingBox.h>
#include <maya/MObjectHandle.h>
#include <maya/MDagPath.h>
#include <maya/MDrawContext.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFrameContext.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MPxDrawOverride.h>
#include <maya/MString.h>
#include <maya/MUserData.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
	_tokens,
	(render)
);

MString UsdMayaProxyDrawOverride::sm_drawDbClassification("drawdb/geometry/usdMaya");
MString UsdMayaProxyDrawOverride::sm_drawRegistrantId("pxrUsdPlugin");


/// \brief struct to hold all the information needed for a 
/// draw request in vp2, without requiring shape querying at
/// draw time. 
class DrawUserData : public MUserData
{
public:
	// The Draw User Data are prepared for 
	// the active draw which is not batch render.
	// Now the active draw is just bounding box.
	bool _isSelected;
	MBoundingBox _bounds;
	GfVec4f _wireframeColor;

public:
	// Constructor to hold the info for bounding box draw.
	DrawUserData(bool selected, const MBoundingBox &bounds, const GfVec4f &wireFrameColor)
		: MUserData(false)
		, _isSelected(selected)
		, _bounds(bounds)
		, _wireframeColor(wireFrameColor) {}

	// Make sure everything gets freed!
	~DrawUserData() {}
};


static inline UsdBatchRenderer& _GetBatchRenderer()
{
    return UsdBatchRenderer::GetGlobalRenderer();
}


UsdMayaProxyDrawOverride::UsdMayaProxyDrawOverride(
    const MObject& obj)
	: MHWRender::MPxDrawOverride(obj, UsdMayaProxyDrawOverride::draw, /*bool isAlwaysDirty =*/ false)
{
}

UsdMayaProxyDrawOverride::~UsdMayaProxyDrawOverride()
{
	// Deregister the shape renderer from the render queue.
	_GetBatchRenderer().RemoveRenderQueue(&_shapeRenderer);
}

/* static */
MHWRender::MPxDrawOverride* 
UsdMayaProxyDrawOverride::Creator(const MObject& obj)
{
    UsdBatchRenderer::Init();
    return new UsdMayaProxyDrawOverride(obj);
}

MHWRender::DrawAPI
UsdMayaProxyDrawOverride::supportedDrawAPIs() const
{
	// Support GL and GLCoreProfile
	return (MHWRender::kOpenGL | MHWRender::kOpenGLCoreProfile);
}

bool
UsdMayaProxyDrawOverride::isBounded(
    const MDagPath& objPath,
    const MDagPath& /* cameraPath */) const
{
    return UsdMayaIsBoundingBoxModeEnabled();
}

MBoundingBox
UsdMayaProxyDrawOverride::boundingBox(
    const MDagPath& objPath,
    const MDagPath& /* cameraPath */) const
{
    UsdMayaProxyShape* pShape = getShape(objPath);
    if (!pShape)
    {
        return MBoundingBox();
    }
    return pShape->boundingBox();
}



UsdMayaProxyShape*
UsdMayaProxyDrawOverride::getShape(const MDagPath& objPath)
{
    MObject obj = objPath.node();
    MFnDependencyNode dnNode(obj);
    if (obj.apiType() != MFn::kPluginShape) {
        MGlobal::displayError("Failed apiType test (apiTypeStr=" +
                              MString(obj.apiTypeStr()) + ")");
        return NULL;
    }

    UsdMayaProxyShape* pShape = static_cast<UsdMayaProxyShape*>(dnNode.userNode());
    if (!pShape) {
        MGlobal::displayError("Failed getting userNode");
        return NULL;
    }

    return pShape;
}

MMatrix 
UsdMayaProxyDrawOverride::transform(
	const MDagPath& objPath,
	const MDagPath& cameraPath) const
{
	MStatus status;
	MMatrix transform = objPath.inclusiveMatrix(&status);

	// Set transform matrix in the shape delegate
	const_cast<UsdShapeRenderer&>(_shapeRenderer).SetTransform(GfMatrix4d(transform.matrix));

	return transform;
}

MUserData* 
UsdMayaProxyDrawOverride::prepareForDraw(
    const MDagPath& objPath,
    const MDagPath& /* cameraPath */,
    const MHWRender::MFrameContext& frameContext,
    MUserData* userData ) 
{
    UsdMayaProxyShape* shape = getShape(objPath);
    
	// Get all the data from the shape.
    UsdPrim usdPrim;
    SdfPathVector excludePaths;
    UsdTimeCode timeCode;
    int subdLevel;
    bool showGuides, showRenderGuides;
    bool tint;
    GfVec4f tintColor;
    if( !shape->GetAllRenderAttributes(
                    &usdPrim, &excludePaths, &subdLevel, &timeCode,
                    &showGuides, &showRenderGuides,
                    &tint, &tintColor ) )
    {
        return NULL;
    }

	// XXX Not yet adding ability to turn off display of proxy geometry, but
    // we should at some point, as in usdview
    TfTokenVector renderTags;
    renderTags.push_back(HdTokens->geometry);
    renderTags.push_back(HdTokens->proxy);
    if (showGuides) {
        renderTags.push_back(HdTokens->guide);
    } 
    if (showRenderGuides) {
        renderTags.push_back(_tokens->render);
    }

	// Set the data into the shape renderer.
	_shapeRenderer.PrepareForDelegate(
		_GetBatchRenderer().GetRenderIndex(),
		MObjectHandle(objPath.transform()).hashCode(),
		usdPrim,
		excludePaths,
		timeCode,
		subdLevel
		);
	// Push the shape renderer into the batch render queue.
	_GetBatchRenderer().InsertRenderQueue(
		&_shapeRenderer,
		subdLevel,
		renderTags,
		tint ? tintColor : GfVec4f(.0f, .0f, .0f, .0f)
		);

	//
	bool enableHighlightSelection = true; // renderParams.highlight;
	_GetBatchRenderer().SetSelectionEnable(enableHighlightSelection);
	_GetBatchRenderer().SetSelectionColor(GfVec4f(1, 1, 0, 1));

	MStatus status;
	MMatrix transform = objPath.inclusiveMatrix(&status);
	// Set transform matrix in the shape delegate
	_shapeRenderer.SetTransform(GfMatrix4d(transform.matrix));
    
	// Get the wireframe color from the displayStatus.
	const MHWRender::DisplayStatus& displayStatus = MHWRender::MGeometryUtilities::displayStatus(objPath);
	const MColor& wireframeColor =
		MHWRender::MGeometryUtilities::wireframeColor(objPath);
	bool isSelected =
		(displayStatus == MHWRender::kActive) ||
		(displayStatus == MHWRender::kLead) ||
		(displayStatus == MHWRender::kHilite);

	// Build DrawUserData for the draw which is not batch render.
	userData = new DrawUserData(
		isSelected,
		shape->boundingBox(),
		GfVec4f(
			wireframeColor.r,
			wireframeColor.g,
			wireframeColor.b,
			1.f)
		);
    return userData;
}

static void
drawBox(GLfloat size, GLenum type)
{
	static const GLfloat n[6][3] =
	{
		{ -1.0, 0.0, 0.0 },
		{ 0.0, 1.0, 0.0 },
		{ 1.0, 0.0, 0.0 },
		{ 0.0, -1.0, 0.0 },
		{ 0.0, 0.0, 1.0 },
		{ 0.0, 0.0, -1.0 }
	};
	static const GLint faces[6][4] =
	{
		{ 0, 1, 2, 3 },
		{ 3, 2, 6, 7 },
		{ 7, 6, 5, 4 },
		{ 4, 5, 1, 0 },
		{ 5, 6, 2, 1 },
		{ 7, 4, 0, 3 }
	};
	GLfloat v[8][3];
	GLint i;

	v[0][0] = v[1][0] = v[2][0] = v[3][0] = -size / 2;
	v[4][0] = v[5][0] = v[6][0] = v[7][0] = size / 2;
	v[0][1] = v[1][1] = v[4][1] = v[5][1] = -size / 2;
	v[2][1] = v[3][1] = v[6][1] = v[7][1] = size / 2;
	v[0][2] = v[3][2] = v[4][2] = v[7][2] = -size / 2;
	v[1][2] = v[2][2] = v[5][2] = v[6][2] = size / 2;

	for (i = 5; i >= 0; i--) {
		glBegin(type);
		glNormal3fv(&n[i][0]);
		glVertex3fv(&v[faces[i][0]][0]);
		glVertex3fv(&v[faces[i][1]][0]);
		glVertex3fv(&v[faces[i][2]][0]);
		glVertex3fv(&v[faces[i][3]][0]);
		glEnd();
	}
}

void
UsdMayaProxyDrawOverride::_RenderBounds(
	const MBoundingBox &bounds,
	const GfVec4f &wireframeColor,
	const MMatrix& worldViewMat,
	const MMatrix& projectionMat)
{
	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LIGHTING_BIT);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadMatrixd(projectionMat.matrix[0]);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadMatrixd(worldViewMat.matrix[0]);

	glColor4fv((float*)&wireframeColor);
	glTranslated(bounds.center()[0],
		bounds.center()[1],
		bounds.center()[2]);
	glScaled(bounds.width(),
		bounds.height(),
		bounds.depth());
	//glutWireCube(1.0);
	drawBox(1.0, GL_LINE_LOOP);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib(); // GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LIGHTING_BIT
}

void
UsdMayaProxyDrawOverride::_SetupLighting(
	const MHWRender::MDrawContext& context,
	bool enableLighting,
	bool enableShadow
	)
{
	GlfSimpleLightVector lights;

	MStatus status;
	const unsigned int nbLights = enableLighting ? 
		std::min(context.numberOfActiveLights(&status), 8u) // Take into account only the 8 lights supported by the basic OpenGL profile.
		: 0;
	lights.reserve(nbLights);

	if (status == MStatus::kSuccess)
	{
		GlfSimpleLight light;
		for (unsigned int i = 0; i < nbLights; ++i) {
			MFloatVector direction;
			float intensity;
			MColor color;
			bool hasDirection;
			bool hasPosition;
#if MAYA_API_VERSION >= 201300
			// Starting with Maya 2013, getLightInformation() uses MFloatPointArray for positions
			MFloatPointArray positions;
			status = context.getLightInformation(
				i, positions, direction, intensity, color,
				hasDirection, hasPosition);
			const MFloatPoint &position = positions[0];
#else 
			// Maya 2012, getLightInformation() uses MFloatPoint for position
			MFloatPoint position;
			status = context.getLightInformation(
				i, position, direction, intensity, color,
				hasDirection, hasPosition);
#endif
			if (status != MStatus::kSuccess) continue;

			if (hasDirection) {
				if (hasPosition) {
					// Assumes a Maya Spot Light!
					light.SetPosition(
						GfVec4f(position[0], position[1], position[2], 1.0f));
					light.SetSpotDirection(
						GfVec3f(direction[0], direction[1], direction[2]));
					// Maya's default value's for spot lights.
					light.SetSpotCutoff(20.0);
					light.SetSpotFalloff(0.0);
					light.SetDiffuse(
						GfVec4f(intensity * color[0], intensity * color[1], intensity * color[2], 1.0f));
					light.SetAmbient(
						GfVec4f(0.0f, 0.0f, 0.0f, 1.0f));
					light.SetSpecular(
						GfVec4f(1.0f, 1.0f, 1.0f, 1.0f));
					// Spot light support shadow.
					light.SetHasShadow(enableShadow);
				}
				else {
					// Assumes a Maya Directional Light!
					light.SetPosition(
						GfVec4f(-direction[0], -direction[1], -direction[2], 0.0f));
					light.SetSpotCutoff(180.0);
					light.SetDiffuse(
						GfVec4f(intensity * color[0], intensity * color[1], intensity * color[2], 1.0f));
					light.SetAmbient(
						GfVec4f(0.0f, 0.0f, 0.0f, 1.0f));
					// Directional light support shadow.
					light.SetHasShadow(enableShadow);
				}
			}
			else if (hasPosition) {
				// Assumes a Maya Point Light!
				light.SetPosition(
					GfVec4f(position[0], position[1], position[2], 1.0f));
				// Maya's default value's for spot lights.
				light.SetSpotCutoff(180.0);
				light.SetDiffuse(
					GfVec4f(intensity * color[0], intensity * color[1], intensity * color[2], 1.0f));
				light.SetAmbient(
					GfVec4f(0.0f, 0.0f, 0.0f, 1.0f));
			}
			else {
				// Assumes a Maya Ambient Light!
				light.SetPosition(
					GfVec4f(0.0f, 0.0f, 0.0f, 1.0f));
				light.SetSpotCutoff(180.0);
				light.SetDiffuse(
					GfVec4f(0.0f, 0.0f, 0.0f, 1.0f));
				light.SetAmbient(
					GfVec4f(intensity * color[0], intensity * color[1], intensity * color[2], 1.0f));
			}

			lights.push_back(light);
		}
	}

	_GetBatchRenderer().SetLightings(lights);
}

void
UsdMayaProxyDrawOverride::_RenderShapes(
	const MHWRender::MDrawContext& context,
	TfToken drawRepr,
	bool enableLighting,
	HdCullStyle cullStyle)
{
	// Draw only once for batch render in the same time stamp.
	if (! _GetBatchRenderer().updateRenderTimeStamp(context.getFrameStamp()))
		return;

	// Get the world/view/projection matrix.
	MStatus status;
	MMatrix projectionMat = context.getMatrix(MHWRender::MDrawContext::kProjectionMtx, &status);
	MMatrix viewMat = context.getMatrix(MHWRender::MDrawContext::kViewMtx, &status);
	if (status != MStatus::kSuccess) return;

	// Extract camera settings from maya view
	int viewX, viewY, viewWidth, viewHeight;
	context.getViewportDimensions(viewX, viewY, viewWidth, viewHeight);
	GfVec4d viewport(viewX, viewY, viewWidth, viewHeight);

	// Setup lighting and shadow before batch render.
	bool enableShadow = false; // renderParams.shadow;
	_SetupLighting(context, enableLighting, enableShadow);

	// Batch render
	//
	_GetBatchRenderer().RenderBatches(
		drawRepr,
		cullStyle,
		GfMatrix4d(viewMat.matrix),
		GfMatrix4d(projectionMat.matrix),
		viewport
	);
}

void
UsdMayaProxyDrawOverride::draw(const MHWRender::MDrawContext& context, const MUserData *data)
{
	// Draw callback
	//
	MHWRender::MRenderer* theRenderer = MHWRender::MRenderer::theRenderer();
	if (!theRenderer || !theRenderer->drawAPIIsOpenGL())
		return;

	// Populate the shape renderer queue that would be cleared after populated.
	//
	_GetBatchRenderer().PopulateShapeRenderer();

	bool drawShape = true;
	bool enableLighting = true;
	TfToken drawRepr = HdTokens->refined;
	const unsigned int& displayStyle = context.getDisplayStyle();

	// Maya 2016 SP2 lacks MHWRender::MFrameContext::DisplayStyle::kBackfaceCulling for whatever reason...
	HdCullStyle cullStyle = HdCullStyleNothing;
#if MAYA_API_VERSION >= 201603
	if (displayStyle & MHWRender::MFrameContext::DisplayStyle::kBackfaceCulling)
		cullStyle = HdCullStyleBackUnlessDoubleSided;
#endif

	// Maya 2015 lacks MHWRender::MFrameContext::DisplayStyle::kFlatShaded for whatever reason...
	bool flatShaded =
#if MAYA_API_VERSION >= 201600
		displayStyle & MHWRender::MFrameContext::DisplayStyle::kFlatShaded;
#else
		false;
#endif

	if (flatShaded)
	{
		drawRepr = HdTokens->hull;
	}
	else if (displayStyle & MHWRender::MFrameContext::DisplayStyle::kGouraudShaded)
	{
		if (displayStyle & MHWRender::MFrameContext::DisplayStyle::kWireFrame)
			drawRepr = HdTokens->refinedWireOnSurf;
		else
			drawRepr = HdTokens->refined;
	}
	else if (displayStyle & MHWRender::MFrameContext::DisplayStyle::kWireFrame)
	{
		drawRepr = HdTokens->refinedWire;
		enableLighting = false;
	}
	else
	{
		drawShape = false;
	}

	if (drawShape)
	{
		// Batch render the shapes in the render queue.
		_RenderShapes(context, drawRepr, enableLighting, cullStyle);
	}
	
	// Draw the bounding Box for the single hilite shape.
	//
	const DrawUserData* drawData = static_cast<const DrawUserData*>(data);
	if (drawData)
	{
		if (drawData->_isSelected ||
			(! UsdMayaIsBoundingBoxModeEnabled() && (displayStyle & MHWRender::MFrameContext::DisplayStyle::kBoundingBox)))
		{
			MStatus status;
			MMatrix projectionMat = context.getMatrix(MHWRender::MDrawContext::kProjectionMtx, &status);
			MMatrix worldViewMat = context.getMatrix(MHWRender::MDrawContext::kWorldViewMtx, &status);
			_RenderBounds(
				drawData->_bounds, drawData->_wireframeColor, worldViewMat, projectionMat);
		}
	}
}

void UsdMayaProxyDrawOverride::_RenderSelects(
	MHWRender::MSelectionInfo& selectInfo,
	const MHWRender::MDrawContext& context)
{
	TfToken drawRepr = HdTokens->refined;
	const unsigned int& displayStyle = context.getDisplayStyle();

	// Maya 2015 lacks MHWRender::MFrameContext::DisplayStyle::kFlatShaded for whatever reason...
	bool flatShaded =
#if MAYA_API_VERSION >= 201600
		displayStyle & MHWRender::MFrameContext::DisplayStyle::kFlatShaded;
#else
		false;
#endif

	if (flatShaded)
	{
		drawRepr = HdTokens->hull;
	}
	else if (displayStyle & MHWRender::MFrameContext::DisplayStyle::kGouraudShaded)
	{
		if (displayStyle & MHWRender::MFrameContext::DisplayStyle::kWireFrame)
			drawRepr = HdTokens->refinedWireOnSurf;
		else
			drawRepr = HdTokens->refined;
	}
	else if (displayStyle & MHWRender::MFrameContext::DisplayStyle::kWireFrame)
	{
		drawRepr = HdTokens->refinedWire;
	}
	// Maya 2016 SP2 lacks MHWRender::MFrameContext::DisplayStyle::kBackfaceCulling for whatever reason...
	HdCullStyle cullStyle = HdCullStyleNothing;
#if MAYA_API_VERSION >= 201603
	if (displayStyle & MHWRender::MFrameContext::DisplayStyle::kBackfaceCulling)
		cullStyle = HdCullStyleBackUnlessDoubleSided;
#endif

	// We need to get the view and projection matrices for the
	// area of the view that the user has clicked or dragged.
	// Unfortunately the view does not give us that in an easy way..
	// If we extract the view and projection matrices from the view object,
	// it is just for the regular camera. The selectInfo also gives us the
	// selection box, so we could use that to construct the correct view
	// and projection matrices, but if we call beginSelect on the view as
	// if we were going to use the selection buffer, maya will do all the
	// work for us and we can just extract the matrices from opengl.
	MStatus status;
	MMatrix viewMat = context.getMatrix(MHWRender::MDrawContext::kViewMtx, &status);
	MMatrix projectionMat = context.getMatrix(MHWRender::MDrawContext::kProjectionMtx, &status);
	unsigned int sel_x, sel_y, sel_width, sel_height;
	selectInfo.selectRect(sel_x, sel_y, sel_width, sel_height);
	int scr_x, scr_y, scr_width, scr_height;
	context.getViewportDimensions(scr_x, scr_y, scr_width, scr_height);
	MMatrix selectMatrix;
	selectMatrix.setToIdentity();
	selectMatrix[0][0] = (double)scr_width / sel_width;
	selectMatrix[1][1] = (double)scr_height / sel_height;
	selectMatrix[3][0] = (scr_width - (double)(sel_x * 2 + sel_width)) / sel_width;
	selectMatrix[3][1] = (scr_height - (double)(sel_y * 2 + sel_height)) / sel_height;
	projectionMat = projectionMat * selectMatrix;

	// We will miss very small objects with this setting, but it's faster.
	const unsigned int pickResolution = 256;

	// Batch render the select buffer for the render queue.
	_GetBatchRenderer().RenderSelects(
			pickResolution,
			selectInfo.singleSelection(),
			GfMatrix4d(viewMat.matrix),
			GfMatrix4d(projectionMat.matrix),
			drawRepr,
			cullStyle
		);
}

#if MAYA_API_VERSION >= 20180000
bool UsdMayaProxyDrawOverride::wantUserSelection() const
{
	MHWRender::MRenderer* theRenderer = MHWRender::MRenderer::theRenderer();
	// Perform user selection code when the viewport renderer is in legacy OpenGL mode(to make sure OpenGL pick mode is available)
	return theRenderer->drawAPIIsOpenGL() && (theRenderer->drawAPI() == MHWRender::kOpenGL || theRenderer->drawAPI() == MHWRender::kOpenGLCoreProfile);
}

bool UsdMayaProxyDrawOverride::userSelect(MHWRender::MSelectionInfo& selectInfo, const MHWRender::MDrawContext& context, MPoint& hitPoint, const MUserData* data)
{
	// Draw only once for batch render in the same time stamp.
	//
	if (_GetBatchRenderer().updateSelectTimeStamp(context.getFrameStamp()))
	{
		// Batch render the selects buffer in the render queue.
		_RenderSelects(selectInfo, context);
	}

	HdxSelectionSharedPtr primSelection(new HdxSelection);
    HdxSelectionHighlightMode mode = HdxSelectionHighlightModeSelect;
    
	// Require the hit info from the select result map.
	UsdBatchRenderer::HitInfoPair hitPos = _GetBatchRenderer().GetHitInfo( _shapeRenderer.GetSdfPath() );
	if (hitPos.first == hitPos.second)
	{
		// Clear prim selection.
		UsdBatchRenderer::GetGlobalRenderer().SetSelection(primSelection);
		return false;
	}
	// Set Multiply prim selection.
	for (auto m = hitPos.first; m != hitPos.second; m++)
	{
		primSelection->AddInstance(mode, m->second.objectId);

		if (TfDebug::IsEnabled(PXRUSDMAYARENDER_QUEUE_INFO))
		{
			cout << "FOUND HIT: " << endl;
			cout << "\tdelegateId: " << m->second.delegateId << endl;
			cout << "\tobjectId: " << m->second.objectId << endl;
			cout << "\tndcDepth: " << m->second.ndcDepth << endl;
		}
	}
	UsdBatchRenderer::GetGlobalRenderer().SetSelection(primSelection);

	hitPoint = MPoint(hitPos.first->second.worldSpaceHitPoint.data());

	return true;
}
#endif

PXR_NAMESPACE_CLOSE_SCOPE
