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
#ifndef HDX_UNIT_TEST_UTILS
#define HDX_UNIT_TEST_UTILS

#include "pxr/pxr.h"

#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec2i.h"

#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/hdx/intersector.h"
#include "pxr/imaging/hdx/selectionTracker.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HdEngine;
class HdxIntersector;
class HdRprimCollection;

namespace HdxUnitTestUtils
{
    struct PickParams {
        PickParams() :
             highlightMode(HdSelection::HighlightModeSelect)
           , pickTarget(HdxIntersector::PickPrimsAndInstances)
           , pickThrough(false) {}

    public:
        GfVec2i pickRadius;

        // window/viewport
        int screenWidth;
        int screenHeight;
        GfFrustum viewFrustum;
        GfMatrix4d viewMatrix;

        // hd/hdx
        HdEngine* engine;
        const HdRprimCollection* pickablesCol;
        HdSelection::HighlightMode highlightMode;
        HdxIntersector::PickTarget pickTarget;
        bool pickThrough;
    };

    class Picker {
    public:
        Picker();
        ~Picker();

        void InitIntersector(HdRenderIndex* renderIndex);
        
        void SetPickParams(PickParams const& pParams) {
            _pParams = pParams;
        }
        void SetWidthHeight(int w, int h) {
            _pParams.screenWidth  = w;
            _pParams.screenHeight = h;
        }
        void SetViewFrustum(GfFrustum const& frustum) {
            _pParams.viewFrustum = frustum;
        }
        void SetViewMatrix(GfMatrix4d const& matrix) {
            _pParams.viewMatrix = matrix;
        }
        void SetHighlightMode(HdSelection::HighlightMode mode) {
            _pParams.highlightMode = mode;
        }
        void SetPickTarget(HdxIntersector::PickTarget mode) {
            _pParams.pickTarget = mode;
        }
         void SetPickThrough(bool pickThrough) {
            _pParams.pickThrough = pickThrough;
        }

        void Pick(GfVec2i const& startPos,
                  GfVec2i const& endPos);
        HdxSelectionTrackerSharedPtr GetSelectionTracker() const;
        HdSelectionSharedPtr GetSelection() const;

    private:
        std::unique_ptr<HdxIntersector> _intersector;
        HdxSelectionTrackerSharedPtr _selectionTracker;
        PickParams _pParams;
    };

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

#endif // HDX_UNIT_TEST_UTILS
