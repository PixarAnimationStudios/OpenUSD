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
   \brief Standard traversal algos.
*/
#ifndef _GUSD_USD_STDTRAVERSE_H_
#define _GUSD_USD_STDTRAVERSE_H_

#include <pxr/pxr.h>

#include "gusd/USD_Traverse.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace GusdUSD_StdTraverse
{


/** Core prim traversal.
    These traverse only default-imageable prims. This does not account for
    visibility which, for performance reasons, is expected to occur as
    a post-traversal operation.
    
    None of the traversals return nested matches.
    @{ */
const GusdUSD_Traverse& GetComponentTraversal();

const GusdUSD_Traverse& GetComponentAndBoundableTraversal();

const GusdUSD_Traverse& GetAssemblyTraversal();

const GusdUSD_Traverse& GetModelTraversal();

const GusdUSD_Traverse& GetGroupTraversal();

const GusdUSD_Traverse& GetBoundableTraversal();

const GusdUSD_Traverse& GetGprimTraversal();

const GusdUSD_Traverse& GetMeshTraversal();
/** @} */

/** Recursive model traversal, returning all nested models.
    This is primarily provided for UI menus.*/
const GusdUSD_Traverse& GetRecursiveModelTraversal();


/** Register core traversals.*/
void                    Register();


} /*namespace GusdUSD_StdTraverse*/

PXR_NAMESPACE_CLOSE_SCOPE

#endif /*_GUSD_USD_STDTRAVERSE_H_*/
