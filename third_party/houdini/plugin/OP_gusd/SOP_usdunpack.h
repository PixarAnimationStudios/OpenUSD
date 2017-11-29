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
#ifndef __GUSD_SOP_USDUNPACK_H__
#define __GUSD_SOP_USDUNPACK_H__

#include <PRM/PRM_Template.h>
#include <SOP/SOP_Node.h>

#include "gusd/defaultArray.h"
#include "gusd/purpose.h"
#include "gusd/USD_Traverse.h"

#include <pxr/pxr.h>
#include "pxr/usd/usd/prim.h"

PXR_NAMESPACE_OPEN_SCOPE

class GusdUSD_Traverse;
class GusdUT_ErrorContext;

class GusdSOP_usdunpack : public SOP_Node
{
public:
    static OP_Node*         Create(OP_Network* net,
                                   const char* name,
                                   OP_Operator* op);

    void                    UpdateTraversalParms();
    
protected:
    GusdSOP_usdunpack(OP_Network* net, const char* name, OP_Operator* op);
    
    virtual ~GusdSOP_usdunpack() {}

    virtual OP_ERROR    cookMySop(OP_Context& ctx);

    virtual OP_ERROR    cookInputGroups(OP_Context& ctx, int alone);

    OP_ERROR            _Cook(OP_Context& ctx);

    bool _Traverse(const UT_String& traversal,
                   const fpreal time,
                   const UT_Array<UsdPrim>& prims,
                   const GusdDefaultArray<UsdTimeCode>& times,
                   const GusdDefaultArray<GusdPurposeSet>& purposes,
                   bool skipRoot,
                   UT_Array<GusdUSD_Traverse::PrimIndexPair>& traversed,
                   GusdUT_ErrorContext& err);


    /** Add micro nodes of all traversal parms as dependencies
        to this node's data micro node.*/
    void                _AddTraversalParmDependencies();

    virtual void        finishedLoadingNetwork(bool isChildCall);

private:
    UT_Array<PRM_Template>  _templates;
    PRM_Default             _tabs[2];
    const GA_Group*         _group;

public:
    static void         Register(OP_OperatorTable* table);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __GUSD_SOP_USDUNPACK_H__
