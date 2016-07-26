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
#ifndef SDF_SITE_UTILS_H
#define SDF_SITE_UTILS_H

#include "pxr/usd/sdf/site.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/propertySpec.h"
#include "pxr/usd/sdf/spec.h"

/// Convenience API for working with SdfSite. These functions simply forward
/// to the indicated functions on SdfLayer.

inline
SdfSpecHandle
SdfGetObjectAtPath(const SdfSite& site)
{
    return site.layer->GetObjectAtPath(site.path);
}

inline
SdfPrimSpecHandle
SdfGetPrimAtPath(const SdfSite& site)
{
    return site.layer->GetPrimAtPath(site.path);
}

inline
SdfPropertySpecHandle
SdfGetPropertyAtPath(const SdfSite& site)
{
    return site.layer->GetPropertyAtPath(site.path);
}

inline 
bool
SdfHasField(const SdfSite& site, const TfToken& field)
{
    return site.layer->HasField(site.path, field);
}

template <class T>
inline bool 
SdfHasField(const SdfSite& site, const TfToken& field, T* value)
{
    return site.layer->HasField(site.path, field, value);
}

inline
const VtValue
SdfGetField(const SdfSite& site, const TfToken& field)
{
    return site.layer->GetField(site.path, field);
}

template <class T>
inline 
T
SdfGetFieldAs(const SdfSite& site, const TfToken& field, 
              const T& defaultValue = T())
{
    return site.layer->GetFieldAs<T>(site.path, field, defaultValue);
}

#endif // SDF_SITE_UTILS_H
