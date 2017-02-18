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
#ifndef USDIMAGING_VERSION_H
#define USDIMAGING_VERSION_H

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE



// Version 3 -- add support for nested instancers in InsertInstancer.
// Version 4 -- Populate returns SdfPath, HdxSelectionInstanceMap.
// Version 5 -- GetPathForInstanceIndex returns absoluteInstanceIndex.
// Version 6 -- PrimAdater::GetDependPaths.
// Version 7 -- GetPathForInstanceIndex returns instanceContext.
// Version 8 -- GetPathForInstanceIndex returns instanceContext (as 
//              SdfPathVector) and rprimPath  separately.
// Version 9 -- Rework UsdImagingEngineGL::RenderParams API to conform to
//              updated purpose tokens and make proxy imaging optional.

#define USD_IMAGING_API_VERSION 9


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDIMAGING_VERSION_H
