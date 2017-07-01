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
#include "gusd/USD_ThreadedTraverse.h"

#include "gusd/UT_Usd.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace GusdUSD_ThreadedTraverse {


TaskData::~TaskData()
{
    for(auto it = threadData.begin(); it != threadData.end(); ++it) {
        if(auto* tdata = it.get())
            delete tdata;
    }
}


bool
TaskData::GatherPrimsFromThreads(
    UT_Array<GusdUSD_Traverse::PrimIndexPair>& prims)
{
    prims.clear();
        
    /* Compute the full prim count & pre-allocate space.*/
    exint nPrims = 0;
    for(auto it = threadData.begin(); it != threadData.end(); ++it) {
        if(const auto* tdata = it.get())
            nPrims += tdata->prims.size();
    }
    prims.setCapacity(nPrims);

    /* Concat the per-thread arrays.*/
    for(auto it = threadData.begin(); it != threadData.end(); ++it) {
        if(const auto* tdata = it.get())
            prims.concat(tdata->prims);
    }

    /* The ordering of prims coming directly from different threads
       will be non-deterministic. Sort them to make our results deterministic.*/
    UTparallelStableSort<
        UT_Array<GusdUSD_Traverse::PrimIndexPair>::iterator>(
        prims.begin(), prims.end(),
        [](const GusdUSD_Traverse::PrimIndexPair& lhs,
           const GusdUSD_Traverse::PrimIndexPair& rhs)
        {
            return lhs.second < rhs.second ||
                   (lhs.second == rhs.second &&
                    lhs.first.GetPath() < rhs.first.GetPath());
        });
    return !UTgetInterrupt()->opInterrupt();
}


bool
TaskData::GatherPrimsFromThreads(UT_Array<UsdPrim>& prims)
{
    prims.clear();
        
    /* Compute the full prim count */
    exint nPrims = 0;
    for(auto it = threadData.begin(); it != threadData.end(); ++it) {
        if(const auto* tdata = it.get())
            nPrims += tdata->prims.size();
    }
        
    /* Pre-allocate all the space we need */
    prims.setCapacity(nPrims);

    /* Concat the per-thread arrays.*/
    for(auto it = threadData.begin(); it != threadData.end(); ++it) {
        if(const auto* tdata = it.get()) {
            /* Entries are stored in threads as (prim,index) pairs,
               so need to pull the prims separately.*/
            exint start = prims.size();
            exint count = tdata->prims.size();
            prims.setSize(start + count);
            for(exint i = 0; i < count; ++i)
                prims(start + i) = tdata->prims(i).first;
        }
    }

    /* The ordering of prims coming directly from different threads
       will be non-deterministic. Sort them to make our results deterministic.*/
    return GusdUSD_Utils::SortPrims(prims);
}


} /*namespace GusdUSD_ThreadedTraverse*/

PXR_NAMESPACE_CLOSE_SCOPE

