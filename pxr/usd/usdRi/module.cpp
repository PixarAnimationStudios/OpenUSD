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
#include "pxr/pxr.h"
#include "pxr/base/tf/pyModule.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_WRAP_MODULE
{
    // Base classes must precede derived classes.  Indentation indicates class
    // hierarchy.
    TF_WRAP(UsdRiRisObject);
        TF_WRAP(UsdRiRisBxdf);

    TF_WRAP(UsdRiRisPattern);
        TF_WRAP(UsdRiRisOslPattern);

    TF_WRAP(UsdRiLightAPI);
    TF_WRAP(UsdRiLightFilterAPI);
    TF_WRAP(UsdRiLightPortalAPI);
    TF_WRAP(UsdRiMaterialAPI);
    TF_WRAP(UsdRiTextureAPI);
    TF_WRAP(UsdRiSplineAPI);

    TF_WRAP(UsdRiRisIntegrator);
    TF_WRAP(UsdRiRslShader);
    TF_WRAP(UsdRiStatementsAPI);

    TF_WRAP(UsdRiTokens);

    TF_WRAP(UsdRiRmanUtilities);

    TF_WRAP(UsdRiPxrAovLight);
    TF_WRAP(UsdRiPxrEnvDayLight);

    TF_WRAP(UsdRiPxrBarnLightFilter);
    TF_WRAP(UsdRiPxrIntMultLightFilter);
    TF_WRAP(UsdRiPxrCookieLightFilter);
    TF_WRAP(UsdRiPxrRampLightFilter);
    TF_WRAP(UsdRiPxrRodLightFilter);
}
