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
#include "gusd/OP_ParmChangeMicroNode.h"

#include "gusd/UT_Assert.h"

#include <PRM/PRM_Parm.h>
#include <PRM/PRM_ParmList.h>

PXR_NAMESPACE_OPEN_SCOPE

struct GusdOP_ParmChangeMicroNode::_ParmCache
{
    _ParmCache(const PRM_Parm& parm, int pi, int vi)
        : _pi(pi), _vi(vi)
        {
            UT_ASSERT(vi < parm.getVectorSize());
        }

    virtual ~_ParmCache() {}

    virtual bool    Update(OP_Parameters& node, fpreal t, int thread) = 0;
    
protected:
    const int   _pi, _vi;
};


namespace {


struct _FloatEval
{
    void    operator()(fpreal& val, OP_Parameters& node,    
                       int pi, int vi, fpreal t, int thread) const
            { val = node.evalFloatT(pi, vi, t, thread); }

    void    operator()(fpreal64* vals, OP_Parameters& node,
                       int pi, fpreal t, int thread) const
            {
                UT_ASSERT_P(vals);
                node.evalFloatsT(pi, vals, t, thread);
            }
};


struct _IntEval
{
    void    operator()(int& val, OP_Parameters& node,
                       int pi, int vi, fpreal t, int thread) const
            { val = node.evalIntT(pi, vi, t, thread); }
    void    operator()(int* vals, OP_Parameters& node,
                       int pi, fpreal t, int thread) const
            {
                UT_ASSERT_P(vals);
                const int vecSize = node.getParm(pi).getVectorSize();
                for(int i = 0; i < vecSize; ++i)
                    vals[i] = node.evalIntT(pi, i, t, thread);
            }
};


struct _StrEval
{
    void    operator()(UT_String& val, OP_Parameters& node,
                       int pi, int vi, fpreal t, int thread) const
            { node.evalStringT(val, pi, vi, t, thread); }

    void    operator()(UT_String* vals, OP_Parameters& node,
                       int pi, fpreal t, int thread) const
            {
                UT_ASSERT_P(vals);
                const int vecSize = node.getParm(pi).getVectorSize();
                for(int i = 0; i < vecSize; ++i)
                    node.evalStringT(vals[i], pi, i, t, thread);
            }
};


template <class T, class EVALFN>
struct _ParmCacheSingleT : public GusdOP_ParmChangeMicroNode::_ParmCache
{
    _ParmCacheSingleT(const PRM_Parm& parm, int pi, int vi)
        : _ParmCache(parm, pi, vi) {}

    virtual bool    Update(OP_Parameters& node, fpreal t, int thread)
                    {
                        T tmp;
                        EVALFN()(tmp, node, _pi, _vi, t, thread);
                        if(tmp != _val) {
                            SetVal(tmp);
                            return true;
                        }
                        return false;
                    }

    void            SetVal(const T& val)    { _val = val; }
private:
    T _val;
};


/* Specialize setter for strings.*/
template <>
void
_ParmCacheSingleT<UT_String,_StrEval>::SetVal(const UT_String& src)
{
    _val.harden(src);
}


template <class T, class EVALFN>
struct _ParmCacheMultiT : public GusdOP_ParmChangeMicroNode::_ParmCache
{
    _ParmCacheMultiT(const PRM_Parm& parm, int pi, int vi)
        : _ParmCache(parm, pi, vi)
        {
            const int vecSize = parm.getVectorSize();
            _vals.setSize(vecSize);
            _tmpVals.setSize(vecSize);
        }

    virtual bool    Update(OP_Parameters& node, fpreal t, int thread)
                    {
                        EVALFN()(_tmpVals.array(), node, _pi, t, thread);
                        if(_tmpVals != _vals) {
                            SetVals(_tmpVals);
                            return true;
                        }
                        return false;
                    }
    void            SetVals(const UT_Array<T>& vals)    { _vals = vals; }
private:
    UT_Array<T> _vals;
    UT_Array<T> _tmpVals; /*! Temp value buffer.
                              Used to avoid allocation when updating.*/
};


/* Specialize setter for strings.*/
template <>
void
_ParmCacheMultiT<UT_String,_StrEval>::SetVals(const UT_Array<UT_String>& vals)
{
    _vals = vals;
    for(exint i = 0; i < _vals.size(); ++i)
        _vals(i).harden();
}


} /*namespace*/


GusdOP_ParmChangeMicroNode::~GusdOP_ParmChangeMicroNode()
{
    for(exint i = 0; i < _cachedVals.size(); ++i)
        delete GusdUTverify_ptr(_cachedVals(i));
}


void
GusdOP_ParmChangeMicroNode::addParm(int parmIdx, int vecIdx)
{
    UT_ASSERT(parmIdx >= 0 && parmIdx <= _node.getNumParms());

    const PRM_Parm& parm = _node.getParm(parmIdx);
    const int vecSize = parm.getVectorSize();

    UT_ASSERT(vecSize > 0);

    if(vecSize == 1)
        vecIdx = 0;

    if(parm.getType().isFloatType())
    {
        if(vecIdx >= 0)
            _cachedVals.append(new _ParmCacheSingleT<fpreal,_FloatEval>(
                                   parm, parmIdx, vecIdx));
        else
            _cachedVals.append(new _ParmCacheMultiT<fpreal64,_FloatEval>(
                                   parm, parmIdx, vecIdx));
    }
    else if(parm.getType().isOrdinalType())
    {
        if(vecIdx >= 0)
            _cachedVals.append(new _ParmCacheSingleT<int,_IntEval>(
                                   parm, parmIdx, vecIdx));
        else
            _cachedVals.append(new _ParmCacheMultiT<int,_IntEval>(
                                   parm, parmIdx, vecIdx));
    }
    else if(parm.getType().isStringType())
    {
        if(vecIdx >= 0)
            _cachedVals.append(new _ParmCacheSingleT<UT_String,_StrEval>(
                                   parm, parmIdx, vecIdx));
        else
            _cachedVals.append(new _ParmCacheMultiT<UT_String,_StrEval>(
                                   parm, parmIdx, vecIdx));
    } else {
        /* nothing to track.*/
        return;
    }
    PRM_ParmList* parms = GusdUTverify_ptr(_node.getParmList());
    if(vecIdx < 0) {
        for(int i = 0; i < vecSize; ++i)
            addExplicitInput(parms->parmMicroNode(parmIdx, i));
    } else {
        addExplicitInput(parms->parmMicroNode(parmIdx, vecIdx));
    }
    _parmsAdded = true;
}


bool
GusdOP_ParmChangeMicroNode::updateVals(fpreal t, int thread)
{
    int changed = _parmsAdded;
    _parmsAdded = false;

    for(exint i = 0; i < _cachedVals.size(); ++i)
        changed += GusdUTverify_ptr(_cachedVals(i))->Update(_node, t, thread);

    DEP_TimedMicroNode::update(t);
    return changed;
}

PXR_NAMESPACE_CLOSE_SCOPE
