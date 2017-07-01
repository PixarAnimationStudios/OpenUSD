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
#ifndef _GUSD_USD_CUSTOMTRAVERSE_H_
#define _GUSD_USD_CUSTOMTRAVERSE_H_

#include <pxr/pxr.h>

#include "gusd/USD_Traverse.h"

#include <UT/UT_StringMMPattern.h>

PXR_NAMESPACE_OPEN_SCOPE

/** A traversal implementation offering users full configuration
    over many aspects of traversal.*/
class GusdUSD_CustomTraverse : public GusdUSD_Traverse
{
public:
    enum TriState
    {
        TRUE_STATE,
        FALSE_STATE,
        ANY_STATE
    };

    struct Opts : public GusdUSD_Traverse::Opts
    {
        Opts() : GusdUSD_Traverse::Opts() {}
        virtual ~Opts() {}

        virtual void            Reset();
        virtual bool            Configure(OP_Parameters& parms, fpreal t);

        /** Methods for matching components by wildcard pattern.
            Note that for all methods, an empty pattern is treated
            as equivalent to '*'. I.e., an empty pattern matches everything.
            @{ */
        bool                    SetKindsByPattern(const char* pattern,
                                                  bool caseSensitive=true,
                                                  std::string* err=NULL);
        
        bool                    SetPurposesByPattern(const char* pattern,
                                                     bool caseSensitive=true,
                                                     std::string* err=NULL);

        bool                    SetTypesByPattern(const char* pattern,
                                                  bool caseSensitive=true,
                                                  std::string* err=NULL);

        void                    SetNamePattern(const char* pattern,
                                               bool caseSensitive=true);

        void                    SetPathPattern(const char* pattern,
                                               bool caseSensitive=true);
        /** @} */

        /** Create a predicate matching all of the configurable
            options that refer to prim flags.*/
        Usd_PrimFlagsPredicate  MakePredicate() const;

        
        TriState            active, visible, imageable, defined, abstract,
                            model, group, instance, master, clips;
        bool                traverseMatched;
        UT_Array<TfToken>   purposes, kinds;
        UT_Array<TfType>    types;
        UT_StringMMPattern  namePattern, pathPattern;
    };

    virtual Opts*   CreateOpts() const  { return new Opts; }

    virtual bool    FindPrims(const UsdPrim& root,
                              UsdTimeCode time,
                              const GusdPurposeSet& purposes,
                              UT_Array<UsdPrim>& prims,
                              bool skipRoot=true,
                              const GusdUSD_Traverse::Opts* opts=NULL) const;
    
    virtual bool    FindPrims(const UT_Array<UsdPrim>& roots,
                              const GusdUSD_Utils::PrimTimeMap& timeMap,
                              const UT_Array<GusdPurposeSet>& purposes,
                              UT_Array<PrimIndexPair>& prims,
                              bool skipRoot=true,
                              const GusdUSD_Traverse::Opts* opts=NULL) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif /*_GUSD_USD_CUSTOMTRAVERSE_H_*/
