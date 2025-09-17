//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/span.h"
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

static bool TestPointerIterators() {
    // Ensure that TF_FOR_ALL works with iterators that are raw pointers.

    static const int data[] = { 3, 2, 1 };
    TfSpan<const int> span(data, TfArraySize(data));

    int sum = 0;
    int count = 0;
    TF_FOR_ALL(it, span) {
        sum += *it;
        ++count;
    }
    TF_AXIOM(sum == 6);
    TF_AXIOM(count == 3);

    TfSpan<std::string> empty(nullptr, nullptr);

    sum = 0;
    count = 0;
    TF_FOR_ALL(it, empty) {
        sum += it->size();
        ++count;
    }
    TF_AXIOM(sum == 0);
    TF_AXIOM(count == 0);

    return true;
}

static bool
Test_TfIterator()
{
    return TestNonConst() && TestConst() && TestRefsAndTempsForAll()
        && TestPointerIterators();
}

TF_ADD_REGTEST(TfIterator);




