//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/projectionParams.h"
#include <map>

PXR_NAMESPACE_OPEN_SCOPE

namespace HdPrman_ProjectionParams {

    void GetIntegratorParamRole(const TfToken& paramName, TfToken& role)
{
    static std::map<TfToken, TfToken> _integratorParamMap = {
        {TfToken("ri:integrator:PxrUnified:photonVisibilityRodMin"), TfToken("point")},
        {TfToken("ri:integrator:PxrUnified:photonVisibilityRodMax"), TfToken("point")},

        {TfToken("ri:integrator:PxrVCM:photonGuidingBBoxMin"), TfToken("point")},
        {TfToken("ri:integrator:PxrVCM:photonGuidingBBoxMax"), TfToken("point")},

        {TfToken("ri:integrator:PxrVisualizer:wireframeColor"), TfToken("color")}
    };

    const auto it = _integratorParamMap.find(paramName);
    if (it != _integratorParamMap.end())
    {
        role = it->second;
    }
}

void GetProjectionParamRole(TfToken& paramName, TfToken& role)
{
    static std::map<TfToken, TfToken> _ProjectionParamMap = {
        //// PxrPerspective
        {TfToken("ri:projection:PxrPerspective:fovEnd"), TfToken("float")},

        //// PxrCamera
        {TfToken("ri:projection:PxrCamera:fovEnd"), TfToken("float")},
        // Tilt shift
        {TfToken("ri:projection:PxrCamera:tilt"), TfToken("float")},
        {TfToken("ri:projection:PxrCamera:roll"), TfToken("float")},
        {TfToken("ri:projection:PxrCamera:focus1"), TfToken("point")},
        {TfToken("ri:projection:PxrCamera:focus2"), TfToken("point")},
        {TfToken("ri:projection:PxrCamera:focus3"), TfToken("point")},
        {TfToken("ri:projection:PxrCamera:shiftX"), TfToken("float")},
        {TfToken("ri:projection:PxrCamera:shiftY"), TfToken("float")},
        // Lens Distortion
        {TfToken("ri:projection:PxrCamera:radial1"), TfToken("float")},
        {TfToken("ri:projection:PxrCamera:radial2"), TfToken("float")},
        {TfToken("ri:projection:PxrCamera:assymX"), TfToken("float")},
        {TfToken("ri:projection:PxrCamera:assymY"), TfToken("float")},
        {TfToken("ri:projection:PxrCamera:squeeze"), TfToken("float")},
        // Chromatic Aberration
        {TfToken("ri:projection:PxrCamera:axial"), TfToken("color")},
        {TfToken("ri:projection:PxrCamera:transverse"), TfToken("color")},
        // Vignetting
        {TfToken("ri:projection:PxrCamera:natural"), TfToken("float")},
        {TfToken("ri:projection:PxrCamera:optical"), TfToken("float")},
        // Shutter
        {TfToken("ri:projection:PxrCamera:sweep"), TfToken("string")},
        {TfToken("ri:projection:PxrCamera:duration"), TfToken("float")},
        // Advanced
        {TfToken("ri:projection:PxrCamera:detail"), TfToken("float")},
        {TfToken("ri:projection:PxrCamera:enhance"), TfToken("vector")},
        {TfToken("ri:projection:PxrCamera:matte"), TfToken("string")},

        //// PxrCylinderCamera
        {TfToken("ri:projection:PxrCylinderCamera:hsweep"), TfToken("float")},
        {TfToken("ri:projection:PxrCylinderCamera:vsweep"), TfToken("float")},

        //// PxrSphereCamera
        {TfToken("ri:projection:PxrSphereCamera:hsweep"), TfToken("float")},
        {TfToken("ri:projection:PxrSphereCamera:vsweep"), TfToken("float")},

        //// OmnidirectionalStereo
        {TfToken("ri:projection:OmnidirectionalStereo:interpupilaryDistance"), TfToken("float")}
    };

    const auto it = _ProjectionParamMap.find(paramName);
    if (it != _ProjectionParamMap.end())
    {
        role = it->second;
    }
}

void GetFilterParamRole(TfToken& paramName, TfToken& role)
{
    // color params won't work unless set in the param list as color
    // rather than float3
    static const std::map<TfToken, TfToken> _filterParamMap = {
        {TfToken("PxrBackgroundSampleFilter:backgroundColor"), TfToken("color")},
        {TfToken("PxrGradeSampleFilter:blackPoint"), TfToken("color")},
        {TfToken("PxrGradeSampleFilter:whitePoint"), TfToken("color")},
        {TfToken("PxrGradeSampleFilter:lift"), TfToken("color")},
        {TfToken("PxrGradeSampleFilter:gain"), TfToken("color")},
        {TfToken("PxrGradeSampleFilter:multiply"), TfToken("color")},
        {TfToken("PxrGradeSampleFilter:gamma"), TfToken("color")},
        {TfToken("PxrGradeSampleFilter:offset"), TfToken("color")},
        {TfToken("PxrGradeSampleFilter:mask"), TfToken("color")},
        {TfToken("PxrBackgroundDisplayFilter:backgroundColor"), TfToken("color")},
        {TfToken("PxrImagePlaneFilter:colorGain"), TfToken("color")},
        {TfToken("PxrImagePlaneFilter:colorOffset"), TfToken("color")},
        {TfToken("PxrWhitePointSampleFilter:manualWhitePoint"), TfToken("color")},
        {TfToken("PxrGradeDisplayFilter:blackPoint"), TfToken("color")},
        {TfToken("PxrGradeDisplayFilter:whitePoint"), TfToken("color")},
        {TfToken("PxrGradeDisplayFilter:lift"), TfToken("color")},
        {TfToken("PxrGradeDisplayFilter:gain"), TfToken("color")},
        {TfToken("PxrGradeDisplayFilter:multiply"), TfToken("color")},
        {TfToken("PxrGradeDisplayFilter:gamma"), TfToken("color")},
        {TfToken("PxrGradeDisplayFilter:offset"), TfToken("color")},
        {TfToken("PxrGradeDisplayFilter:mask"), TfToken("color")},
        {TfToken("PxrImageDisplayFilter:colorGain"), TfToken("color")},
        {TfToken("PxrImageDisplayFilter:colorOffset"), TfToken("color")},
        {TfToken("PxrWhitePointDisplayFilter:manualWhitePoint"), TfToken("color")}
    };

    const auto it = _filterParamMap.find(paramName);
    if (it != _filterParamMap.end())
    {
        role = it->second;
    }
}

} // namespace HdPrman_ProjectionParams

PXR_NAMESPACE_CLOSE_SCOPE
