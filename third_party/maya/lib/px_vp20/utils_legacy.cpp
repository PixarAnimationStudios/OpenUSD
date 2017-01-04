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
#include "px_vp20/utils_legacy.h"

#include "pxr/base/gf/matrix4d.h"

#include <maya/M3dView.h>

#include "pxr/imaging/garch/gl.h"


/* static */
void px_LegacyViewportUtils::GetViewSelectionMatrices(
        M3dView& view,
        GfMatrix4d* viewMatrix,
        GfMatrix4d* projectionMatrix)
{
    if (!viewMatrix && !projectionMatrix) {
        return;
    }

    // We need to get the view and projection matrices for the
    // area of the view that the user has clicked or dragged.
    // Unfortunately the M3dView does not give us that in an easy way.
    // If we extract the view and projection matrices from the M3dView object,
    // it is just for the regular camera. MSelectInfo also gives us the
    // selection box, so we could use that to construct the correct view
    // and projection matrixes, but if we call beginSelect on the view as
    // if we were going to use the selection buffer, Maya will do all the
    // work for us and we can just extract the matrices from OpenGL.

    // Hit record can just be one because we are not going to draw
    // anything anyway. We only want the matrices.
    GLuint glHitRecord;
    view.beginSelect(&glHitRecord, 1);

    if (viewMatrix) {
        glGetDoublev(GL_MODELVIEW_MATRIX, viewMatrix->GetArray());
    }
    if (projectionMatrix) {
        glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix->GetArray());
    }

    view.endSelect();
}

