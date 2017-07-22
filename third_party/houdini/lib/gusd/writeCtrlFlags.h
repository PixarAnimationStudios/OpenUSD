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

#ifndef __GUSD_WRITECTRLFLAGS_H__
#define __GUSD_WRITECTRLFLAGS_H__

#include <pxr/pxr.h>

#include <GT/GT_Primitive.h>

PXR_NAMESPACE_OPEN_SCOPE

// Flags indicating how we want to write geometry to a USD file. These flags 
// are initialized by the ROP but they may be modified by primitive attributes.
// Values set in geometry packed prims will be inherited by the children of that
// prim.

struct GusdWriteCtrlFlags {

    // Flags indicating what data to write when we are writing overlays
    bool overPoints;         // For point instancers, overlayPoints and overlayTransforms are synonymous.
    bool overTransforms;
    bool overPrimvars;
    bool overAll;    // Completely replace prims, including topology. 
                     // For point instancers, if overlayAll is set and 
                     // prototypes are specified, replace the prototypes.

    GusdWriteCtrlFlags() 
        : overPoints( false )
        , overTransforms( false )
        , overPrimvars( false )
        , overAll( false )
    {}

    // Update flags with values read from prims attributes.
    void update( const GT_PrimitiveHandle &prim );

    static bool getBoolAttr( 
        const GT_PrimitiveHandle& prim,
        const char *attrName,
        bool defaultValue );
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __GUSD_WRITECTRLFLAGS_H__
