//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
//
#ifndef PXR_IMAGING_HDX_UNIT_TEST_UTILS_H
#define PXR_IMAGING_HDX_UNIT_TEST_UTILS_H

#include "pxr/pxr.h"

#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec2i.h"

#include "pxr/imaging/garch/glApi.h"
#include "pxr/imaging/hdx/pickTask.h"
#include "pxr/imaging/hdx/selectionTracker.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HdEngine;
class HdRprimCollection;

namespace HdxUnitTestUtils
{
    HdSelectionSharedPtr TranslateHitsToSelection(
        TfToken const& pickTarget,
        HdSelection::HighlightMode highlightMode,
        HdxPickHitVector const& allHits);

    // For a drag-select from start to end, with given pick radius, what size
    // ID buffer should we ask for?
    GfVec2i CalculatePickResolution(
        GfVec2i const& start, GfVec2i const& end, GfVec2i const& pickRadius);

    GfMatrix4d ComputePickingProjectionMatrix(
        GfVec2i const& start, GfVec2i const& end, GfVec2i const& screen,
        GfFrustum const& viewFrustum);

    class Marquee {
    public:
        Marquee();
        ~Marquee();

        void InitGLResources();
        void DestroyGLResources();
        void Draw(float width, float height, 
                  GfVec2f const& startPos, GfVec2f const& endPos);

    private:
        GLuint _vbo;
        GLuint _program;
    };
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDX_UNIT_TEST_UTILS_H
