//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/vdf/linearMap.h"

#include "pxr/base/tf/iterator.h"

#include <iostream>
#include <utility>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

typedef std::vector< std::pair<int, int> > ResultType;

typedef VdfLinearMap<int, int> MapType;


// Tests that the result of Find is as expected.
//
bool
_TestFind(const MapType &map,
          int key,
          int expected)
{
    int result = map.find(key)->second;

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
    TF_FOR_ALL(i, map) {
        int resultKey = i->first;
        int result = i->second;

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

    map.insert( std::make_pair(1, 10) );
    map.insert( std::make_pair(2, 20) );

    if (map == emptyMap) {
        std::cerr << "Maps should have been unequal."
                  << std::endl;
        numErrors++;
    }

    // Test the contents of the map.
    {
        ResultType result;
        result.push_back( std::make_pair(1, 10) );
        result.push_back( std::make_pair(2, 20) );

        if ( !_TestMapContents(map, result) ) {
            ++numErrors;
        }
    }

    // Find individual entries in the map.
    if (!_TestFind(map, 1, 10)) {
        ++numErrors;
    }

    if (!_TestFind(map, 2, 20)) {
        ++numErrors;
    }

    // Test count().
    if (map.count(1) != 1) {
        std::cerr << "Wrong result from count()"
                  << std::endl;
        numErrors++;
    }

    // Swap maps.
    map.swap(emptyMap);
    if (map.size() != 0) {
        std::cerr << "Wrong size for map.  Expected 0, got "
                  << map.size() << std::endl;
        numErrors++;
    }

    // Swap back.
    map.swap(emptyMap);

    // Clear the map.
    map.clear();
    if (map.size() != 0) {
        std::cerr << "Wrong size for map.  Expected 0, got "
                  << map.size() << std::endl;
        numErrors++;
    }

    // This coveres max_size.
    if (map.size() > map.max_size()) {
        std::cerr << "Bad max_size = "
                  << map.max_size() << std::endl;
        numErrors++;
    }


    return numErrors;
}
