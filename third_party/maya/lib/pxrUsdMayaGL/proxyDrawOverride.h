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

#include "pxr/usd/usd/stage.h"
#include "pxr/usdImaging/usdImaging/gl.h"
#include "pxrUsdMayaGL/batchRenderer.h"

#include "usdMaya/proxyShape.h"

#include <maya/MBoundingBox.h>
#include <maya/MDagPath.h>
#include <maya/MDrawContext.h>
#include <maya/MFrameContext.h>
#include <maya/MObject.h>
#include <maya/MPxDrawOverride.h>
#include <maya/MString.h>
#include <maya/MUserData.h>


class UsdMayaProxyDrawOverride : public MHWRender::MPxDrawOverride
{
public:
    static MHWRender::MPxDrawOverride* Creator(const MObject& obj);

    virtual ~UsdMayaProxyDrawOverride();

    virtual bool isBounded(
        const MDagPath& objPath,
        const MDagPath& cameraPath) const;

    virtual MBoundingBox boundingBox(
        const MDagPath& objPath,
        const MDagPath& cameraPath) const;

    virtual MUserData* prepareForDraw(
        const MDagPath& objPath,
        const MDagPath& cameraPath,
        const MHWRender::MFrameContext& frameContext,
        MUserData* oldData);

    static MString sm_drawDbClassification;
    static MString sm_drawRegistrantId;

    static void draw(const MHWRender::MDrawContext& context, const MUserData* data);

    static UsdMayaProxyShape* getShape(const MDagPath& objPath);

private:
    UsdMayaProxyDrawOverride(const MObject& obj);
};


#endif // PXRUSDMAYAGL_PROXYDRAWOVERRIDE_H
