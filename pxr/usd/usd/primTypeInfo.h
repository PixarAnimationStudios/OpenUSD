//
// Copyright 2020 Pixar
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
#ifndef PXR_USD_USD_PRIM_TYPE_INFO_H
#define PXR_USD_USD_PRIM_TYPE_INFO_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/primDefinition.h"
#include "pxr/base/tf/token.h"

#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

// Private class that stores the full type information for a prim. These are
// cached, one for each unique, prim type/applied schema list encountered. This 
// provides the PrimData with lazy access to the unique prim definition for this 
// exact prim type in a thread safe way.
//
// Right now this class only represents single concrete schema prim types and is 
// essentially a wrapper around a TfToken. But in an upcoming change, a prim's 
// type will be extended to include the list of API schemas applied to the prim
// (which will affect its prim definition) so this class will provide the 
// necessary full type identity of the prim as well as access to its full prim
// definition.
class Usd_PrimTypeInfo 
{
public:
    // Returns the concrete typed prim type name.
    const TfToken &GetTypeName() const { return _primTypeName; }

    // Returns the prim definition associated with this prim type.
    const UsdPrimDefinition &GetPrimDefinition() const {
        // First check if we've already cached the prim definition pointer; 
        // we can just return it.
        if (const UsdPrimDefinition *primDef = 
                _primDefinition.load(std::memory_order_relaxed)) {
            return *primDef;
        }
        return *_FindOrCreatePrimDefinition();
    }

private:
    // Only the PrimTypeInfoCache can create the PrimTypeInfo prims.
    friend class Usd_PrimTypeInfoCache;

    Usd_PrimTypeInfo() : _primDefinition(nullptr) {};

    Usd_PrimTypeInfo(const TfToken &primTypeName) 
        : _primTypeName(primTypeName), _primDefinition(nullptr) {}

    // Finds the prim definition, creating it if it doesn't already exist. This
    // cache access must be thread safe.
    // Note that right now, all existing prim definitions for single types 
    // will been created when the schema registry is instantiated. When applied
    // API schemas are finally added to this class, this function may have to
    // create new prim definitions when called.
    USD_API
    const UsdPrimDefinition *_FindOrCreatePrimDefinition() const;

    TfToken _primTypeName;

    // Cached pointer to the prim definition.
    mutable std::atomic<const UsdPrimDefinition *> _primDefinition;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_USD_USD_PRIM_TYPE_INFO_H
