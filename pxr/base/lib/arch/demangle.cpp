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
#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/error.h"
#include <cstdlib>
#include <typeinfo>
#include <string>
#include <string.h>

using std::string;

#if (ARCH_COMPILER_GCC_MAJOR == 3 && ARCH_COMPILER_GCC_MINOR >= 1) || \
    ARCH_COMPILER_GCC_MAJOR > 3 || defined(ARCH_COMPILER_CLANG)
#define _AT_LEAST_GCC_THREE_ONE_OR_CLANG
#endif

/*
 * The define below allows you to run both the old and new mangling schemes, 
 * and compare their results.  When you're satisfied that they always agree, 
 * comment out the define or just rewrite the code.
 *
 * NOTE: I'm commenting out the define, and not just rewriting the code, so
 * that we can be paranoid again when we switch to gcc4.0 and we want to
 * use _DemangleOld() again. 5/14/05
 *
 * NOTE: Removing _DemangleOld() from the define, so it's compiled
 * all the time. We still need it to demangle function names properly.
 * - 5/23/05.
 *
 */
//#define _PARANOID_CHECK_MODE

/*
 * Make references to "string" just a bit more understandable...
 */

static void
_FixupStringNames(string* name)
{
    static string* from = new string(typeid(string).name());
    static string* to = new string("string");

    /* Replace occurrences of stringRttiName with "string" */

    string::size_type pos = 0;
    while ((pos = name->find(*from, pos)) != string::npos) {
	name->replace(pos, from->size(), *to);
	pos += to->size();
    }

    pos = 0;
    while ((pos = name->find("std::", pos)) != string::npos) {
	name->erase(pos, 5);
    }
}

#if defined(_AT_LEAST_GCC_THREE_ONE_OR_CLANG)
#include <cxxabi.h>

/*
 * This routine doesn't work when you get to gcc3.4.
 */
static bool
_DemangleOld(string* mangledTypeName)
{
    int status;
    if (char* realName =
	    abi::__cxa_demangle(mangledTypeName->c_str(), NULL, NULL, &status))
    {
	*mangledTypeName = string(realName);
	free(realName);	    
	_FixupStringNames(mangledTypeName);
	return true;
    }

    return false;
}


/*
 * This routine should work for both gcc3.3 library and the "broken" gcc3.4
 * library.  It should also work for gcc4.0 (I think).
 * 
 * Currently this doesn't do the correct thing with function names, so
 * Arch_DemangleFunctionName has been changed to call _DemangleOld if
 * using a version of gcc >= 3.1.
 */
static bool
_DemangleNew(string* mangledTypeName)
{
    /*
     * The new gcc3.4 demangle, just like libiberty before it, doesn't like
     * simple types.  So given a mangled string xyz, we feed it Pxyz which
     * should simply pump out an extra '*' at the end.
     */

    string input("P");
    input += *mangledTypeName;

    int status;
    bool ok = false;
		 
    if (char* realName =
	abi::__cxa_demangle(input.c_str(), NULL, NULL, &status))
    {
	size_t len = strlen(realName);
	if (len > 1 && realName[len-1] == '*') {
	    *mangledTypeName = string(&realName[0], len-1);
	    _FixupStringNames(mangledTypeName);
	    ok = true;
	}
	
	free(realName);	    
    }

    return ok;
}

bool
ArchDemangle(string* mangledTypeName)
{
#if defined(_PARANOID_CHECK_MODE)
    string copy = *mangledTypeName;
    if (_DemangleNew(mangledTypeName)) {
        if (_DemangleOld(&copy) && copy != *mangledTypeName) {
            fprintf(stderr, "ArchDemangle: disagreement between old and new\n"
                    "demangling schemes: '%s' (old way) vs '%s' (new way)\n", 
                    copy.c_str(), mangledTypeName->c_str());
        }
        return true;
    }
    return false;
#else
    return _DemangleNew(mangledTypeName);
#endif // _PARANOID_CHECK_MODE
}

void
Arch_DemangleFunctionName(string* mangledFunctionName)
{
    if (mangledFunctionName->size() > 2 &&
        (*mangledFunctionName)[0] == '_' && (*mangledFunctionName)[1] == 'Z') {
        // Note: _DemangleNew isn't doing the correct thing with
        //       function names, use the old codepath. 
        _DemangleOld(mangledFunctionName);
    }
}

#endif // _AT_LEAST_GCC_THREE_ONE_OR_CLANG
