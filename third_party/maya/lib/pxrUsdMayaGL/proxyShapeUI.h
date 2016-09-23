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
#ifndef PXRUSDMAYAGL_PROXYSHAPEUI_H
#define PXRUSDMAYAGL_PROXYSHAPEUI_H

#include "pxrUsdMayaGL/batchRenderer.h"
#include "usdMaya/proxyShape.h"

#include <maya/M3dView.h>
#include <maya/MDagPath.h>
#include <maya/MDrawInfo.h>
#include <maya/MDrawRequest.h>
#include <maya/MDrawRequestQueue.h>
#include <maya/MPointArray.h>
#include <maya/MPxSurfaceShapeUI.h>
#include <maya/MSelectInfo.h>
#include <maya/MSelectionList.h>


class UsdMayaProxyShapeUI : public MPxSurfaceShapeUI
{
 public:
    /**
     * method to construct node
     */
    static void* creator();

    /**
     * method to handle draw requests
     */
    void getDrawRequests(
        const MDrawInfo& drawInfo,
        bool isObjectAndActiveOnly,
        MDrawRequestQueue& requests);

    /**
     * draw method
     */
    void draw(
        const MDrawRequest& request,
        M3dView& view) const;

    /**
     * select method
     */
    bool select(
        MSelectInfo& selectInfo,
        MSelectionList& selectionList,
        MPointArray& worldSpaceSelectPts) const;

 private:

    /**
     * method to prepare renderer, used in both draw and select...
     */
    UsdMayaGLBatchRenderer::ShapeRenderer* _GetShapeRenderer(
         const MDagPath &objPath,
         bool prepareForQueue) const;

    UsdMayaProxyShapeUI();
    UsdMayaProxyShapeUI(const UsdMayaProxyShapeUI&);

    ~UsdMayaProxyShapeUI();

    UsdMayaProxyShapeUI& operator=(const UsdMayaProxyShapeUI&);
};


#endif // PXRUSDMAYAGL_PROXYSHAPEUI_H
