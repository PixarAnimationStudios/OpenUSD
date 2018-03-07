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

#ifndef __GUSD_CONTEXT_H__
#define __GUSD_CONTEXT_H__

#include <pxr/pxr.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/tokens.h>

#include <GT/GT_Primitive.h>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class GusdGT_AttrFilter;
class UsdGeomXformCache;

/// A GusdContext structure is created by the ROPs that write USD files and
/// passed to the GusdPrimWrappers to control how they are written to the USD file.

class GusdContext {
public:
    typedef boost::function<UsdStageRefPtr ()> GetStageFunc;
    enum Granularity { ONE_FILE, PER_FRAME };

    GusdContext( UsdTimeCode t, 
                 Granularity g,
                 const GusdGT_AttrFilter& af ) 
        : time( t )
        , granularity( g )
	    , writeOverlay( false )
        , overlayPoints( false )
        , overlayTransforms( false )
        , overlayPrimvars( false )
        , overlayAll( false )
        , writeStaticGeo( false )
        , writeStaticTopology( false )
        , writeStaticPrimvars( false )
        , attributeFilter( af )
        , purpose( UsdGeomTokens->default_ )
        , makeRefsInstanceable( true )
    {}

    // Time of the current frame we are writing
    UsdTimeCode time;

    // Are we writing on frame per file or all the frames into a single file?
    Granularity granularity;

    bool writeOverlay;    // Overlay existing geometry rather than creating new geometry?

    // Flags indicating what should be overlayed
    bool overlayPoints;         // For point instancers, overlayPoints and overlayTransforms are synonymous.π
    bool overlayTransforms;
    bool overlayPrimvars;
    bool overlayAll;    // Completely replace prims, including topology. 
                     // For point instancers, if overlayAll is set and 
                     // prototypes are specified, replace the prototypes.

    bool writeStaticGeo;
    bool writeStaticTopology;
    bool writeStaticPrimvars;

    // Filter specifying what primvars to write for each prim.
    const GusdGT_AttrFilter& attributeFilter;

    // Name of attribute that specifies usd prim path to write prims to.
    // a prim. 
    std::string primPathAttribute;
    
    // Path to a sop or obj node that contains all the prototypes so we can
    // write a complete and static relationship array. Will be overridden by
    // attributes if they exist.
    std::string usdPrototypesPath;

    // Identifier (and possibly path to a primitive) to both create an entry
    // in a point instancer's relationship array and mark which prototype to use
    // for a point. Will be overridden by attributes if they exist.
    std::string usdInstancePath;

    // Offset value to set in a Layer Offset of a USD reference. For retiming
    // references.
    double usdTimeOffset;

    // Scale value to set in a Layer Offset of a USD reference. For retiming
    // references.
    double usdTimeScale;

    // When we write a USD packed prim to a USD file, we write a USD reference.
    // If the prim path attribute in the USD packed prim contains a variant
    // selection, write that with the reference.
    bool authorVariantSelections;

    // Purpose (render, proxy or guide) to tag prims with.
    TfToken purpose;

    // Whether to make references to a USD prims instanceable.
    bool makeRefsInstanceable;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __GUSD_CONTEXT_H__
