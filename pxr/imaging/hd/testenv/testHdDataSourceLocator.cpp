//
// Copyright 2021 Pixar
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
#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/base/tf/denseHashSet.h"
#include "pxr/base/tf/stringUtils.h"
#include <iostream>
#include <algorithm>

PXR_NAMESPACE_USING_DIRECTIVE

static bool
_LocatorCompare(const char *msg, const HdDataSourceLocator &loc, 
                const char *expectedStr)
{
    std::string s = loc.GetString();
    if (s != expectedStr) {
        std::cerr << msg << ": expected: \"" << expectedStr << "\" but got: \""
                  << s << "\"" << std::endl;
        return false; 
    } 
    return true;
}


// clang expects these functions to be forward declared or defined before
// the template definition (_ValueCompare) call.
template<typename T>
static std::ostream &
operator<<(std::ostream &out, const std::vector<T> &vec)
{
    out << "{ ";
    bool separator = false;
    for (auto const& e : vec) {
        if (separator) {
           out << ", ";
        } else {
            separator = true;
        }
        out << e;
    }
    out << " }";
    return out;
}

template<typename T>
bool 
_ValueCompare(const char *msg, const T &v1, const T &v2)
{
    if (v1 != v2) {
        std::cerr << msg << " expected: " << v2 << " but got: " << v1 
                  << std::endl;
        return false;
    }
    return true;
}


static HdDataSourceLocator 
_Parse(const std::string & inputStr)
{
    std::vector<TfToken> tokens;

    for (const std::string & s : TfStringSplit(inputStr, "/")) {
        if (!s.empty()) {
            tokens.push_back(TfToken(s));
        }
    }

    return HdDataSourceLocator(tokens.size(), tokens.data());
}


//-----------------------------------------------------------------------------

static bool 
TestConstructors()
{
    bool result = 

           _LocatorCompare("0 element ctor", HdDataSourceLocator(), "")

        && _LocatorCompare("1 element ctor", HdDataSourceLocator(TfToken("a")),
                           "a")
        && _LocatorCompare("2 element ctor", 
                           HdDataSourceLocator(TfToken("a"), TfToken("b")),
                           "a/b")
        && _LocatorCompare("3 element ctor",
                           HdDataSourceLocator(TfToken("a"), TfToken("b"), 
                                               TfToken("c")),
                           "a/b/c")

        && _LocatorCompare("4 element ctor",
                           HdDataSourceLocator(TfToken("a"), TfToken("b"),
                                               TfToken("c"), TfToken("d")),
                            "a/b/c/d")

        && _LocatorCompare("5 element ctor",
                           HdDataSourceLocator(TfToken("a"), TfToken("b"),
                                               TfToken("c"), TfToken("d"),
                                               TfToken("e")),
                           "a/b/c/d/e")

        && _LocatorCompare("6 element ctor",
                           HdDataSourceLocator(TfToken("a"), TfToken("b"),
                                               TfToken("c"), TfToken("d"),
                                               TfToken("e"), TfToken("f")),
                           "a/b/c/d/e/f")

        && _LocatorCompare("copy ctor",
                           HdDataSourceLocator(HdDataSourceLocator(
                                TfToken("a"), TfToken("b"), TfToken("c"),
                                TfToken("d"), TfToken("e"), TfToken("f"))),
                           "a/b/c/d/e/f");

    if (!result) {
        return false;
    }

    {
        TfTokenVector tokens = {
            TfToken("a"),
            TfToken("b"),
            TfToken("c"),
            TfToken("d"),
            TfToken("e"),
            TfToken("f"),
        };

        if (!_LocatorCompare("n elements ctor",
                HdDataSourceLocator(tokens.size(), tokens.data()),
                "a/b/c/d/e/f")) {
            return false;
        }
    }

    result =  
           _LocatorCompare("parsing", _Parse("a/b"), "a/b")
        && _LocatorCompare("parsing with leading slash", _Parse("/a/b"), "a/b");

    return result;
}

//-----------------------------------------------------------------------------

