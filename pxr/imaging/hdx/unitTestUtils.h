//
// Copyright 2017 Pixar
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
