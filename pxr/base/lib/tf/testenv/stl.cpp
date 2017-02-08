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

#include "pxr/pxr.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/hash.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/stopwatch.h"

#include "pxr/base/tf/hashmap.h"

using std::map;
using std::pair;
using std::string;
using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

static void testSetDifferences()
{
    int a1[] = {1, 3, 3, 1};
    int a2[] = {2, 3, 2};

    int e1[] = {1, 3, 1};
    int e2[] = {1};
    
    vector<int> v1(a1, a1 + (sizeof(a1)/sizeof(a1[0])));
    vector<int> v2(a2, a2 + (sizeof(a2)/sizeof(a2[0])));

    vector<int> expected1(e1, e1 + (sizeof(e1)/sizeof(e1[0])));
    vector<int> expected2(e2, e2 + (sizeof(e2)/sizeof(e2[0])));

    
    vector<int> result1 =
        TfOrderedSetDifferenceToContainer<vector<int> >(v1.begin(), v1.end(),
                                                        v2.begin(), v2.end());
    TF_AXIOM(result1 == expected1);

    vector<int> result2 =
        TfOrderedUniquingSetDifferenceToContainer<vector<int> >
        (v1.begin(), v1.end(), v2.begin(), v2.end());
    TF_AXIOM(result2 == expected2);
}


static bool
Test_TfStl()
{
    testSetDifferences();
    
    TfHashMap<string, int, TfHash> hm;
    map<string, int> m;

    const TfHashMap<string, int, TfHash>& chm = hm;
    const map<string, int>& cm = m;

    int hvalue = 0, mvalue = 0;
    string key("key");
    string badKey("blah");
    
    hm[key] = 1;
    m[key] = 1;

    TF_AXIOM(TfMapLookup(hm, key, &hvalue) && hvalue == 1);
    TF_AXIOM(TfMapLookup(m, key, &mvalue) && mvalue == 1);
    TF_AXIOM(TfMapLookupPtr(m, key) == &m[key]);
    TF_AXIOM(TfMapLookupPtr(hm, key) == &hm[key]);

    TF_AXIOM(TfMapLookupPtr(cm, key) == &m[key]);
    TF_AXIOM(TfMapLookupPtr(chm, key) == &hm[key]);


    TF_AXIOM(!TfMapLookup(m, badKey, &mvalue));
    TF_AXIOM(!TfMapLookup(hm, badKey, &hvalue));
    TF_AXIOM(!TfMapLookupPtr(m, badKey));
    TF_AXIOM(!TfMapLookupPtr(hm, badKey));
    TF_AXIOM(!TfMapLookupPtr(cm, badKey));
    TF_AXIOM(!TfMapLookupPtr(chm, badKey));

    TF_AXIOM(TfOrderedPair(1, 2) == TfOrderedPair(2, 1));
    TF_AXIOM((TfOrderedPair(2, 1) == pair<int,int>(1,2)));

    return true;
}

TF_ADD_REGTEST(TfStl);
