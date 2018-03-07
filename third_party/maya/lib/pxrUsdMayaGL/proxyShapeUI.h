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

/// \file proxyShapeUI.h

#include "pxr/pxr.h"
#include "pxrUsdMayaGL/api.h"
#include "pxrUsdMayaGL/shapeAdapter.h"
#include "usdMaya/proxyShape.h"

#include <maya/M3dView.h>
#include <maya/MDrawInfo.h>
#include <maya/MDrawRequest.h>
#include <maya/MDrawRequestQueue.h>
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
        virtual void getDrawRequests(
                const MDrawInfo& drawInfo,
                bool isObjectAndActiveOnly,
                MDrawRequestQueue& requests) override;

        PXRUSDMAYAGL_API
        virtual void draw(
                const MDrawRequest& request,
                M3dView& view) const override;

        PXRUSDMAYAGL_API
        virtual bool select(
                MSelectInfo& selectInfo,
                MSelectionList& selectionList,
                MPointArray& worldSpaceSelectPts) const override;

    private:

        UsdMayaProxyShapeUI();
        UsdMayaProxyShapeUI(const UsdMayaProxyShapeUI&);

        virtual ~UsdMayaProxyShapeUI() override;

        UsdMayaProxyShapeUI& operator=(const UsdMayaProxyShapeUI&);

        // Note that MPxSurfaceShapeUI::select() is declared as const, so we
        // must declare _shapeAdapter as mutable so that we're able to modify
        // it.
        mutable PxrMayaHdShapeAdapter _shapeAdapter;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif // PXRUSDMAYAGL_PROXY_SHAPE_UI_H
