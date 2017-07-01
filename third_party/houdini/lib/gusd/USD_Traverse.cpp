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
#include "gusd/USD_Traverse.h"
#include "gusd/UT_Assert.h"

PXR_NAMESPACE_OPEN_SCOPE

bool
GusdUSD_Traverse::FindPrims(const UT_Array<UsdPrim>& roots,
                            const GusdUSD_Utils::PrimTimeMap& timeMap,
                            const UT_Array<GusdPurposeSet> &purposes,
                            UT_Array<UsdPrim>& prims,
                            bool skipRoot,
                            const Opts* opts) const
{
    UT_Array<PrimIndexPair> primIndexPairs;
    if(!FindPrims(roots, timeMap, purposes, primIndexPairs, skipRoot, opts))
        return false;
    prims.setSize(primIndexPairs.size());
    for(exint i = 0; i < prims.size(); ++i)
        prims(i) = primIndexPairs(i).first;
    return true;
}


GusdUSD_TraverseType::GusdUSD_TraverseType(const GusdUSD_Traverse* traversal,
                                           const char* name,
                                           const char* label,
                                           const PRM_Template* templates,
                                           const char* help)
    : _traversal(traversal), _name(name, label),
      _templates(templates), _help(help, 1)
{
    UT_ASSERT(traversal);
    GusdUSD_TraverseTable::GetInstance().Register(this);
}


GusdUSD_TraverseTable&
GusdUSD_TraverseTable::GetInstance()
{
    static GusdUSD_TraverseTable table;
    return table;
}


void
GusdUSD_TraverseTable::Register(const GusdUSD_TraverseType* type)
{
    UT_ASSERT(type);
    const char* name = GusdUTverify_ptr(type->GetName().getToken());
    _map[UT_StringHolder(UT_StringHolder::REFERENCE,name)] = type;
}


const GusdUSD_TraverseType*
GusdUSD_TraverseTable::Find(const char* name) const
{
    auto it = _map.find(UT_StringHolder(UT_StringHolder::REFERENCE,name));
    return it == _map.end() ? NULL : it->second;
}


const GusdUSD_Traverse*
GusdUSD_TraverseTable::FindTraversal(const char* name) const
{
    if(const auto* type = Find(name))
        return &(**type);
    return NULL;
}

PXR_NAMESPACE_CLOSE_SCOPE
