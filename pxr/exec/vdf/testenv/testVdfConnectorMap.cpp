//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/vdf/connectorMap.h"

#include <iostream>
#include <utility>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
struct TestConnector
{
};
}

using MapType = VdfConnectorMap<TestConnector>;
using ResultType = std::vector<std::pair<TfToken, TestConnector*>>;

// Tests that the result of Find is as expected.
//
bool
_TestFind(const MapType &map,
          const TfToken &key,
          TestConnector *expected)
{
    TestConnector *result = map.find(key)->second;

    if (result != expected) {
        std::cerr << "Found the wrong value.  Expected "
                  << expected << ", got " << result
                  << std::endl;
        return false;
    }

    return true;
}

// Tests that a map contains the expected elements.  Tests iteration over
// mask maps.
//
bool
_TestMapContents(const MapType &map,
                 const ResultType &expected)
{
    if (map.size() != expected.size()) {
        std::cerr << "Map contains the wrong number of elements.  Expected "
                  << expected.size() << ", got " << map.size()
                  << std::endl;
        return false;
    }

    // Test iteration over the map.
    int index = 0;
    for (const auto &[resultKey, result] : map) {

        if (resultKey != expected[index].first) {
            std::cerr << "Got wrong key for result element " << index
                      << ".  Expected " << expected[index].first
                      << ", got " << resultKey << std::endl;
            return false;
        }

        if (result != expected[index].second) {
            std::cerr << "Got wrong value for result element " << index
                      << ".  Expected " << expected[index].second
                      << ", got " << result << std::endl;
            return false;
        }

        ++index;
    }

    return true;
}

int
main(int argc, char **argv)
{
    MapType map;
    MapType emptyMap;

    int numErrors = 0;

    if (map != emptyMap) {
        std::cerr << "Maps should have been equal."
                  << std::endl;
        numErrors++;
    }

    // Create some entry keys to use as values.
    TfToken a("a");
    TfToken b("b");

    // VdfConnectorMap owns the TestConnector objects and returns
    // pointers to them.
    TestConnector * aptr = map.try_emplace(a).first->second;
    TestConnector * bptr = map.try_emplace(b).first->second;

    if (map == emptyMap) {
        std::cerr << "Maps should have been unequal."
                  << std::endl;
        numErrors++;
    }

    // Test the contents of the map.
    {
        ResultType result;
        result.push_back( std::make_pair(a, aptr) );
        result.push_back( std::make_pair(b, bptr) );

        if ( !_TestMapContents(map, result) ) {
            ++numErrors;
        }
    }

    // Find individual entries in the map.
    if (!_TestFind(map, a, aptr)) {
        ++numErrors;
    }

    if (!_TestFind(map, b, bptr)) {
        ++numErrors;
    }

    // Swap maps.
    map.swap(emptyMap);
    if (map.size() != 0) {
        std::cerr << "Wrong size for map.  Expected 0, got "
                  << map.size() << std::endl;
        numErrors++;
    }

    // Swap back.
    using std::swap;
    swap(map, emptyMap);
    if (map.size() != 2) {
        std::cerr << "Wrong size for map.  Expected 2, got "
                  << map.size() << std::endl;
        numErrors++;
    }
    if (!emptyMap.empty()) {
        std::cerr << "Empty map is not empty.  Contains "
                  << emptyMap.size() << " elements." << std::endl;
        numErrors++;
    }

    // Clear the map.
    map.clear();
    if (map.size() != 0) {
        std::cerr << "Wrong size for map.  Expected 0, got "
                  << map.size() << std::endl;
        numErrors++;
    }

    return numErrors;
}
