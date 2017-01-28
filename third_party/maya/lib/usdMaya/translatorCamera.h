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
#ifndef PXRUSDMAYA_TRANSLATOR_CAMERA_H
#define PXRUSDMAYA_TRANSLATOR_CAMERA_H

#include "usdMaya/primReaderArgs.h"
#include "usdMaya/primReaderContext.h"

#include "pxr/usd/usdGeom/camera.h"

#include <maya/MFnCamera.h>
#include <maya/MObject.h>

/// \brief Provides helper functions for translating to/from UsdGeomCamera
struct PxrUsdMayaTranslatorCamera
{
    /// Reads a UsdGeomCamera \p usdCamera from USD and creates a Maya
    /// MFnCamera under \p parentNode.
    static bool Read(
            const UsdGeomCamera& usdCamera,
            MObject parentNode,
            const PxrUsdMayaPrimReaderArgs& args,
            PxrUsdMayaPrimReaderContext* context);

    /// Helper function to access just the logic that writes from a non-animated
    /// camera into an existing maya camera.
    static bool ReadToCamera(
            const UsdGeomCamera& usdCamera,
            MFnCamera& cameraObject);

};


#endif // PXRUSDMAYA_TRANSLATOR_CAMERA_H
