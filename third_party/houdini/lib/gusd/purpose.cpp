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
#include "gusd/purpose.h"

#include "pxr/usd/usdGeom/imageable.h"

#include <UT/UT_StringMMPattern.h>


PXR_NAMESPACE_OPEN_SCOPE


GusdPurposeSet
GusdPurposeSetFromArray(const UT_StringArray& purposes)
{
    int p = GUSD_PURPOSE_NONE;
    for(const auto& s : purposes)
        p |= GusdPurposeSetFromName(s);
    return GusdPurposeSet(p);
}


GusdPurposeSet
GusdPurposeSetFromArray(const TfTokenVector& purposes)
{
    int p = GUSD_PURPOSE_NONE;
    for(const auto& t : purposes)
        p |= GusdPurposeSetFromName(t);
    return GusdPurposeSet(p);
}


TfTokenVector
GusdPurposeSetToTokens(GusdPurposeSet purposes)
{
    TfTokenVector tokens;
    if(purposes&GUSD_PURPOSE_DEFAULT)
        tokens.push_back(UsdGeomTokens->default_);
    if(purposes&GUSD_PURPOSE_PROXY)
        tokens.push_back(UsdGeomTokens->proxy);
    if(purposes&GUSD_PURPOSE_RENDER)
        tokens.push_back(UsdGeomTokens->render);
    if(purposes&GUSD_PURPOSE_GUIDE)
        tokens.push_back(UsdGeomTokens->guide);
    return tokens;
}


UT_StringArray
GusdPurposeSetToStrings(GusdPurposeSet purposes)
{
    UT_StringArray names;
    if(purposes&GUSD_PURPOSE_DEFAULT)
        names.append(UTmakeUnsafeRef(UsdGeomTokens->default_.GetString()));
    if(purposes&GUSD_PURPOSE_PROXY)
        names.append(UTmakeUnsafeRef(UsdGeomTokens->proxy.GetString()));
    if(purposes&GUSD_PURPOSE_RENDER)
        names.append(UTmakeUnsafeRef(UsdGeomTokens->render.GetString()));
    if(purposes&GUSD_PURPOSE_GUIDE)
        names.append(UTmakeUnsafeRef(UsdGeomTokens->guide.GetString()));
    return names;
}


GusdPurposeSet
GusdPurposeSetFromMask(const char* mask)
{
    int p = GUSD_PURPOSE_NONE;

    if(UTisstring(mask)) {
        UT_StringMMPattern pat;
        pat.compile(mask);
        
        for(const auto& t : UsdGeomImageable::GetOrderedPurposeTokens()) {
            if(UT_String(t.GetText()).multiMatch(pat)) {
                p |= GusdPurposeSetFromName(t);
            }
        }
    }
    return GusdPurposeSet(p);
}


PXR_NAMESPACE_CLOSE_SCOPE
