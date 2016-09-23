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
#ifndef __PX_VP20_UTILS_LEGACY_H__
#define __PX_VP20_UTILS_LEGACY_H__

/// \file utils_legacy.h

#include "pxr/base/gf/matrix4d.h"

#include <maya/M3dView.h>

/// This class contains helper methods and utilities to help with the
/// transition from the Maya legacy viewport to Viewport 2.0.
class px_LegacyViewportUtils
{
public:
    /// Get the view and projection matrices used for selection from the given
    /// M3dView \p view.
    static void GetViewSelectionMatrices(M3dView& view,
                                         GfMatrix4d* viewMatrix,
                                         GfMatrix4d* projectionMatrix);

private:
    /// Creating instances of this class is disallowed by making the
    /// constructor private.
    px_LegacyViewportUtils();
    ~px_LegacyViewportUtils();
};


#endif // __PX_VP20_UTILS_LEGACY_H__