static bool 
TestEqualityAndHashing()
{
    if (!(_Parse("a/b") == _Parse("a/b"))) {
        std::cerr << "equality test failed" << std::endl;
        return false;
    }

    if (_Parse("a/b") != _Parse("a/b")) {
        std::cerr << "inequality test failed" << std::endl;
        return false;
    }

    if (_Parse("a/b") == _Parse("a/c")) {
        std::cerr << "false equality test failed" << std::endl;
        return false;
    }

    TfDenseHashSet<HdDataSourceLocator, TfHash> tokenSet;
    tokenSet.insert(_Parse("a/b"));
    tokenSet.insert(_Parse("a/b/c"));
    
    if (tokenSet.size() != 2) {
        std::cerr << "set size is expected to be 2" << std::endl;
        return false;
    }

    if (tokenSet.find(_Parse("a/b")) == tokenSet.end()) {
        std::cerr << "couldn't find a/b in set" << std::endl;
        return false;
    }

    if (tokenSet.find(_Parse("a/b/c")) == tokenSet.end()) {
        std::cerr << "couldn't find a/b/c in set" << std::endl;
        return false;
    }

    if (tokenSet.find(_Parse("a/b/d")) != tokenSet.end()) {
        std::cerr << "found non-existent a/b/d in set" << std::endl;
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------

static bool 
TestAccessors()
{
    if (!HdDataSourceLocator().IsEmpty()) {
        std::cerr << "empty locator should be reported as empty" << std::endl;
    }

    HdDataSourceLocator locator = _Parse("a/b/c");

    if (locator.IsEmpty()) {
        std::cerr << "a/b/c should not be reported as empty" << std::endl;
        return false;
    }

    if (locator.GetElementCount() != 3) {
        std::cerr << "expecting 3 for GetElementCount" << std::endl;
        return false;
    }

    bool tokenCompareResult = 
       _ValueCompare("GetElement(0)", locator.GetElement(0), TfToken("a"))
    && _ValueCompare("GetElement(1)", locator.GetElement(1), TfToken("b"))
    && _ValueCompare("GetElement(2)", locator.GetElement(2), TfToken("c"))
    && _ValueCompare("GetLastElement()", locator.GetLastElement(),
            TfToken("c"));

    if (!tokenCompareResult) {
        return false;
    }

    if (!_LocatorCompare("RemoveLastElement()", locator.RemoveLastElement(), 
                "a/b")) {
        return false;
    }

    if (!locator.HasPrefix(HdDataSourceLocator())) {
        std::cerr << "HasPrefix(emptylocator) should always be true" << std::endl;
        return false;
    }

    if (!locator.HasPrefix(locator.RemoveLastElement())) {
        std::cerr << "HasPrefix(parentlocator) should always be true" 
                  << std::endl;
        return false;
    }

    if (!locator.HasPrefix(HdDataSourceLocator(TfToken("a")))) {
        std::cerr << "HasPrefix(shallowerAncestor) should always be true"
                << std::endl;
        return false;
    }

    if (locator.HasPrefix(_Parse("a/e"))) {
        std::cerr << "HasPrefix(unrelatedlocator) false positive"
                  << std::endl;
        return false;
    }

    if (locator.GetCommonPrefix(_Parse("a/e")) != 
            HdDataSourceLocator(TfToken("a"))) {
        std::cerr << "GetCommonPrefix should equal a" << std::endl;
        return false;
    }


    if (locator.GetCommonPrefix(_Parse("e/f")) != HdDataSourceLocator()) {
        std::cerr << "GetCommonPrefix should be empty" << std::endl;
        return false;
    }


    return true;
}

//-----------------------------------------------------------------------------

static bool 
TestAppendsAndReplaces()
{
    HdDataSourceLocator locator = _Parse("a/b/c");

    return
        _LocatorCompare("ReplaceLastElement",
                locator.ReplaceLastElement(TfToken("z")), "a/b/z")
        && _LocatorCompare("Append", locator.Append(TfToken("z")), "a/b/c/z")
        && _LocatorCompare("Append", locator.Append(locator), "a/b/c/a/b/c")

        && _LocatorCompare("ReplacePrefix", 
                           locator.ReplacePrefix(_Parse("a"), _Parse("X/Y")),
                           "X/Y/b/c")

        && _LocatorCompare("ReplacePrefix with empty",
                           locator.ReplacePrefix(_Parse("a/b"),
                           HdDataSourceLocator()), "c")

        && _LocatorCompare("ReplacePrefix with unrelated locator",
                           locator.ReplacePrefix(_Parse("X/Y"), 
                           HdDataSourceLocator()), "a/b/c");
}

//-----------------------------------------------------------------------------

static bool 
TestIntersection()
{
    return 
        _ValueCompare( "Intersect against empty: ", 
            HdDataSourceLocator(TfToken("a")).Intersects(HdDataSourceLocator()),
            true)

    && _ValueCompare( "Intersect equal: ", 
            HdDataSourceLocator(TfToken("a")).Intersects(
                HdDataSourceLocator(TfToken("a"))), true)

    && _ValueCompare( "Intersect nested: ",
            _Parse("a/b/c").Intersects(_Parse("a")), true)

    && _ValueCompare( "Intersect unrelated: ", 
            _Parse("a/b/c").Intersects(_Parse("d/e")), false)

    && _ValueCompare( "Intersect siblings: ",
            _Parse("a/b/c").Intersects(_Parse("a/b/d")), false);
}

//-----------------------------------------------------------------------------


bool TestLocatorSet()
{
    {
        HdDataSourceLocatorSet locators;
        locators.insert(_Parse("a/b"));
        locators.insert(_Parse("c/b"));

        HdDataSourceLocatorSet baseline = {
            _Parse("a/b"),
            _Parse("c/b"),
        };

        if (!_ValueCompare("Insert exclusion (non-intersecting): ", 
                    locators, baseline)) {
            return false;
        }
    }

    {
        HdDataSourceLocatorSet locators;
        locators.insert(_Parse("a/b"));
        locators.insert(_Parse("c/d"));
        locators.insert(_Parse("a/b/c"));
        locators.insert(_Parse("f"));
        locators.insert(_Parse("a/b/d"));

        HdDataSourceLocatorSet baseline;
        baseline.insert(_Parse("a/b"));
        baseline.insert(_Parse("c/d"));
        baseline.insert(_Parse("f"));

        if (!_ValueCompare("Insert exclusion (intersecting, single): ", 
                           locators, baseline)) {
                return false;
        }
    }

    {
        HdDataSourceLocatorSet locators = {
            _Parse("a/b"),
            _Parse("c/d"),
            _Parse("a/b/c"),
        };
        HdDataSourceLocatorSet locators2 = {
            _Parse("f"),
            _Parse("a/b/d"),
        };
        locators.insert(locators2);

        HdDataSourceLocatorSet baseline;
        baseline.insert(_Parse("a/b"));
        baseline.insert(_Parse("c/d"));
        baseline.insert(_Parse("f"));

        if (!_ValueCompare("Insert exclusion (intersecting, set): ", 
                           locators, baseline)) {
            return false;
        }
    }

    {
        HdDataSourceLocatorSet locators;
        locators.insert(_Parse("a/b"));
        locators.insert(_Parse("a/b/c"));
        locators.insert(_Parse("q/e/d"));
        locators.insert(HdDataSourceLocator());

        HdDataSourceLocatorSet baseline;
        baseline.insert(HdDataSourceLocator());

        if (!_ValueCompare("Insert exclusion (empty locator): ", 
                           locators, baseline)) {
            return false;
        }
    }

    {
        HdDataSourceLocatorSet locators;
        locators.insert(_Parse("a/b"));
        locators.insert(_Parse("c"));

        bool result = 
               _ValueCompare("intersection 1: ",
                        locators.Intersects(_Parse("c/d")), true)
            && _ValueCompare("intersection 1: ",
                        locators.Intersects(_Parse("e/f")), false);

        if (!result) {
            return false;
        }
    }

    return true;
}

//-----------------------------------------------------------------------------

static bool
TestLocatorSetIntersects()
{
    {
        // Exercise code-path where size is smaller than _binarySearchCuroff.

        HdDataSourceLocatorSet locators = {
            _Parse("a/b"),
            _Parse("c/d"),
            _Parse("a/b/c"),
            _Parse("f"),
            _Parse("a/b/d"),
        };

        bool result = 
                _ValueCompare("Intersect single (parent, small set)", 
                    locators.Intersects(_Parse("a")), true)
            && _ValueCompare("Intersect single (child, small set)", 
                   locators.Intersects(_Parse("a/b/e")), true)
            && _ValueCompare("Intersect single (sibling, small set)",
                   locators.Intersects(_Parse("a/c")), false)
            && _ValueCompare("Intersect single (equal, small set)",
                   locators.Intersects(_Parse("f")), true)
            && _ValueCompare("Intersect single (unrelated, small set)",
                   locators.Intersects(_Parse("x/y/z")), false)
            && _ValueCompare("Intersect single (empty locator, small set)",
                   locators.Intersects(HdDataSourceLocator()), true);
        if (!result) {
            return false;
        }
    }

    {
        // Exercise code-path where size is larger than _binarySearchCuroff.

        HdDataSourceLocatorSet locators = {
            _Parse("a/b"),
            _Parse("c/d"),
            _Parse("f"),
            _Parse("g/a"),
            _Parse("g/b"),
            _Parse("g/c"),
            _Parse("g/d"),
            _Parse("g/e"),
            _Parse("g/f"),
            _Parse("g/g"),
        };

        bool result = 
                _ValueCompare("Intersect single (parent, large set)", 
                    locators.Intersects(_Parse("a")), true)
            && _ValueCompare("Intersect single (child, large set)", 
                   locators.Intersects(_Parse("a/b/e")), true)
            && _ValueCompare("Intersect single (sibling, large set)",
                   locators.Intersects(_Parse("a/c")), false)
            && _ValueCompare("Intersect single (equal, large set)",
                   locators.Intersects(_Parse("f")), true)
            && _ValueCompare("Intersect single (unrelated, large set)",
                   locators.Intersects(_Parse("x/y/z")), false)
            && _ValueCompare("Intersect single (empty locator, large set)",
                   locators.Intersects(HdDataSourceLocator()), true);
        if (!result) {
            return false;
        }
    }

    {
        // Exercise code-path where size is smaller than _zipperCompareCutoff

        HdDataSourceLocatorSet locators = {
            _Parse("a/b"),
            _Parse("c/d"),
            _Parse("a/b/c"),
            _Parse("f"),
            _Parse("a/b/d"),
        };

        HdDataSourceLocatorSet test1;
        HdDataSourceLocatorSet test2 = { HdDataSourceLocator() };
        HdDataSourceLocatorSet test3 = {
            _Parse("g/h/i"),
            _Parse("q/r/s"),
        };
        HdDataSourceLocatorSet test4 = {
            _Parse("a/b/z"),
            _Parse("f/g/h"),
        };
        HdDataSourceLocatorSet test5 = {
            _Parse("a"),
            _Parse("z"),
        };
        HdDataSourceLocatorSet test6 = {
            _Parse("a/c"),
            _Parse("z"),
        };

        const bool result = 

            _ValueCompare("Intersect set (empty, small sets)", 
                        locators.Intersects(test1), false)
         && _ValueCompare("Intersect set (empty locator, small sets)", 
                        locators.Intersects(test2), true)
         && _ValueCompare("Intersect set (unrelated, small sets)", 
                        locators.Intersects(test3), false)
         && _ValueCompare("Intersect set (child, small sets)",
                        locators.Intersects(test4), true)
         && _ValueCompare("Intersect set (parent, small sets)",
                        locators.Intersects(test5), true)
         && _ValueCompare("Intersect set (sibling, small sets)",
                        locators.Intersects(test6), false);

        if (!result) {
            return false;
        }
    }

    {
        // Exercise code-path where size is larger than _zipperCompareCutoff

        HdDataSourceLocatorSet locators = {
            _Parse("a/b"),
            _Parse("c/d"),
            _Parse("a/b/c"),
            _Parse("f"),
            _Parse("a/b/d"),
            _Parse("g/a"),
            _Parse("g/b"),
            _Parse("g/c"),
            _Parse("g/d"),
            _Parse("g/e"),
            _Parse("g/f"),
            _Parse("g/g"),
        };

        HdDataSourceLocatorSet test2 = { HdDataSourceLocator() };
        HdDataSourceLocatorSet test3 = {
            _Parse("g/h/i"),
            _Parse("q/r/s"),
        };
        HdDataSourceLocatorSet test4 = {
            _Parse("a/b/z"),
            _Parse("f/g/h"),
        };
        HdDataSourceLocatorSet test5 = {
            _Parse("a"),
            _Parse("z"),
        };
        HdDataSourceLocatorSet test6 = {
            _Parse("a/c"),
            _Parse("z"),
        };

        const bool result = 

            _ValueCompare("Intersect set (empty locator, large sets)", 
                        locators.Intersects(test2), true)
         && _ValueCompare("Intersect set (unrelated, large sets)", 
                        locators.Intersects(test3), false)
         && _ValueCompare("Intersect set (child, large sets)",
                        locators.Intersects(test4), true)
         && _ValueCompare("Intersect set (parent, large sets)",
                        locators.Intersects(test5), true)
         && _ValueCompare("Intersect set (sibling, large sets)",
                        locators.Intersects(test6), false);

        if (!result) {
            return false;
        }
    }

    {
        HdDataSourceLocatorSet test1;
        HdDataSourceLocatorSet test2 = { HdDataSourceLocator() };

        const bool result =

              _ValueCompare("Intersect empty set vs empty locator", 
                          test1.Intersects(test2), false)
           && _ValueCompare("Intersect empty set vs empty set", 
                          test1.Intersects(test1), false)
           && _ValueCompare("Intersect empty locator vs empty locator", 
                          test2.Intersects(test2), true)
           && _ValueCompare("Intersect empty locator vs empty set", 
                          test2.Intersects(test1), false);

        if (!result) {
            return false;
        }
    }

    return true;
}

static bool
TestLocatorSetContains()
{
    {
        HdDataSourceLocatorSet locators;
        
        const bool result =
                _ValueCompare(
                    "Empty set contains nothing (empty locator)",
                    locators.Contains(_Parse("")), false)
            &&  _ValueCompare(
                    "Empty set contains nothing (non-empty locator 1)",
                    locators.Contains(_Parse("c")), false)
            &&  _ValueCompare(
                    "Empty set contains nothing (non-empty locator 2)",
                    locators.Contains(_Parse("c/d")), false);
        if (!result) {
            return false;
        }
    }

    {
        HdDataSourceLocatorSet locators = {
            _Parse("")
        };

        const bool result =
                _ValueCompare(
                    "Universal set contains everything (empty locator)",
                    locators.Contains(_Parse("")), true)
            &&  _ValueCompare(
                    "Universal set contains everything (non-empty locator 1)",
                    locators.Contains(_Parse("c")), true)
            &&  _ValueCompare(
                    "Universal set contains everything (non-empty locator 2)",
                    locators.Contains(_Parse("c/d")), true);
        if (!result) {
            return false;
        }
    }

    {
        // Exercise code-path where size is smaller than _binarySearchCuroff.

        HdDataSourceLocatorSet locators = {
            _Parse("c"),
            _Parse("f/g")
        };

        const bool result =
                _ValueCompare(
                    "Membership test 1 (small set)",
                    locators.Contains(_Parse("")), false)
            &&  _ValueCompare(
                    "Membership test 2 (small set)",
                    locators.Contains(_Parse("b")), false)
            &&  _ValueCompare(
                    "Membership test 3 (small set)",
                    locators.Contains(_Parse("b/c")), false)
            &&  _ValueCompare(
                    "Membership test 4 (small set)",
                    locators.Contains(_Parse("c")), true)
            &&  _ValueCompare(
                    "Membership test 5 (small set)",
                    locators.Contains(_Parse("c/d")), true)
            &&  _ValueCompare(
                    "Membership test 6 (small set)",
                    locators.Contains(_Parse("d")), false)
            &&  _ValueCompare(
                    "Membership test 7 (small set)",
                    locators.Contains(_Parse("f")), false)
            &&  _ValueCompare(
                    "Membership test 8 (small set)",
                    locators.Contains(_Parse("f/g")), true)
            &&  _ValueCompare(
                    "Membership test 9 (small set)",
                    locators.Contains(_Parse("f/g/h")), true) 
            &&  _ValueCompare(
                    "Membership test 10 (small set)",
                    locators.Contains(_Parse("g")), false); 

        if (!result) {
            return false;
        }
    }

    {
        // Exercise code-path where size is larger than _binarySearchCuroff.

        HdDataSourceLocatorSet locators = {
            _Parse("c"),
            _Parse("e/a"),
            _Parse("e/b"),
            _Parse("e/c"),
            _Parse("e/d"),
            _Parse("e/e"),
            _Parse("e/f"),
            _Parse("e/g"),
            _Parse("e/h"),
            _Parse("e/i"),
            _Parse("e/j"),
            _Parse("e/k"),
            _Parse("e/l"),
            _Parse("f/g") };
        
        const bool result =
                _ValueCompare(
                    "Membership test 1 (large set)",
                    locators.Contains(_Parse("")), false)
            &&  _ValueCompare(
                    "Membership test 2 (large set)",
                    locators.Contains(_Parse("b")), false)
            &&  _ValueCompare(
                    "Membership test 3 (large set)",
                    locators.Contains(_Parse("b/c")), false)
            &&  _ValueCompare(
                    "Membership test 4 (large set)",
                    locators.Contains(_Parse("c")), true)
            &&  _ValueCompare(
                    "Membership test 5 (large set)",
                    locators.Contains(_Parse("c/d")), true)
            &&  _ValueCompare(
                    "Membership test 6 (large set)",
                    locators.Contains(_Parse("d")), false)
            &&  _ValueCompare(
                    "Membership test 7 (large set)",
                    locators.Contains(_Parse("f")), false)
            &&  _ValueCompare(
                    "Membership test 8 (large set)",
                    locators.Contains(_Parse("f/g")), true)
            &&  _ValueCompare(
                    "Membership test 9 (large set)",
                    locators.Contains(_Parse("f/g/h")), true) 
            &&  _ValueCompare(
                    "Membership test 10 (large set)",
                    locators.Contains(_Parse("g")), false); 

        if (!result) {
            return false;
        }
    }

    return true;
}

static bool
TestLocatorSetReplaces()
{
    // Empty locator set.
    {
        HdDataSourceLocatorSet locators;
        HdDataSourceLocatorSet baseline = locators;

        const bool result =
                _ValueCompare(
                    "Replace empty set having empty prefix with foo",
                    locators.ReplacePrefix(
                        HdDataSourceLocator::EmptyLocator(), _Parse("foo")),
                     baseline)
            &&  _ValueCompare(
                    "Replace empty set having the prefix foo with bar",
                    locators.ReplacePrefix(
                        _Parse("foo"), _Parse("bar")),
                    baseline);
        if (!result) {
            return false;
        }
    }

    // Universal locator set.
    {
        HdDataSourceLocatorSet locators = {
            _Parse("")
        };
        HdDataSourceLocatorSet baseline = {
            _Parse("foo")
        };

        const bool result =
                _ValueCompare(
                    "Replace universal set having empty prefix with foo",
                    locators.ReplacePrefix(
                        HdDataSourceLocator::EmptyLocator(), _Parse("foo")),
                     baseline)
            &&  _ValueCompare(
                    "Replace universal set having the prefix foo with bar",
                    locators.ReplacePrefix(
                        _Parse("foo"), _Parse("bar")),
                    locators);
        if (!result) {
            return false;
        }
    }

    // Exercise code-path where size is smaller than _binarySearchCuroff.
    {
        {
            HdDataSourceLocatorSet locators = {
                _Parse("a/a/c"),
                _Parse("a/c/d"),
                _Parse("a/c/e"),
                _Parse("a/d/e"),
            };
            HdDataSourceLocatorSet baseline2 = {
                _Parse("a/a/c"),
                _Parse("X/Y/d"),
                _Parse("X/Y/e"),
                _Parse("a/d/e"),
            };
            HdDataSourceLocatorSet baseline3 = {
                _Parse("a/a/c"),
                _Parse("a/d/d"),
                _Parse("a/d/e"),
            };
            HdDataSourceLocatorSet baseline4 = {
                _Parse("X/Y/a/a/c"),
                _Parse("X/Y/a/c/d"),
                _Parse("X/Y/a/c/e"),
                _Parse("X/Y/a/d/e"),
            };
            HdDataSourceLocatorSet baseline5 = {
                _Parse("a/c"),
                _Parse("c/d"),
                _Parse("c/e"),
                _Parse("d/e"),
            };
            HdDataSourceLocatorSet baseline6 = {
                _Parse("a/a/c"),
                _Parse("b"),
                _Parse("a/c/e"),
                _Parse("a/d/e"),
            };
            const bool result =
                    _ValueCompare(
                        "Replace test 1 (prefix not matched) (small set)",
                        locators.ReplacePrefix(_Parse("a/b"), _Parse("a/d")),
                        locators)
                &&  _ValueCompare(
                        "Replace test 2 (small set)",
                        locators.ReplacePrefix(_Parse("a/c"), _Parse("X/Y")),
                        baseline2)
                &&  _ValueCompare(
                        "Replace test 3 w/ uniquify (small set)",
                        locators.ReplacePrefix(_Parse("a/c"), _Parse("a/d")),
                        baseline3)
                &&  _ValueCompare(
                        "Replace test 4 (empty prefix match) (small set)",
                        locators.ReplacePrefix(_Parse(""), _Parse("X/Y")),
                        baseline4)
                &&  _ValueCompare(
                        "Replace test 5 (prefix changed to empty) (small set)",
                        locators.ReplacePrefix(_Parse("a/"), _Parse("")),
                        baseline5)
                &&  _ValueCompare(
                        "Replace test 6 (full prefix match) (small set)",
                        locators.ReplacePrefix(_Parse("a/c/d"), _Parse("b")),
                        baseline6);

            if (!result) {
                return false;
            }
        }
    }

    // Exercise code-path where size is larger than _binarySearchCuroff.
    {
        HdDataSourceLocatorSet locators = {
            _Parse("a/b"),
            _Parse("a/c/d"),
            _Parse("a/c/e/f"),
            _Parse("a/c/e/g"),
            _Parse("g/a"),
            _Parse("g/b"),
            _Parse("g/c/c"),
            _Parse("g/d/b"),
        };
        HdDataSourceLocatorSet baseline2 = {
            _Parse("a/b"),
            _Parse("X/Y/d"),
            _Parse("X/Y/e/f"),
            _Parse("X/Y/e/g"),
            _Parse("g/a"),
            _Parse("g/b"),
            _Parse("g/c/c"),
            _Parse("g/d/b"),
        };
        HdDataSourceLocatorSet baseline3 = {
            _Parse("a/b"),
            _Parse("g/a"),
            _Parse("g/b"),
            _Parse("g/c/c"),
            _Parse("g/d/b"),
        };
        HdDataSourceLocatorSet baseline4 = {
            _Parse("X/Y/a/b"),
            _Parse("X/Y/a/c/d"),
            _Parse("X/Y/a/c/e/f"),
            _Parse("X/Y/a/c/e/g"),
            _Parse("X/Y/g/a"),
            _Parse("X/Y/g/b"),
            _Parse("X/Y/g/c/c"),
            _Parse("X/Y/g/d/b"),
        };
        HdDataSourceLocatorSet baseline5 = {
            _Parse("a"),
            _Parse("a/c/d"),
            _Parse("a/c/e/f"),
            _Parse("a/c/e/g"),
            _Parse("b"),
            _Parse("c/c"),
            _Parse("d/b"),
        };
        HdDataSourceLocatorSet baseline6 = {
            _Parse("a/b"),
            _Parse("a/c/d"),
            _Parse("b"),
            _Parse("a/c/e/g"),
            _Parse("g/a"),
            _Parse("g/b"),
            _Parse("g/c/c"),
            _Parse("g/d/b"),
        };

        const bool result =
                    _ValueCompare(
                        "Replace test 1 (prefix not matched) (large set)",
                        locators.ReplacePrefix(_Parse("a/d"), _Parse("a/c")),
                        locators)
                &&  _ValueCompare(
                        "Replace test 2 (large set)",
                        locators.ReplacePrefix(_Parse("a/c"), _Parse("X/Y")),
                        baseline2)
                &&  _ValueCompare(
                        "Replace test 3 w/ uniquify (large set)",
                        locators.ReplacePrefix(_Parse("a/c"), _Parse("g/b")),
                        baseline3)
                &&  _ValueCompare(
                        "Replace test 4 (empty prefix match) (large set)",
                        locators.ReplacePrefix(_Parse(""), _Parse("X/Y")),
                        baseline4)
                &&  _ValueCompare(
                        "Replace test 5 (prefix changed to empty) (large set)",
                        locators.ReplacePrefix(_Parse("g/"), _Parse("")),
                        baseline5)
                &&  _ValueCompare(
                        "Replace test 6 (full prefix match) (large set)",
                        locators.ReplacePrefix(_Parse("a/c/e/f"), _Parse("b")),
                        baseline6);

            if (!result) {
                return false;
            }
    }

    return true;
}

static
std::vector<HdDataSourceLocator>
_ToVector(const HdDataSourceLocatorSet::IntersectionView &v)
{
    return {v.begin(), v.end()};
}

static bool
TestLocatorSetIntersection()
{
    const HdDataSourceLocator empty;
    const HdDataSourceLocator mesh(TfToken("mesh"));
    const HdDataSourceLocator primvars(TfToken("primvars"));
    const HdDataSourceLocator primvarsColor =
        primvars.Append(TfToken("color"));
    const HdDataSourceLocator primvarsColorInterpolation =
        primvarsColor.Append(TfToken("interpolation"));
    const HdDataSourceLocator primvarsOpacity =
        primvars.Append(TfToken("opacity"));

    {
        const HdDataSourceLocatorSet locators;

        const bool result =
                _ValueCompare(
                    "Compute intersection of empty locator set with empty locator",
                    _ToVector(locators.Intersection(empty)),
                    { })
            &&  _ValueCompare(
                    "Compute intersection of empty locator with non-empty locator",
                    _ToVector(locators.Intersection(primvars)),
                    { });

        if (!result) {
            return false;
        }
    }

    {
        const HdDataSourceLocatorSet locators{empty};

        const bool result =
                _ValueCompare(
                    "Compute intersection of empty locator set with empty locator",
                    _ToVector(locators.Intersection(empty)),
                    { empty })
            &&  _ValueCompare(
                    "Compute intersection of empty locator with non-empty locator",
                    _ToVector(locators.Intersection(primvars)),
                    { primvars });

        if (!result) {
            return false;
        }
    }

    {
        const HdDataSourceLocatorSet locators{mesh, primvars};

        const bool result =
                _ValueCompare(
                    "D",
                    _ToVector(locators.Intersection(empty)),
                    { mesh, primvars })
            &&  _ValueCompare(
                    "A",
                    _ToVector(locators.Intersection(mesh)),
                    { mesh })
            &&  _ValueCompare(
                    "B",
                    _ToVector(locators.Intersection(primvars)),
                    { primvars })
            &&  _ValueCompare(
                    "C",
                    _ToVector(locators.Intersection(primvarsColor)),
                    { primvarsColor });

        if (!result) {
            return false;
        }
    }

    {
        const HdDataSourceLocatorSet locators{
            mesh,
            primvarsColorInterpolation,
            primvarsOpacity};

        const bool result =
                _ValueCompare(
                    "E",
                    _ToVector(locators.Intersection(primvars)),
                    { primvarsColorInterpolation,
                      primvarsOpacity })
            &&  _ValueCompare(
                    "F",
                    _ToVector(locators.Intersection(primvarsColor)),
                    { primvarsColorInterpolation });

        if (!result) {
            return false;
        }
    }

    {
        // Trigger path where we actually do binary search.

        const HdDataSourceLocatorSet locators{
            mesh,
            primvarsColorInterpolation,
            primvarsOpacity,
            HdDataSourceLocator(TfToken("za")),
            HdDataSourceLocator(TfToken("zb")),
            HdDataSourceLocator(TfToken("zc")),
            HdDataSourceLocator(TfToken("zd")),
            HdDataSourceLocator(TfToken("ze")),
            HdDataSourceLocator(TfToken("zf")),
            HdDataSourceLocator(TfToken("zg")),
            HdDataSourceLocator(TfToken("zh")),
            HdDataSourceLocator(TfToken("zi")),
            HdDataSourceLocator(TfToken("zj"))};

        const bool result =
                _ValueCompare(
                    "E",
                    _ToVector(locators.Intersection(primvars)),
                    { primvarsColorInterpolation,
                      primvarsOpacity })
            &&  _ValueCompare(
                    "F",
                    _ToVector(locators.Intersection(primvarsColor)),
                    { primvarsColorInterpolation });

        if (!result) {
            return false;
        }
    }

    {
        const HdDataSourceLocatorSet locators{
            mesh,
            primvarsColor,
            primvarsOpacity};
        const HdDataSourceLocatorSet::IntersectionView v =
            locators.Intersection(primvars);

        std::vector<TfToken> lastElementsIntersection;
        for (auto it = v.begin(); it != v.end(); it++) {
            lastElementsIntersection.push_back(it->GetLastElement());
        }

        const bool result =
            _ValueCompare(
                "Test IntersectionIterator::operator-> and post increment",
                lastElementsIntersection,
                { primvarsColor.GetLastElement(),
                  primvarsOpacity.GetLastElement() });

        if (!result) {
            return false;
        }
    }


    return true;
}

//-----------------------------------------------------------------------------

#define xstr(s) str(s)
#define str(s) #s
#define TEST(X) std::cout << (++i) << ") " <<  str(X) << "..." << std::endl; \
if (!X()) { std::cout << "FAILED" << std::endl; return -1; } \
else std::cout << "...SUCCEEDED" << std::endl;

int main(int argc, char**argv)
{
    std::cout << "STARTING testHdDataSourceLocator" << std::endl;
    // ------------------------------------------------------------------------

    int i = 0;
    TEST(TestConstructors);
    TEST(TestEqualityAndHashing);
    TEST(TestAccessors);
    TEST(TestAppendsAndReplaces);
    TEST(TestIntersection);
    TEST(TestLocatorSet);
    TEST(TestLocatorSetIntersects);
    TEST(TestLocatorSetContains);
    TEST(TestLocatorSetReplaces);
    TEST(TestLocatorSetIntersection);

    // ------------------------------------------------------------------------
    std::cout << "DONE testHdDataSourceLocator: SUCCESS" << std::endl;
    return 0;
}
