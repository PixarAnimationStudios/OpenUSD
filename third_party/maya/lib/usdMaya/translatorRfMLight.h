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
#ifndef PXRUSDMAYA_TRANSLATOR_RFM_LIGHT_H
#define PXRUSDMAYA_TRANSLATOR_RFM_LIGHT_H

/// \file usdMaya/translatorRfMLight.h

#include "pxr/pxr.h"

#include "usdMaya/api.h"
#include "usdMaya/primReaderArgs.h"
#include "usdMaya/primReaderContext.h"
#include "usdMaya/primWriterArgs.h"
#include "usdMaya/primWriterContext.h"


PXR_NAMESPACE_OPEN_SCOPE


struct UsdMayaTranslatorRfMLight
{
    /// Exports a UsdLux schema prim when provided args and a context that
    /// identify a RenderMan for Maya light.
    ///
    /// Returns true if this succeeds in creating a UsdLux schema prim.
    PXRUSDMAYA_API
    static bool Write(
            const UsdMayaPrimWriterArgs& args,
            UsdMayaPrimWriterContext* context);

    /// Imports a UsdLux schema prim as a RenderMan for Maya light.
    ///
    /// Returns true if this succeeds in creating a RenderMan for Maya light.
    PXRUSDMAYA_API
    static bool Read(
            const UsdMayaPrimReaderArgs& args,
            UsdMayaPrimReaderContext* context);
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
