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
#ifndef _GUSD_USD_TRAVERSESIMPLE_H_
#define _GUSD_USD_TRAVERSESIMPLE_H_

#include <pxr/pxr.h>

#include "gusd/defaultArray.h"
#include "gusd/USD_Traverse.h"
#include "gusd/USD_ThreadedTraverse.h"


PXR_NAMESPACE_OPEN_SCOPE

/** Templated class for declaring simple, threaded traversals.
    See GusdUSD_ThreadedTraverse::VisiblePrimVisitorT for an
    example of the structure expected for visitors.*/
template <class Visitor>
class GusdUSD_TraverseSimpleT : public GusdUSD_Traverse
{
public:
    GusdUSD_TraverseSimpleT(const Visitor& visitor)
        : GusdUSD_Traverse(), _visitor(visitor) {}

    virtual ~GusdUSD_TraverseSimpleT() {}

    virtual bool    FindPrims(const UsdPrim& root,
                              UsdTimeCode time,
                              GusdPurposeSet purposes,
                              UT_Array<UsdPrim>& prims,
                              bool skipRoot=true,
                              const Opts* opts=NULL) const;

    virtual bool    FindPrims(const UT_Array<UsdPrim>& roots,
                              const GusdDefaultArray<UsdTimeCode>& times,
                              const GusdDefaultArray<GusdPurposeSet>& purposes,
                              UT_Array<PrimIndexPair>& prims,
                              bool skipRoot=true,
                              const Opts* opts=NULL) const;

private:
    const Visitor&  _visitor;
};


template <class Visitor>
bool
GusdUSD_TraverseSimpleT<Visitor>::FindPrims(const UsdPrim& root,
                                            UsdTimeCode time,
                                            GusdPurposeSet purposes,
                                            UT_Array<UsdPrim>& prims,
                                            bool skipRoot,
                                            const Opts* opts) const
{
    return GusdUSD_ThreadedTraverse::ParallelFindPrims(
        root, time, purposes, prims, _visitor, skipRoot);
}


template <class Visitor>
bool
GusdUSD_TraverseSimpleT<Visitor>::FindPrims(
    const UT_Array<UsdPrim>& roots,
    const GusdDefaultArray<UsdTimeCode>& times,
    const GusdDefaultArray<GusdPurposeSet>& purposes,
    UT_Array<PrimIndexPair>& prims,
    bool skipRoot,
    const Opts* opts) const
{
    return GusdUSD_ThreadedTraverse::ParallelFindPrims(
        roots, times, purposes, prims, _visitor, skipRoot);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif /*_GUSD_USD_TRAVERSESIMPLE_H_*/
