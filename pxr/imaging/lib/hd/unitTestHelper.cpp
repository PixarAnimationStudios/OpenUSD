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
#include "pxr/imaging/hd/unitTestHelper.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/glf/diagnostic.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/frustum.h"
#include "pxr/base/tf/getenv.h"

#include <string>

template <typename T>
static VtArray<T>
_BuildArray(T values[], int numValues)
{
    VtArray<T> result(numValues);
    std::copy(values, values+numValues, result.begin());
    return result;
}

Hd_TestDriver::Hd_TestDriver()
    : _renderPassState(new HdRenderPassState())
{
    TfToken reprName = HdTokens->hull;
    if (TfGetenv("HD_ENABLE_SMOOTH_NORMALS", "CPU") == "CPU" or
        TfGetenv("HD_ENABLE_SMOOTH_NORMALS", "CPU") == "GPU") {
        reprName = HdTokens->smoothHull;
    }
    _Init(reprName);
}

Hd_TestDriver::Hd_TestDriver(TfToken const &reprName)
    : _renderPassState(new HdRenderPassState())
{
    _Init(reprName);
}

void
Hd_TestDriver::_Init(TfToken const &reprName)
{
    _reprName = reprName;

    GfMatrix4d viewMatrix = GfMatrix4d().SetIdentity();
    viewMatrix *= GfMatrix4d().SetTranslate(GfVec3d(0.0, 1000.0, 0.0));
    viewMatrix *= GfMatrix4d().SetRotate(GfRotation(GfVec3d(1.0, 0.0, 0.0), -90.0));

    GfFrustum frustum;
    frustum.SetPerspective(45, true, 1, 1.0, 10000.0);
    GfMatrix4d projMatrix = frustum.ComputeProjectionMatrix();

    SetCamera(viewMatrix, projMatrix, GfVec4d(0, 0, 512, 512));

    // set depthfunc to GL default
    _renderPassState->SetDepthFunc(HdCmpFuncLess);
}

void
Hd_TestDriver::Draw(bool withGuides)
{
    Draw(GetRenderPass(withGuides));
}

void
Hd_TestDriver::Draw(HdRenderPassSharedPtr const &renderPass)
{
    _engine.Draw(_delegate.GetRenderIndex(), renderPass, _renderPassState);

    GLF_POST_PENDING_GL_ERRORS();
}

void
Hd_TestDriver::SetCamera(GfMatrix4d const &modelViewMatrix,
                         GfMatrix4d const &projectionMatrix,
                         GfVec4d const &viewport)
{
    _renderPassState->SetCamera(modelViewMatrix,
                                projectionMatrix,
                                viewport);
}

void
Hd_TestDriver::SetCullStyle(HdCullStyle cullStyle)
{
    _renderPassState->SetCullStyle(cullStyle);
}

HdRenderPassSharedPtr const &
Hd_TestDriver::GetRenderPass(bool withGuides)
{
    if (withGuides) {
        if (not _geomAndGuidePass) 
            _geomAndGuidePass = HdRenderPassSharedPtr(
                new HdRenderPass(&_delegate.GetRenderIndex(),
                                 HdRprimCollection(
                                     Hd_UnitTestTokens->geometryAndGuides,
                                     _reprName)));
        return _geomAndGuidePass;
    } else {
        if (not _geomPass)
            _geomPass = HdRenderPassSharedPtr(
                new HdRenderPass(&_delegate.GetRenderIndex(),
                                 HdRprimCollection(
                                     HdTokens->geometry,
                                     _reprName)));
        return _geomPass;
    }
}

void
Hd_TestDriver::SetRepr(TfToken const &reprName)
{
    _reprName = reprName;

    if (_geomAndGuidePass) {
        _geomAndGuidePass->SetRprimCollection(
            HdRprimCollection(Hd_UnitTestTokens->geometryAndGuides, _reprName));
    }
    if (_geomPass) {
        _geomPass->SetRprimCollection(
            HdRprimCollection(HdTokens->geometry, _reprName));
    }
}
