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
#ifndef PXRUSDMAYAGL_PROXY_SHAPE_UI_H
#define PXRUSDMAYAGL_PROXY_SHAPE_UI_H

/// \file pxrUsdMayaGL/proxyShapeUI.h

#include "pxr/pxr.h"
#include "pxrUsdMayaGL/api.h"
#include "pxrUsdMayaGL/usdProxyShapeAdapter.h"

#include <maya/M3dView.h>
#include <maya/MDrawInfo.h>
#include <maya/MDrawRequest.h>
#include <maya/MDrawRequestQueue.h>
#include <maya/MMessage.h>
#include <maya/MObject.h>
#include <maya/MPointArray.h>
#include <maya/MPxSurfaceShapeUI.h>
#include <maya/MSelectInfo.h>
#include <maya/MSelectionList.h>


PXR_NAMESPACE_OPEN_SCOPE


class UsdMayaProxyShapeUI : public MPxSurfaceShapeUI
{
    public:

        PXRUSDMAYAGL_API
        static void* creator();

        PXRUSDMAYAGL_API
        void getDrawRequests(
                const MDrawInfo& drawInfo,
                bool objectAndActiveOnly,
                MDrawRequestQueue& requests) override;

        PXRUSDMAYAGL_API
        void draw(
                const MDrawRequest& request,
                M3dView& view) const override;

        PXRUSDMAYAGL_API
        bool select(
                MSelectInfo& selectInfo,
                MSelectionList& selectionList,
                MPointArray& worldSpaceSelectPts) const override;

    private:

        UsdMayaProxyShapeUI();
        ~UsdMayaProxyShapeUI() override;

        UsdMayaProxyShapeUI(const UsdMayaProxyShapeUI&) = delete;
        UsdMayaProxyShapeUI& operator=(const UsdMayaProxyShapeUI&) = delete;

        // Note that MPxSurfaceShapeUI::select() is declared as const, so we
        // must declare _shapeAdapter as mutable so that we're able to modify
        // it.
        mutable PxrMayaHdUsdProxyShapeAdapter _shapeAdapter;

        // In Viewport 2.0, the MPxDrawOverride destructor is called when its
        // shape is deleted, in which case the shape's adapter is removed from
        // the batch renderer. In the legacy viewport though, that's not the
        // case. The MPxSurfaceShapeUI destructor may not get called until the
        // scene is closed or Maya exits. As a result, MPxSurfaceShapeUI
        // objects must listen for node removal messages from Maya and remove
        // their shape adapter from the batch renderer if their node is the one
        // being removed. Otherwise, deleted shapes may still be drawn.
        static void _OnNodeRemoved(MObject& node, void* clientData);

        MCallbackId _onNodeRemovedCallbackId;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
