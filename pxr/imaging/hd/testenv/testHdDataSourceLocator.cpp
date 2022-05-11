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
static std::ostream & 
operator<<(
        std::ostream &out, const HdDataSourceLocator &locator)
{
    return out << locator.GetString();
}

static std::ostream &
operator<<(std::ostream &out, const HdDataSourceLocatorSet &locatorSet)
{
    out << "{ ";
    bool separator = false;
    for (auto const& l : locatorSet) {
        if (separator) {
           out << ", ";
        } else {
            separator = true;
        }
        out << l;
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
TestLocatorSetIntersection()
{
    {
        HdDataSourceLocatorSet locators = {
            _Parse("a/b"),
            _Parse("c/d"),
            _Parse("a/b/c"),
            _Parse("f"),
            _Parse("a/b/d"),
        };

        bool result = 
                _ValueCompare("Intersect single (parent)", 
                    locators.Intersects(_Parse("a")), true)
            && _ValueCompare("Intersect single (child)", 
                   locators.Intersects(_Parse("a/b/e")), true)
            && _ValueCompare("Intersect single (sibling)",
                   locators.Intersects(_Parse("a/c")), false)
            && _ValueCompare("Intersect single (equal)",
                   locators.Intersects(_Parse("f")), true)
            && _ValueCompare("Intersect single (unrelated)",
                   locators.Intersects(_Parse("x/y/z")), false)
            && _ValueCompare("Intersect single (empty locator)",
                   locators.Intersects(HdDataSourceLocator()), true);
        if (!result) {
            return false;
        }
    }

    {
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

            _ValueCompare("Intersect set (empty)", 
                        locators.Intersects(test1), false)
         && _ValueCompare("Intersect set (empty locator)", 
                        locators.Intersects(test2), true)
         && _ValueCompare("Intersect set (unrelated)", 
                        locators.Intersects(test3), false)
         && _ValueCompare("Intersect set (child)", locators.Intersects(test4),
                          true)
         && _ValueCompare("Intersect set (parent)", locators.Intersects(test5),
                          true)
         && _ValueCompare("Intersect set (sibling)", locators.Intersects(test6),
                          false);

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
    TEST(TestLocatorSetIntersection);

    // ------------------------------------------------------------------------
    std::cout << "DONE testHdDataSourceLocator: SUCCESS" << std::endl;
    return 0;
}
