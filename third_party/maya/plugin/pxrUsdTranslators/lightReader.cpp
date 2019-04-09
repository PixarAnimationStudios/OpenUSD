//
// Copyright 2019 Pixar
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
#include "pxr/usd/usdLux/diskLight.h"
#include "pxr/usd/usdLux/distantLight.h"
#include "pxr/usd/usdLux/domeLight.h"
#include "pxr/usd/usdLux/geometryLight.h"
#include "pxr/usd/usdLux/rectLight.h"
#include "pxr/usd/usdLux/sphereLight.h"

#include "pxr/usd/usdRi/pxrAovLight.h"
#include "pxr/usd/usdRi/pxrEnvDayLight.h"

#include "usdMaya/translatorRfMLight.h"
#include "usdMaya/primReaderRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE

PXRUSDMAYA_DEFINE_READER(UsdLuxDiskLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Read(args, context);
}

PXRUSDMAYA_DEFINE_READER(UsdLuxDistantLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Read(args, context);
}

PXRUSDMAYA_DEFINE_READER(UsdLuxDomeLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Read(args, context);
}

PXRUSDMAYA_DEFINE_READER(UsdLuxGeometryLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Read(args, context);
}

PXRUSDMAYA_DEFINE_READER(UsdLuxRectLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Read(args, context);
}

PXRUSDMAYA_DEFINE_READER(UsdLuxSphereLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Read(args, context);
}

PXRUSDMAYA_DEFINE_READER(UsdRiPxrAovLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Read(args, context);
}

PXRUSDMAYA_DEFINE_READER(UsdRiPxrEnvDayLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Read(args, context);
}

PXR_NAMESPACE_CLOSE_SCOPE
