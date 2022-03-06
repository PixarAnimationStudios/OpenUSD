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


namespace pxrBaseArchFunction {

// Returns the start of the type name in s that ends at i.
// For example, given:
//   s = "int Foo<A>::Bar<B, C>::Blah () [with A = int, B = float, C = bool]"
// and i = the position of "Blah" in s, then:
//   _GetStartOfName(s, i) --> the position of "Foo" in s.
static std::string::size_type
_GetStartOfName(const std::string& s, std::string::size_type i)
{
    // Skip backwards until we find the start of the function name.  We
    // do this by skipping over everything between matching '<' and '>'
    // and then searching for a space.
    i = s.find_last_of(" >", i);
    while (i != std::string::npos && s[i] != ' ') {
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
    return i == std::string::npos ? 0 : i + 1;
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
static std::string
_GetFunctionName(const std::string& function, std::string prettyFunction)
{
    // Prepend '::' to the function name so that we can search for it as a
    // member function in prettyFunction.
    std::string memberFunction = "::";
    memberFunction += function;

    // First search to see if function is a member function.  If it's not,
    // then we bail out early, returning function.
    std::string::size_type functionStart = prettyFunction.find(memberFunction);
    if (functionStart == std::string::npos || functionStart == 0)
        return function;

    // The +2 is because of the '::' we prepended.
    std::string::size_type functionEnd = functionStart + function.length() + 2;

    // Find the start of the function name.
    std::string::size_type i = _GetStartOfName(prettyFunction, functionStart);

    // Cut everything that's not part of the function name out
    return prettyFunction.substr(i, functionEnd - i);
}

// Split prettyFunction into the function part and the template list part.
// For example:
//   "int Foo<A,B>::Bar(float) [with A = int, B = float]"
// becomes:
//   pair("int Foo<A,B>::Bar(float)", " A = int, B = float")
// Note the leading space in the template list.
static std::pair<std::string, std::string>
_Split(const std::string& prettyFunction)
{
    auto i = prettyFunction.find(" [with ");
    if (i != std::string::npos) {
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
static std::map<std::string, std::string>
_GetTemplateList(const std::string& templates)
{
    std::map<std::string, std::string> result;

    std::string::size_type typeEnd = templates.size();
    std::string::size_type i = templates.rfind('=', typeEnd);
    while (i != std::string::npos) {
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

static std::string
_FormatTemplateList(const std::map<std::string, std::string>& templates)
{
    std::string result;
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
static std::string
_GetNextIdentifier(const std::string& prettyFunction, std::string::size_type &pos)
{
    // Skip '<' or leading space.
    const std::string::size_type first = prettyFunction.find_first_not_of("< ", pos);

    // If we found nothing or '<' then this is probably operator< or <<.
    if (first == std::string::npos || prettyFunction[first] == '<') {
        pos = std::string::npos;
        return std::string();
    }

    // Find the next separator, which should be a ',', unless we are on
    // the last identifier, and then it should be a '>'.  Update pos to
    // be just before the next identifier.
    std::string::size_type last = prettyFunction.find_first_of(",>", first);
    if(last == std::string::npos) {
        pos = std::string::npos;
        last = prettyFunction.find('>', first);
        if(last == std::string::npos)
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
static std::map<std::string, std::string>
_FilterTemplateList(const std::string& prettyFunction,
                    const std::map<std::string, std::string>& templates)
{
    std::map<std::string, std::string> result;

    std::string::size_type pos = prettyFunction.find("<");
    while (pos != std::string::npos) {
        const auto identifier = _GetNextIdentifier(prettyFunction, pos);
        if (!identifier.empty()) {
            auto i = templates.find(identifier);
            if (i != templates.end()) {
                result.insert(*i);
            }
        }
    }

    return result;
}

} // anonymous namespace

std::string
ArchGetPrettierFunctionName(const std::string &function,
                            const std::string &prettyFunction)
{
    // Get the function signature and template list, respectively.
    const std::pair<std::string, std::string> parts = pxrBaseArchFunction::_Split(prettyFunction);

    // Get just the function name.
    const auto functionName = pxrBaseArchFunction::_GetFunctionName(function, parts.first);

    // Get the types from the template list.
    auto templateList = pxrBaseArchFunction::_GetTemplateList(parts.second);

    // Discard types from the template list that aren't in functionName.
    templateList = pxrBaseArchFunction::_FilterTemplateList(functionName, templateList);

    // Construct the prettier function name.
    return functionName + pxrBaseArchFunction::_FormatTemplateList(templateList);
}

PXR_NAMESPACE_CLOSE_SCOPE
