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

#include "context.h"
#include "GU_USD.h"

#include <GT/GT_Primitive.h>
#include <GT/GT_GEOPrimPacked.h>
#include <GT/GT_AttributeList.h>

namespace {
bool getBoolAttr( 
        const GT_PrimitiveHandle& prim,
        const char *attrName,
        bool defaultValue );
}

bool
GusdContext::getOverTransforms( const GT_PrimitiveHandle &sourcePrim ) const
{
    return getBoolAttr( sourcePrim, GUSD_OVERTRANSFORMS_ATTR,
                        overlayGeo && overlayTransforms );
}

bool
GusdContext::getOverPoints( const GT_PrimitiveHandle &sourcePrim ) const
{
    return getBoolAttr( sourcePrim, GUSD_OVERPOINTS_ATTR,
                        overlayGeo && overlayPoints );
}

bool
GusdContext::getOverPrimvars( const GT_PrimitiveHandle &sourcePrim ) const
{
    return getBoolAttr( sourcePrim, GUSD_OVERPRIMVARS_ATTR,
                        overlayGeo && overlayPrimvars );
}

bool
GusdContext::getOverAll( const GT_PrimitiveHandle &sourcePrim ) const
{
    return getBoolAttr( sourcePrim, GUSD_OVERALL_ATTR,
                        overlayGeo && overlayAll );
}

bool
GusdContext::getOverGeo( const GT_PrimitiveHandle &sourcePrim ) const
{
    return getOverTransforms( sourcePrim ) || getOverPoints( sourcePrim ) ||
           getOverPrimvars( sourcePrim ) || getOverAll( sourcePrim );
}

namespace {

bool 
getBoolAttr( 
    const GT_PrimitiveHandle& prim,
    const char *attrName,
    bool defaultValue ) 
{   
    if( prim ) {
        GT_DataArrayHandle data;
        if( prim->getPrimitiveType() == GT_GEO_PACKED ) {
            GT_AttributeListHandle instAttrs = 
                UTverify_cast<const GT_GEOPrimPacked*>(prim.get())->getInstanceAttributes();
            if( instAttrs ) {
                data = instAttrs->get( attrName );
            }    
        }
        if( !data ) {
            GT_Owner own;
            data = prim->findAttribute( attrName, own, 0 );        
        }
        if( data ) {
            return bool( data->getI32(0) );
        }
    }
    return defaultValue;
}
}