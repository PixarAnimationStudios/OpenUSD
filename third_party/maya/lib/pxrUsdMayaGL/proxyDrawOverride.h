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
#ifndef PXRUSDMAYAGL_PROXYDRAWOVERRIDE_H
#define PXRUSDMAYAGL_PROXYDRAWOVERRIDE_H

#include "pxrUsdMayaGL/usdShapeRenderer.h"
#include "usdMaya/proxyShape.h"

#include <maya/MPxDrawOverride.h>

PXR_NAMESPACE_OPEN_SCOPE



class UsdMayaProxyDrawOverride : public MHWRender::MPxDrawOverride
{
public:
    PXRUSDMAYAGL_API
    static MHWRender::MPxDrawOverride* Creator(const MObject& obj);

	PXRUSDMAYAGL_API
    virtual ~UsdMayaProxyDrawOverride();

	PXRUSDMAYAGL_API
	virtual MHWRender::DrawAPI supportedDrawAPIs() const override;

	PXRUSDMAYAGL_API
	virtual MMatrix transform(
		const MDagPath& objPath,
		const MDagPath& cameraPath) const override;

	PXRUSDMAYAGL_API
    virtual bool isBounded(
        const MDagPath& objPath,
        const MDagPath& cameraPath) const override;

	PXRUSDMAYAGL_API
    virtual MBoundingBox boundingBox(
        const MDagPath& objPath,
        const MDagPath& cameraPath) const override;

	PXRUSDMAYAGL_API
    virtual MUserData* prepareForDraw(
        const MDagPath& objPath,
        const MDagPath& cameraPath,
        const MHWRender::MFrameContext& frameContext,
        MUserData* oldData) override;

    PXRUSDMAYAGL_API
    virtual MHWRender::DrawAPI supportedDrawAPIs() const;

#if MAYA_API_VERSION >= 20180000
	PXRUSDMAYAGL_API
	virtual bool wantUserSelection() const override;
	PXRUSDMAYAGL_API
	virtual bool userSelect(MHWRender::MSelectionInfo& selectInfo, const MHWRender::MDrawContext& context, MPoint& hitPoint, const MUserData* data) override;
#endif

	PXRUSDMAYAGL_API
    static MString sm_drawDbClassification;
	PXRUSDMAYAGL_API
    static MString sm_drawRegistrantId;

	PXRUSDMAYAGL_API
    static void draw(const MHWRender::MDrawContext& context, const MUserData* data);

	PXRUSDMAYAGL_API
    static UsdMayaProxyShape* getShape(const MDagPath& objPath);

private:
    UsdMayaProxyDrawOverride(const MObject& obj);

	/// \brief Setup lighting context in the lighting task.
	static
	void _SetupLighting(
			const MHWRender::MDrawContext& context,
			bool enableLighting,
			bool enableShadow);

	/// \brief Render the shapes in the render queue.
	static
	void _RenderShapes(
		const MHWRender::MDrawContext& context,
		TfToken drawRepr,
		bool enableLighting,
		HdCullStyle cullStyle);

	/// \brief Render specific object's bounds.
	static
	void _RenderBounds(
		const MBoundingBox& bounds,
		const GfVec4f& wireframeColor,
		const MMatrix& worldViewMat,
		const MMatrix& projectionMat);

	/// \brief Render the selects buffer in the render queue.
	void _RenderSelects(
		MHWRender::MSelectionInfo& selectInfo,
		const MHWRender::MDrawContext& context);

	/// \brief Cache the shape delegate for render.
	UsdShapeRenderer _shapeRenderer;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYAGL_PROXYDRAWOVERRIDE_H
