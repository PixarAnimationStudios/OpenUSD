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
#include "pxr/base/tf/iterator.h"
#include <vector>
#include <map>

using namespace std;
PXR_NAMESPACE_USING_DIRECTIVE

#define TESTBODY() \
    vector<int> origVec, copyVec; \
\
    origVec.push_back(0); \
    origVec.push_back(-5); \
    origVec.push_back(5); \
\
    TfIterator<CONST vector<int> > vecIter; \
    for(vecIter = origVec; vecIter; ++ vecIter) { \
        copyVec.push_back(*vecIter); \
    } \
    TF_AXIOM(!vecIter); \
\
    TF_AXIOM(origVec == copyVec); \
\
    map<string, char> origMap, copyMap; \
    origMap["a"] = 'a'; \
    origMap["b"] = 'b'; \
    origMap["c"] = 'c'; \
\
    TfIterator<CONST map<string, char> > \
        mapIter(origMap.begin(), origMap.end()), \
        mapIterCopy = mapIter; \
    TF_AXIOM(mapIterCopy == mapIter); \
    for(; mapIter; mapIter ++) { \
        copyMap[mapIter->first] = mapIter->second; \
    } \
\
    TfIterator<CONST map<string, char> > \
        mapEnd(origMap.end(), origMap.end()); \
    TF_AXIOM(mapIter == mapEnd); \
    TF_AXIOM(mapIter != mapIterCopy); \
\
    TF_AXIOM(origMap == copyMap); \
\
    copyMap.clear(); \
    for(; mapIterCopy; mapIterCopy = mapIterCopy.GetNext()) { \
        map<string, char>::ITERTYPE stlIter = mapIterCopy; \
        copyMap[stlIter->first] = stlIter->second; \
    } \
    TF_AXIOM(!mapIter); \
\
    TF_AXIOM(origMap == copyMap);\

#define CONST
#define ITERTYPE iterator
static bool
TestNonConst()
{
    TESTBODY();
    return true;
}
#undef CONST
#undef ITERTYPE

#define CONST const
#define ITERTYPE const_iterator
static bool
TestConst()
{
    TESTBODY();
    return true;
}
#undef CONST
#undef ITERTYPE



// static vector<int> GetTemporary() {
//     vector<int> ret;
//     ret.push_back(1);
//     ret.push_back(2);
//     ret.push_back(3);
//     return ret;
// }

// static const vector<int> GetConstTemporary() {
//     vector<int> ret;
//     ret.push_back(1);
//     ret.push_back(2);
//     ret.push_back(3);
//     return ret;
// }

static vector<int> const &GetConstRef() {
    static const vector<int> _data = { 3, 2, 1 };
    return _data;
}

static vector<int> &GetNonConstRef() {
    static vector<int> _data = { 3, 2, 1 };
    return _data;
}


static bool TestRefsAndTempsForAll() {

    // Temporaries cannot be TF_FOR_ALL iterated over unless they're special
    // "iterate over-a-copy" objects.

    int count;
    
//     count = 1;
//     TF_FOR_ALL(i, GetTemporary())
//         TF_AXIOM(*i == count++);

//     count = 1;
//     TF_FOR_ALL(i, GetConstTemporary())
//         TF_AXIOM(*i == count++);

    count = 3;
    TF_FOR_ALL(i, GetConstRef())
        TF_AXIOM(*i == count--);

    count = 3;
    TF_FOR_ALL(i, GetNonConstRef())
        TF_AXIOM(*i == count--);

    count = 1;
    TF_REVERSE_FOR_ALL(i, GetConstRef())
        TF_AXIOM(*i == count++);

    count = 1;
    TF_REVERSE_FOR_ALL(i, GetNonConstRef())
        TF_AXIOM(*i == count++);

    return true;
}    

static bool
Test_TfIterator()
{
    return TestNonConst() && TestConst() && TestRefsAndTempsForAll();
}

TF_ADD_REGTEST(TfIterator);




