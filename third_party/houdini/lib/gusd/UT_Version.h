//
// Copyright 2017 Pixar
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
/**
   \file
   \brief Helpers for dealing with versioning in the HDK.
*/
#ifndef _GUSD_UT_VERSION_H_
#define _GUSD_UT_VERSION_H_

#include <pxr/pxr.h>

#include <UT/UT_Version.h>

PXR_NAMESPACE_OPEN_SCOPE

/** Max number of versions for any component (major,minor,etc.) of the
    current build. Should be at least 1K under current conventions.*/
#define _GUSD_MAX_VERS 10000

/** Construct a single, consolidated integer value that allows legal  
    comparison between the combination of major+minor+build+patch components.*/
#define _GUSD_VER_INT(major,minor,build,patch) \
    (major*_GUSD_MAX_VERS*_GUSD_MAX_VERS*_GUSD_MAX_VERS +   \
     minor*_GUSD_MAX_VERS*_GUSD_MAX_VERS +                  \
     build*_GUSD_MAX_VERS + patch)

/** Construct a consolidated version integer for the current Houdini version.
    Each macro specifies a different level of granularity.
    @{ */
#define _GUSD_CURR_VER_INT_1 _GUSD_VER_INT(UT_MAJOR_VERSION_INT,0,0,0)

#define _GUSD_CURR_VER_INT_2 _GUSD_VER_INT(UT_MAJOR_VERSION_INT,        \
                                           UT_MINOR_VERSION_INT,0,0)
    
#define _GUSD_CURR_VER_INT_3 _GUSD_VER_INT(UT_MAJOR_VERSION_INT,    \
                                           UT_MINOR_VERSION_INT,    \
                                           UT_BUILD_VERSION_INT,0)

#define _GUSD_CURR_VER_INT_4 _GUSD_VER_INT(UT_MAJOR_VERSION_INT,    \
                                           UT_MINOR_VERSION_INT,    \
                                           UT_BUILD_VERSION_INT,    \
                                           UT_PATCH_VERSION_INT)
/* @} */

/** Compare the current Houdini version against some other version.
    
    @a op should be a comparison operator (<,>,...). The @a major, @a minor,...
    args form the right-hand side of the comparison.

    These are typically used to simplify expressions that are used to control
    compilation that must change from version to version. The different macros
    each compare the version at a different level of granularity, from the major
    version all the way down to the patch.
    
    Example usage:
    @code
    #if (GUSD_VER_CMP_2(>=, 13, 256))
    // special code path
    ...
    @endcode
    
    This is equivalent to the comparison, "major.minor >= 13.256"

    @note: Only the components of the version int specified in the comparison
    operator are compared. For example, take the following expression:
    
    @code
    GUSD_VER_CMP_2(>, 13, 1)
    @endcode

    The expression above will evaluate to _false_ when the internal Houdini
    version is 13.1.211, because only the major and minor components are
    being compared; any build difference is ignored.
    @{ */
#define GUSD_VER_CMP_1(op, major)                           \
        _GUSD_CURR_VER_INT_1 op _GUSD_VER_INT(major,0,0,0)
        
#define GUSD_VER_CMP_2(op, major, minor)                    \
    _GUSD_CURR_VER_INT_2 op _GUSD_VER_INT(major,minor,0,0)

#define GUSD_VER_CMP_3(op, major, minor, build)                     \
        _GUSD_CURR_VER_INT_3 op _GUSD_VER_INT(major,minor,build,0)

#define GUSD_VER_CMP_4(op, major, minor, build, patch)                  \
        _GUSD_CURR_VER_INT_4 op _GUSD_VER_INT(major,minor,build,patch)
/** @} */

PXR_NAMESPACE_CLOSE_SCOPE

#endif /* _GUSD_UT_VERSION_H_ */
