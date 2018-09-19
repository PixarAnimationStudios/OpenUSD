//
// Copyright 2018 Pixar
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
#ifndef PXRUSDMAYAGL_USER_DATA_H
#define PXRUSDMAYAGL_USER_DATA_H

/// \file pxrUsdMayaGL/userData.h

#include "pxr/pxr.h"
#include "pxrUsdMayaGL/api.h"

#include "pxr/base/gf/vec4f.h"

#include <maya/MBoundingBox.h>
#include <maya/MUserData.h>

#include <memory>


PXR_NAMESPACE_OPEN_SCOPE


/// Container for all of the information needed for a draw request in the
/// legacy viewport or Viewport 2.0, without requiring shape querying at draw
/// time.
///
/// Maya shapes may implement their own derived classes of this class if they
/// require storage for additional data that's not specific to the batch
/// renderer.
class PxrMayaHdUserData : public MUserData
{
    public:

        bool drawShape;
        std::unique_ptr<MBoundingBox> boundingBox;
        std::unique_ptr<GfVec4f> wireframeColor;

        PXRUSDMAYAGL_API
        PxrMayaHdUserData();

        PXRUSDMAYAGL_API
        ~PxrMayaHdUserData() override;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
