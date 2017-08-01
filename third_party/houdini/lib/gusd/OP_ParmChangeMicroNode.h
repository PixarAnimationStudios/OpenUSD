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
   \brief
*/
#ifndef _GUSD_OP_PARMCHANGEMICRONODE_H_
#define _GUSD_OP_PARMCHANGEMICRONODE_H_

#include <pxr/pxr.h>

#include "gusd/api.h"

#include <DEP/DEP_TimedMicroNode.h>
#include <OP/OP_Parameters.h>
#include <SYS/SYS_SequentialThreadIndex.h>

PXR_NAMESPACE_OPEN_SCOPE

/** Micro node for tracking changes to a set of parameters.
    This is similar to OP_ParmCache, except that this tracks changes to the
    *resolved* values of parameters, rather than their dirty states.

    Only persistent parameters can be tracked. Spare and dynamic parameters
    (I.e., instances of multi parms) cannot be tracked through this class.

    This class is not thread safe!*/
class GusdOP_ParmChangeMicroNode : public DEP_TimedMicroNode
{
public:
    explicit GusdOP_ParmChangeMicroNode(OP_Parameters& node)
        : DEP_TimedMicroNode(), _node(node), _parmsAdded(false) {}

    GUSD_API
    virtual ~GusdOP_ParmChangeMicroNode();

    /** Begin tracking the given parm.
        If @a vectorIdx is less than zero, all elements of the parm
        tuple are tracked.*/
    GUSD_API
    void            addParm(int parmIdx, int vecIdx=-1);

    /** Update the resolved parm values.
        @return true if any resolved values changed.*/
    GUSD_API
    bool            updateVals(fpreal t, int thread=SYSgetSTID());

    bool            updateIfNeeded(fpreal t, int thread=SYSgetSTID())
                    { return requiresUpdate(t) && updateVals(t, thread); }

    virtual void    update(fpreal t)    { updateVals(t); }

    /** Clear our inputs.
        This is overridden to disallow clearing of explicit inputs,
        which are meant to persist on this micro node.*/
    virtual void    clearInputs()       { setTimeDependent(false); }

    struct _ParmCache;

private:
    OP_Parameters&          _node;

    UT_Array<_ParmCache*>   _cachedVals;
    bool                    _parmsAdded;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif /*_GUSD_OP_PARMCHANGEMICRONODE_H_*/
