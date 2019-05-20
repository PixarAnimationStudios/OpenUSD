//
// Copyright 2018 Pixar
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
#ifndef WORK_FILTER_H
#define WORK_FILTER_H

/// \file work/filter.h
#include "pxr/pxr.h"
#include "pxr/base/work/threadLimits.h"
#include "pxr/base/work/api.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/reduce.h"

#include <tbb/enumerable_thread_specific.h>
#include <vector>
#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE


///////////////////////////////////////////////////////////////////////////////
///
/// WorkParallelFilterN(size_t n, 
///     Predicate&& predicate,
///     size_t grainsSize)
///
///
/// Runs a filtering operation that in parallel extracts a subset from 
/// a data set of length N based on the predicated value that is 
/// evaluated once for every element of the input
///
/// Predicate must be of the form:
///
///     bool Predicate(size_t index begin, V* value);
///
/// if it evalutes as true then value contains the item from the original list.
///
/// grainSize specifices a minumum amount of work to be done per-thread. There
/// is overhead to launching a thread (or task) and a typical guideline is that
/// you want to have at least 10,000 instructions to count for the overhead of
/// launching a thread.
///
template <typename V, typename Fn>
std::vector<V>
WorkParallelFilterN(size_t N,
                    Fn &&predicate,
                    size_t grainSize)
{
    
    // Use Parallel Reduce to assemble the elements of the vector together
    tbb::enumerable_thread_specific< std::vector<V> > parallelVectors;
    size_t numElements = 0;
    numElements = WorkParallelReduceN(
        numElements /*initial value*/,
        N /*size of data */,        
        [&parallelVectors,
         &predicate](size_t begin, size_t end, size_t numElements)
        {
            V val;
            for(size_t j = begin; j < end; ++j){
                if( std::forward<Fn>(predicate)(j, &val)) {
                    parallelVectors.local().push_back(val);
                    numElements++;
                }
            };
            return numElements;
        },
        [](size_t lhs, size_t rhs) {
           return lhs + rhs;
        },
        grainSize
    );

    // To combine in parallel pre-calculate offsets into the vector
    std::vector<int> starts;
    starts.reserve(parallelVectors.size()+1);
    starts.push_back(0);
    for (const auto& vec : parallelVectors) {
        starts.push_back(starts.back()+vec.size());
    }

    // Accumulate into one vector
    std::vector<V> accumVector;
    accumVector.resize(numElements);
    WorkParallelForN(parallelVectors.size(),
        [&parallelVectors,
         &accumVector,
         &starts](size_t begin, size_t end)
           {
                for(int j = begin; j < end; ++j){
                    const auto& vec = *(parallelVectors.begin()+j);
                    for(unsigned int i =0; i < vec.size(); ++i){
                        accumVector[starts[j] + i] = vec[i];
                    }
                }
            }
        );
    return accumVector;
}

///////////////////////////////////////////////////////////////////////////////
///
/// WorkParallelFilterN(size_t n, 
///     Predicate&& predicate,
///     size_t grainSize)
///
/// Runs a filtering operation that in parallel extracts a subset from 
/// a data set of length N based on the predicated value that is 
/// evaluated once for every element of the input
///
/// Predicate must be of the form:
///
///     bool Predicate(size_t index begin, V& value);
///
/// if it evalutes as true then value contains the item from the original list.
///
template <typename V, typename Fn>
std::vector<V>
WorkParallelFilterN(size_t N,
                    Fn &&predicate)
{
    return WorkParallelFilterN<V>(N, predicate, 1);
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif // WORK_FILTER_H
