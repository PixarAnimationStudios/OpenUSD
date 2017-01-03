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
#include "pxr/base/arch/function.h"
#include "pxr/base/arch/defines.h"

#include <ciso646>

using namespace std;

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
_GetFunctionName(string function, string prettyFunction)
{
    // Prepend '::' to the function name so that we can search for it as a
    // member function in prettyFunction.
    string memberFunction = "::";
    memberFunction += function;

    // First search to see if function is a member function.  If it's not,
    // then we bail out early, returning 'function'.
    std::string::size_type functionStart = prettyFunction.find(memberFunction);
    if (functionStart == string::npos || functionStart == 0)
        return function;

    // The +2 is because of the '::' we prepended.
    std::string::size_type functionEnd = functionStart + function.length() + 2;

    // Template arguments are separated by a ", ", so we search
    // for ' ' backwards from the start of the function name until
    // we get to one that isn't preceded by a comma
    do {
        functionStart = prettyFunction.rfind(' ', functionStart - 1);
    } while(functionStart != string::npos && functionStart > 0
            && prettyFunction[functionStart - 1] == ',');

    // Cut everything that's not part of the function name out
    prettyFunction.erase(functionEnd);
    if (functionStart != string::npos) {
        prettyFunction.erase(0, ++functionStart);
    }

    return prettyFunction;
}

#if defined(ARCH_COMPILER_GCC) || defined(ARCH_COMPILER_CLANG)
/*
 * Extracts the list of template values from prettyFunction. 
 *
 * For example: _GccGetTemplateString("int X<A, B>() [with A = int, B = char]")
 * returns " A = int, B = char"
 */
static string
_GccGetTemplateString(string prettyFunction)
{
    size_t withPos;

    // If there's a "[with" in the string, get everything from the
    // end of that to the last "]"
    if((withPos = prettyFunction.find("[with")) != string::npos) {
        withPos += 5; // 5 is the length of "with"
        
        // We need to leave a space on the front to make looking for whole
        // identifiers easier.
        // Something like "A = int" becomes " A = int", so we can find " A ",
        // and the fact that it's at the start of the string isn't a special
        // case.

        string::size_type i = prettyFunction.rfind(']');
        if (i != string::npos) {
            return prettyFunction.substr(withPos, i - withPos);
        }
    }
    return "";
}

/*
 * Finds the next template identifier in prettyFunction, starting from pos,
 * until a '>' is reached.  If there are no more identifiers, returns
 * an empty string.
 *
 * For example: _GccGetNextIdentifier("Foo<A, B>::Bar", 0) returns "A".
 */
static string
_GccGetNextIdentifier(string prettyFunction, size_t &pos)
{
    // If we're in front of the '<', move to it
    std::string::size_type i = prettyFunction.find("<");
    if(i != std::string::npos && pos < i) {
        pos = i;
    }
    
    // There is no closing '>' or we've passed it
    if(prettyFunction.find('>', pos) == string::npos) {
        pos = string::npos;
        return "";
    }

    // Find the next separator, which should be a ',', unless we are on
    // the last identifier, and then it should be a '>'
    size_t nextPos = prettyFunction.find(',', pos);
    if(nextPos == string::npos)
        nextPos = prettyFunction.find('>', pos);

    pos ++;
    string identifier = prettyFunction.substr(pos, nextPos - pos);

    // Update our position to be the end of the identifier we just got
    pos = nextPos;
    if(pos != string::npos) pos ++;
    return identifier;

}

/*
 * Finds the value of identifier in the list of templates.  templates
 * must be a string of any number of " IDEN = VALUE" separated by commas.
 *
 * For example: _GccGetIdentifierValue("A", " A = int") returns "int".
 */
static string
_GccGetIdentifierValue(string identifier, string templates)
{
    // Search for the identifier surrounded by spaces to make sure
    // we get exactly the right one
    size_t start = templates.find(' ' + identifier + ' ');
    if(start == string::npos) return "";
    start ++;

    // The first '=' we find will be the identifier's
    size_t end = templates.find('=', start);
    if(end == string::npos) return "";

    // This '=' is for the next identifier
    end = templates.find('=', end + 1);

    // If we found a second '=', search back to find our closing ',',
    // otherwise, we were the last identifier, and we want to get the 
    // rest of the string anyway, so npos is ok
    if(end != string::npos)
        end = templates.rfind(',', end);

    return templates.substr(start, end - start) + ", ";
}

/*
 * Constructs a string with all the template values used by prettyFunction,
 * using the list in templates.
 *
 * For example:
 * _GccMakePrettyTemplateString("Foo<A>::Bar", " A = int, B = char")
 * returns "[with A = int]".
 */
static string
_GccMakePrettyTemplateString(string prettyFunction, string templates)
{
    size_t pos = 0;
    string prettyTemplates;

    do {
        string identifier = _GccGetNextIdentifier(prettyFunction, pos);
        prettyTemplates += _GccGetIdentifierValue(identifier, templates);
    } while(pos != string::npos);

    // remove the last ", " and put the brackets on
    if(prettyTemplates.length() >= 2) {
        prettyTemplates.erase(prettyTemplates.length() - 2, 2);
        prettyTemplates = " [with " + prettyTemplates + "]";
    }

    return prettyTemplates;
}

/*
 * Given function as gcc's __FUNCTION__ and prettyFunction as
 * gcc's __PRETTY_FUNCTION__, attempts to construct an even
 * prettier function name.  Removes information about return
 * types and arguments and reconstructs a list of templates used.
 *
 * For example:
 * _GccMakePrettierFunctionName("Bar", "int Foo<A>::Bar(float) [with A = int]")
 * returns "Foo<A>::Bar [with A = int]".
 */
static string
_GccGetPrettierFunctionName(string function, string prettyFunction)
{
    string templates = _GccGetTemplateString(prettyFunction);
    prettyFunction = _GetFunctionName(function, prettyFunction);

    if(templates.empty() || prettyFunction.find("<") == string::npos)
        return prettyFunction;

    templates = _GccMakePrettyTemplateString(prettyFunction, templates);

    return prettyFunction + templates;
}
#endif

#if defined(ARCH_COMPILER_ICC)
/*
 * Given function as icc's __FUNCTION__ and prettyFunction as
 * icc's __PRETTY_FUNCTION__, attempts to construct an even
 * prettier function name.  Removes information about return
 * types and arguments and gets rid of "std::"
 *
 * For example:
 * _IccMakePrettierFunctionName("Bar", "int Foo<std::string>::Bar(float)")
 * returns "Foo<string>::Bar".
 */
static string
_IccMakePrettierFunctionName(string function, string prettyFunction)
{
    prettyFunction = _GetFunctionName(function, prettyFunction);
    size_t stdPos;
    while((stdPos = prettyFunction.find("std::")) != string::npos)
        prettyFunction.erase(stdPos, 5);
    return prettyFunction;
}
#endif

string
ArchGetPrettierFunctionName(const string &function,
                            const string &prettyFunction)
{
#if defined(ARCH_COMPILER_GCC) || defined(ARCH_COMPILER_CLANG)
    return _GccGetPrettierFunctionName(function, prettyFunction);
#elif defined(ARCH_COMPILER_ICC)
    return _IccGetPrettierFunctionName(function, prettyFunction);
#else
    return function;
#endif
}
