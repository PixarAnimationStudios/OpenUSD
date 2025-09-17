//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_UNIT_TEST_HELPER_H
#define PXR_IMAGING_HD_UNIT_TEST_HELPER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/unitTestDelegate.h"
#include "pxr/imaging/hd/unitTestNullRenderDelegate.h"

#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/matrix4d.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class Hd_TestDriver
///
/// A unit test driver that exercises the core engine.
///
/// \note This test driver does NOT assume OpenGL is available; in the even
/// that is is not available, all OpenGL calls become no-ops, but all other work
/// is performed as usual.
///
class Hd_TestDriver final
{
public:
    HD_API
    Hd_TestDriver();
    HD_API
    Hd_TestDriver(HdReprSelector const &reprToken);
    HD_API
    ~Hd_TestDriver();

    /// Draw
    HD_API
    void Draw(bool withGuides=false);

    /// Draw with external renderPass
    HD_API
    void Draw(HdRenderPassSharedPtr const &renderPass, bool withGuides);

    /// Set camera to renderpass
    HD_API
    void SetCamera(GfMatrix4d const &viewMatrix,
                   GfMatrix4d const &projectionMatrix,
                   CameraUtilFraming const &framing);

    /// Set cull style
    HD_API
    void SetCullStyle(HdCullStyle cullStyle);

    /// Returns the renderpass
    HD_API
    HdRenderPassSharedPtr const &GetRenderPass();

    /// Returns the renderPassState
    HdRenderPassStateSharedPtr const &GetRenderPassState() const {
        return _renderPassState;
    }

    /// Returns the UnitTest delegate
    HdUnitTestDelegate& GetDelegate() { return *_sceneDelegate; }

    /// Switch repr
    HD_API
    void SetRepr(HdReprSelector const &reprSelector);

private:

    void _Init(HdReprSelector const &reprSelector);

    HdEngine _engine;
    Hd_UnitTestNullRenderDelegate _renderDelegate;
    HdRenderIndex       *_renderIndex;
    HdUnitTestDelegate *_sceneDelegate;
	SdfPath _cameraId;
    HdRenderPassSharedPtr _renderPass;
    HdRenderPassStateSharedPtr _renderPassState;
    HdRprimCollection          _collection;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_UNIT_TEST_HELPER_H
