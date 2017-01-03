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
#include "pxr/base/tf/denseHashMap.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/stringUtils.h"

#include <cstdio>

// Test equality operator.
struct TestTf_DenseHashMapModuloEqual
{
    TestTf_DenseHashMapModuloEqual(size_t mod = 1)
    :   _mod(mod) {}

    bool operator()(const size_t &x, const size_t &y) const {
        return x%_mod == y%_mod;
    }

    size_t _mod;
};

static void Run()
{
    typedef TfDenseHashMap<size_t, std::string, TfHash> _Map;
    _Map _map;

    // Make sure size expectations are ok.
    TF_AXIOM(sizeof(_Map) == 4 * sizeof(void *));

    // Insert a bunch of numbers in order.
    printf("inserting numbers to 10000\n");
    for(size_t i = 1; i <= 10000; ++i) {
        _map[i] = TfStringify(i);
        TF_AXIOM(_map.size() == i);
        TF_AXIOM(_map.find(i) != _map.end());
        TF_AXIOM(_map.count(i) == 1);
    }

    TF_AXIOM(!_map.empty());
    TF_AXIOM(_map.size() == 10000);

    printf("Exercise assignment operator and swap.\n");
    _Map newMap;

    newMap = _map;
    TF_AXIOM(newMap.size() == _map.size());
    newMap[9999999] = "full";
    TF_AXIOM(newMap.size() == _map.size()+1);

    newMap.swap(_map);
    TF_AXIOM(newMap.size()+1 == _map.size());
    newMap.swap(_map);
    TF_AXIOM(newMap.size() == _map.size()+1);


    printf("checking containment\n");
    for(size_t i = 1; i <= 10000; ++i)
        TF_AXIOM(_map.count(i) == 1);

    printf("checking correct mapping\n");
    for(size_t i = 1; i <= 10000; ++i)
        TF_AXIOM(_map[i] == TfStringify(i));

    // Remove some stuff.
    printf("erasing 8000 elements\n");
    for(size_t i = 1000; i < 9000; ++i)
        TF_AXIOM(_map.erase(i) == 1);

    // Attempt to remove some stuff again.
    printf("erasing 8000 elements\n");
    for(size_t i = 1000; i < 9000; ++i)
        TF_AXIOM(_map.erase(i) == 0);

    TF_AXIOM(!_map.empty());
    TF_AXIOM(_map.size() == 2000);

    printf("checking containment\n");
    for(size_t i = 1; i <= 10000; ++i) {
        if (i < 1000 || i >= 9000)
            TF_AXIOM(_map.count(i) == 1);
        else
            TF_AXIOM(_map.count(i) == 0);
    }

    printf("testing shrink to fit\n");
    _map.shrink_to_fit();

    TF_AXIOM(!_map.empty());
    TF_AXIOM(_map.size() == 2000);

    printf("checking containment\n");
    for(size_t i = 1; i <= 10000; ++i) {
        if (i < 1000 || i >= 9000)
            TF_AXIOM(_map.count(i) == 1);
        else
            TF_AXIOM(_map.count(i) == 0);
    }

    // Put it back.
    printf("reinserting 8000 elements\n");
    for(size_t i = 1000; i < 9000; ++i)
        _map[i] = TfStringify(i);

    TF_AXIOM(!_map.empty());
    TF_AXIOM(_map.size() == 10000);

    printf("checking containment\n");
    for(size_t i = 1; i <= 10000; ++i)
        TF_AXIOM(_map.count(i) == 1);

    printf("checking correct mapping\n");
    for(size_t i = 1; i <= 10000; ++i)
        TF_AXIOM(_map[i] == TfStringify(i));

    // Remove some stuff.
    printf("erasing 8000 elements\n");
    for(size_t i = 1000; i < 9000; ++i)
        _map.erase(i);

    TF_AXIOM(!_map.empty());
    TF_AXIOM(_map.size() == 2000);

    printf("checking containment\n");
    for(size_t i = 1; i <= 10000; ++i) {
        TF_AXIOM(_map.count(i) == (i < 1000 || i >= 9000));
    }

    // iterate
    printf("iterating\n");
    size_t count = 0;
    for(_Map::iterator i = _map.begin(); i != _map.end(); ++i, ++count) {
        TF_AXIOM(TfStringify(i->first) == i->second);
        TF_AXIOM(i->first < 1000 || i->first >= 9000);
    }
    TF_AXIOM(count == 2000);

    // iterate
    printf("const iterating\n");
    count = 0;
    for(_Map::const_iterator i = _map.begin(); i != _map.end(); ++i, ++count) {
        TF_AXIOM(TfStringify(i->first) == i->second);
        TF_AXIOM(i->first < 1000 || i->first >= 9000);
    }
    TF_AXIOM(count == 2000);
            
    printf("remove all but the first two elements using erase(range)...\n");
    _Map::iterator i0 = _map.begin();
    size_t keys[2];
    keys[0] = i0++->first;
    keys[1] = i0++->first;
    _map.erase(i0, _map.end());
    TF_AXIOM(_map.size() == 2);
    i0 = _map.begin();
    TF_AXIOM(i0++->first == keys[0]);
    TF_AXIOM(i0++->first == keys[1]);

    printf("inserting using insert(range)\n");
    std::vector< std::pair<int, std::string> > morekeys;
    for(size_t i=100; i<200; i++)
        morekeys.push_back(std::make_pair(i, "hello"));
    _map.insert(morekeys.begin(), morekeys.end());
    TF_AXIOM(_map.size() == 102);
    for(size_t i=100; i<200; i++)
        TF_AXIOM(_map[i] == "hello");


    // copying and comparing maps.
    printf("copying and comparing...\n");
    _Map other(_map);
    TF_AXIOM(other.size() == _map.size());
    TF_AXIOM(other == _map);
    other[4711] = "different_now";
    TF_AXIOM(other.size() != _map.size());
    TF_AXIOM(other != _map);


    // clear it.
    printf("clearing\n");
    _map.clear();
    TF_AXIOM(_map.empty());
    TF_AXIOM(_map.size() == 0);

    printf("shrinking\n");
    _map.shrink_to_fit();
    TF_AXIOM(_map.empty());
    TF_AXIOM(_map.size() == 0);





    printf("\nTesting TfDenseHashMap using an EqualKey.\n");

    typedef
        TfDenseHashMap<
            size_t, std::string, TfHash, TestTf_DenseHashMapModuloEqual, 128>
        _Map2;

    _Map2 _map2(TfHash(), TestTf_DenseHashMapModuloEqual(2));

    // Make sure size expectations are ok.
    TF_AXIOM(sizeof(_Map2) > 4 * sizeof(void *));

    // Insert a bunch of numbers in order.
    printf("inserting numbers to 10000\n");
    for(size_t i = 1; i <= 10000; ++i) {
        _map2[i] = TfStringify(i);
    }

    printf("expecting only two elements\n");
    TF_AXIOM(!_map2.empty());
    TF_AXIOM(_map2.size() == 2);
}

static bool
Test_TfDenseHashMap()
{
    Run();
    return true;
}

TF_ADD_REGTEST(TfDenseHashMap);

