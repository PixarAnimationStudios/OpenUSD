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

#include "writeCtrlFlags.h"
#include "GU_USD.h"

#include <GT/GT_Primitive.h>
#include <GT/GT_GEOPrimPacked.h>
#include <GT/GT_AttributeList.h>

using std::cerr;
using std::endl;

PXR_NAMESPACE_OPEN_SCOPE

void
GusdWriteCtrlFlags::update( const GT_PrimitiveHandle &sourcePrim )
{
    overPoints =     getBoolAttr( sourcePrim, GUSD_OVERPOINTS_ATTR, overPoints );
    overTransforms = getBoolAttr( sourcePrim, GUSD_OVERTRANSFORMS_ATTR, overTransforms );
    overPrimvars =   getBoolAttr( sourcePrim, GUSD_OVERPRIMVARS_ATTR, overPrimvars );
    overAll =        getBoolAttr( sourcePrim, GUSD_OVERALL_ATTR, overAll );
}

/* static */
bool 
GusdWriteCtrlFlags::getBoolAttr( 
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

PXR_NAMESPACE_CLOSE_SCOPE
