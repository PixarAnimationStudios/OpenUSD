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

#include "pxr/pxr.h"
#include "pxr/base/work/sort.h"

#include "pxr/base/work/arenaDispatcher.h"
#include "pxr/base/work/threadLimits.h"

#include "pxr/base/tf/stopwatch.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/arch/fileSystem.h"

#include <functional>

#include <cstdio>
#include <iostream>

using namespace std::placeholders;

PXR_NAMESPACE_USING_DIRECTIVE

static void
_PopulateVector(size_t arraySize, std::vector<int>* v)
{
    std::srand(std::time(NULL));
    v->clear();
    v->reserve(arraySize);
    for (size_t i = 0; i < arraySize; ++i) {
        v->push_back(std::rand());
    }
}

// Returns the number of seconds it took to complete this operation.
double
_DoTBBTest(bool verify, const size_t arraySize, const size_t numIterations)
{
    std::vector<int> v;
    _PopulateVector(arraySize, &v);
    std::vector<int> save = v;

    TfStopwatch sw;
    sw.Start();
    std::vector<int> filterv;
    for (size_t i = 0; i < numIterations; i++) {
        v = save;
         WorkParallelSort(&v);
    }

    if (verify) {
        TF_AXIOM(numIterations == 1);
        for(unsigned int i = 1; i < v.size(); ++i){
            TF_AXIOM(v[i-1] <= v[i]);
        }
    }
    sw.Stop();
    return sw.GetSeconds();
}


int
main(int argc, char **argv)
{
    const bool perfMode = ((argc > 1) && !strcmp(argv[1], "--perf")); 
    const size_t arraySize = 1000000;
    const size_t numIterations = perfMode ? 1000 : 1;

    WorkSetMaximumConcurrencyLimit();

    std::cout << "Initialized with " << 
        WorkGetPhysicalConcurrencyLimit() << " cores..." << std::endl;

    double tbbSeconds = _DoTBBTest(!perfMode, arraySize, numIterations);

    std::cout << "TBB parallel_sort.h took: " << tbbSeconds << " seconds" 
        << std::endl;

    if (perfMode) {

        // XXX:perfgen only accepts metric names ending in _time.  See bug 97317
        FILE *outputFile = ArchOpenFile("perfstats.raw", "w");

        fprintf(outputFile,
        "{'profile':'TBB Sort_time','metric':'time','value':%f,'samples':1}\n",
            tbbSeconds);        
        fclose(outputFile);

    }

    return 0;
}
