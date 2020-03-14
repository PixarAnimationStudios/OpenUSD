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
#include "pxr/pxr.h"
#include "pxr/usd/usd/primTypeInfo.h"

PXR_NAMESPACE_OPEN_SCOPE

const UsdPrimDefinition * 
Usd_PrimTypeInfo::_FindOrCreatePrimDefinition() const
{
    const UsdPrimDefinition *primDef = nullptr;
    const UsdSchemaRegistry &reg = UsdSchemaRegistry::GetInstance();
    if (_authoredAppliedAPISchemas.empty()) {
        // With no applied schemas we can just get the concrete typed prim 
        // definition from the schema registry. Prim definitions for all 
        // concrete types are created with the schema registry when it is 
        // instantiated so if the type exists, the definition will be there.
        primDef = reg.FindConcretePrimDefinition(_primTypeName);
        if (!primDef) {
            // For invalid types, we use the empty prim definition so we don't
            // have to check again.
            primDef = reg.GetEmptyPrimDefinition();
        }
        // Cache the prim definition pointer. The schema registry created the
        // prim definition and will continue to own it so the pointer value
        // will be constant. Thus, we don't have to check if another thread 
        // cached it first as all threads would store the same pointer.
        _primDefinition.store(primDef, std::memory_order_relaxed);
    } else {
        // If we have applied schemas, then we need ask the schema registry to
        // compose a prim definition for us from the list of types. The schema
        // registry does NOT take ownership of this new prim definition; this 
        // type info will own it instead.
        std::unique_ptr<UsdPrimDefinition> composedPrimDef = 
            reg.BuildComposedPrimDefinition(_primTypeName, 
                                            _authoredAppliedAPISchemas);
        // Try to cache the new prim definition, but if another thread beat us
        // to it, we'll use its definition instead and just let ours get 
        // deleted.
        if (_primDefinition.compare_exchange_strong(
                primDef, composedPrimDef.get(), std::memory_order_acq_rel)) {
            // Since we succeeded, transfer ownership of the new prim definition
            // to this type info.
            _ownedPrimDefinition = std::move(composedPrimDef);
            primDef = _ownedPrimDefinition.get();
        }
    }
    return primDef;
}

PXR_NAMESPACE_CLOSE_SCOPE

