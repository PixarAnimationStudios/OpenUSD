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
#include "pxr/base/arch/function.h"
#include "pxr/base/arch/defines.h"
#include <map>

PXR_NAMESPACE_OPEN_SCOPE

using namespace std;

namespace {

// Returns the start of the type name in s that ends at i.
// For example, given:
//   s = "int Foo<A>::Bar<B, C>::Blah () [with A = int, B = float, C = bool]"
// and i = the position of "Blah" in s, then:
//   _GetStartOfName(s, i) --> the position of "Foo" in s.
static string::size_type
_GetStartOfName(const string& s, string::size_type i)
{
    // Skip backwards until we find the start of the function name.  We
    // do this by skipping over everything between matching '<' and '>'
    // and then searching for a space.
    i = s.find_last_of(" >", i);
    while (i != string::npos && s[i] != ' ') {
        int nestingDepth = 1;
        while (nestingDepth && --i) {
            if (s[i] == '>') {
                ++nestingDepth;
            }
            else if (s[i] == '<') {
                --nestingDepth;
            }
        }
        i = s.find_last_of(" >", i);
    }
    return i == string::npos ? 0 : i + 1;
}

/*
 * Finds the real name of function in prettyFunction.  If function
 * is free, it will just be function.  If function is a member,
 * there will be a "::" preceding it in prettyFunction, and we can
 * search backwards to find the class name.  If function is not in
 * prettyFunction, returns the empty string.
 *
 * For example: _GetFunctionName("Bar", "int Foo<A>::Bar () [with A = int]")
 * returns "Foo<A>::Bar"
 *
 * Note that this is full of heuristics that don't always work.
 * 
 */
static string
_GetFunctionName(const string& function, string prettyFunction)
{
    // Prepend '::' to the function name so that we can search for it as a
    // member function in prettyFunction.
    string memberFunction = "::";
    memberFunction += function;

    // First search to see if function is a member function.  If it's not,
    // then we bail out early, returning function.
    std::string::size_type functionStart = prettyFunction.find(memberFunction);
    if (functionStart == string::npos || functionStart == 0)
        return function;

    // The +2 is because of the '::' we prepended.
    std::string::size_type functionEnd = functionStart + function.length() + 2;

    // Find the start of the function name.
    string::size_type i = _GetStartOfName(prettyFunction, functionStart);

    // Cut everything that's not part of the function name out
    return prettyFunction.substr(i, functionEnd - i);
}

// Split prettyFunction into the function part and the template list part.
// For example:
//   "int Foo<A,B>::Bar(float) [with A = int, B = float]"
// becomes:
//   pair("int Foo<A,B>::Bar(float)", " A = int, B = float")
// Note the leading space in the template list.
static pair<string, string>
_Split(const string& prettyFunction)
{
    auto i = prettyFunction.find(" [with ");
    if (i != string::npos) {
        const auto n = prettyFunction.size();
        return std::make_pair(prettyFunction.substr(0, i),
                              prettyFunction.substr(i + 6, n - i - 7));
    }
    else {
        return std::make_pair(prettyFunction, std::string());
    }
}

// Split template list into a map.
// For example:
//   " A = int, B = float"
// becomes:
//   "A": "int", "B": "float"
// Note the leading space in the template list.
static std::map<string, string>
_GetTemplateList(const std::string& templates)
{
    std::map<string, string> result;

    string::size_type typeEnd = templates.size();
    string::size_type i = templates.rfind('=', typeEnd);
    while (i != string::npos) {
        auto typeStart = templates.find_first_not_of(" =", i);
        auto nameEnd   = templates.find_last_not_of(" =", i);
        auto nameStart = _GetStartOfName(templates, nameEnd);
        result[templates.substr(nameStart, nameEnd + 1 - nameStart)] =
            templates.substr(typeStart, typeEnd - typeStart);
        typeEnd = templates.find_last_not_of(" =,;", nameStart - 1) + 1;
        i = templates.rfind('=', typeEnd);
    }

    return result;
}

static string
_FormatTemplateList(const std::map<string, string>& templates)
{
    string result;
    if (!templates.empty()) {
        for (const auto& value: templates) {
            if (result.empty()) {
                result += " [with ";
            }
            else {
                result += ", ";
            }
            result += value.first;
            result += " = ";
            result += value.second;
        }
        result += "]";
    }
    return result;
}

/*
 * Finds the next template identifier in prettyFunction, starting from pos.
 * pos is updated for the next call to _GetNextIdentifier() and iteration
 * should stop when pos is std::string::npos.
 *
 * For example: _GetNextIdentifier("Foo<A, B>::Bar", 0) returns "A".
 *
 * Note that Windows does not have template lists and directly embeds the
 * types.  This only works on Windows to the extent that it parses the
 * types somehow and tries to filter an empty map, yielding an empty map,
 * which is the result we expect.  (Ultimately this is only invoked on
 * Windows for the testArchFunction test.)
 */
static string
_GetNextIdentifier(const string& prettyFunction, string::size_type &pos)
{
    // Skip '<' or leading space.
    const string::size_type first = prettyFunction.find_first_not_of("< ", pos);

    // Find the next separator, which should be a ',', unless we are on
    // the last identifier, and then it should be a '>'.  Update pos to
    // be just before the next identifier.
    std::string::size_type last = prettyFunction.find_first_of(",>", first);
    if(last == string::npos) {
        pos = string::npos;
        last = prettyFunction.find('>', first);
        if(last == string::npos)
            last = prettyFunction.size();
    }
    else if (prettyFunction[last] == ',') {
        // Skip ','.
        pos = last + 1;
    }
    else {
        // Skip to next template.
        pos = prettyFunction.find('<', first);
    }

    return prettyFunction.substr(first, last - first);
}

// Returns the elements of templates that are found in templates in
// prettyFunction.  For example, if "Foo<A, B>::Bar" is passed as
// prettyFunction then only the 'A' and 'B' elements of templates
// will be returned.
static  std::map<string, string>
_FilterTemplateList(const string& prettyFunction,
                       const std::map<string, string>& templates)
{
    std::map<string, string> result;

    string::size_type pos = prettyFunction.find("<");
    while (pos != string::npos) {
        const auto identifier = _GetNextIdentifier(prettyFunction, pos);
        auto i = templates.find(identifier);
        if (i != templates.end()) {
            result.insert(*i);
        }
    }

    return result;
}

} // anonymous namespace

string
ArchGetPrettierFunctionName(const string &function,
                            const string &prettyFunction)
{
    // Get the function signature and template list, respectively.
    const pair<string, string> parts = _Split(prettyFunction);

    // Get just the function name.
    const auto functionName = _GetFunctionName(function, parts.first);

    // Get the types from the template list.
    auto templateList = _GetTemplateList(parts.second);

    // Discard types from the template list that aren't in functionName.
    templateList = _FilterTemplateList(functionName, templateList);

    // Construct the prettier function name.
    return functionName + _FormatTemplateList(templateList);
}

PXR_NAMESPACE_CLOSE_SCOPE
