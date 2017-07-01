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
#ifndef __GUSD_PURPOSE_H__
#define __GUSD_PURPOSE_H__

#include <UT/UT_StringArray.h>

#include <pxr/pxr.h>
#include "pxr/usd/usdGeom/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

enum GusdPurposeSet {
    GUSD_PURPOSE_NONE =    0x00,
    GUSD_PURPOSE_DEFAULT = 0x01,
    GUSD_PURPOSE_PROXY =   0x02,
    GUSD_PURPOSE_RENDER =  0x04,
    GUSD_PURPOSE_GUIDE =   0x08,
};

inline bool 
GusdPurposeInSet( const TfToken &name, GusdPurposeSet set ) 
{
    if(name == UsdGeomTokens->default_)
        return set&GUSD_PURPOSE_DEFAULT;
    else if(name == UsdGeomTokens->proxy)
        return set&GUSD_PURPOSE_PROXY;
    else if(name == UsdGeomTokens->render)
        return set&GUSD_PURPOSE_RENDER;
    else if(name == UsdGeomTokens->guide)
        return set&GUSD_PURPOSE_GUIDE;
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __GUSD_PURPOSE_H__
