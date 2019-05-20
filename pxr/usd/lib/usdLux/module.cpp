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
#include "pxr/pxr.h"
#include "pxr/usd/usdLux/blackbody.h"
#include "pxr/base/tf/pyModule.h"
#include <boost/python/def.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

TF_WRAP_MODULE
{
    TF_WRAP(UsdLuxTokens);

    boost::python::def("BlackbodyTemperatureAsRgb",
                       UsdLuxBlackbodyTemperatureAsRgb);

    // Generated schema.  Base classes must precede derived classes.
    // Indentation shows class hierarchy.
    TF_WRAP(UsdLuxLight);
    {
        TF_WRAP(UsdLuxCylinderLight);
        TF_WRAP(UsdLuxDiskLight);
        TF_WRAP(UsdLuxDistantLight);
        TF_WRAP(UsdLuxRectLight);
        TF_WRAP(UsdLuxSphereLight);
        TF_WRAP(UsdLuxDomeLight);
        TF_WRAP(UsdLuxGeometryLight);
    }
    TF_WRAP(UsdLuxListAPI);
    TF_WRAP(UsdLuxShapingAPI);
    TF_WRAP(UsdLuxShadowAPI);
    TF_WRAP(UsdLuxLightFilter);
    TF_WRAP(UsdLuxLightPortal);
}
