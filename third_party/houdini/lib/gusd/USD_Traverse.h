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
   \brief Methods for USD scene traversal.
*/
#ifndef _GUSD_USD_TRAVERSE_H_
#define _GUSD_USD_TRAVERSE_H_


#include <PRM/PRM_Name.h>
#include <UT/UT_Array.h>
#include <UT/UT_Error.h>
#include <UT/UT_NonCopyable.h>
#include <UT/UT_String.h>
#include <UT/UT_StringMap.h>

#include "gusd/purpose.h"
#include "gusd/USD_Utils.h"

#include <pxr/pxr.h>
#include "pxr/usd/usd/prim.h"

class OP_Parameters;
class PRM_Template;

PXR_NAMESPACE_OPEN_SCOPE

/** Base class for custom stage traversal.
    
    To register traversals, define a static GusdUSD_TraverseType singleton
    that takes an instance of the traverse object. */
class GusdUSD_Traverse
{
public:
    struct Opts;
    typedef std::pair<UsdPrim,exint>        PrimIndexPair;

    virtual ~GusdUSD_Traverse() {}

    virtual Opts*       CreateOpts() const      { return NULL; }

    /** Find prims beneath the given root.*/
        
    virtual bool        FindPrims(const UsdPrim& root,
                                  UsdTimeCode time,
                                  const GusdPurposeSet& purposes,
                                  UT_Array<UsdPrim>& prims,
                                  bool skipRoot=true,
                                  const Opts* opts=NULL) const = 0;

    /** Find prims beneath the given root prims.
        Note that the input array of prims may contain invalid prims.
        The returned @a prims array holds the new prims, and the index
        of their root prim from the @a roots array. The array is sorted
        by the index and the prim path.*/
    virtual bool        FindPrims(const UT_Array<UsdPrim>& roots,
                                  const GusdUSD_Utils::PrimTimeMap& timeMap,
                                  const UT_Array<GusdPurposeSet>& purposes,
                                  UT_Array<PrimIndexPair>& prims,
                                  bool skipRoot=true,
                                  const Opts* opts=NULL) const = 0;

    bool                FindPrims(const UT_Array<UsdPrim>& roots,
                                  const GusdUSD_Utils::PrimTimeMap& timeMap,
                                  const UT_Array<GusdPurposeSet>& purposes,
                                  UT_Array<UsdPrim>& prims,
                                  bool skipRoot=true,
                                  const Opts* opts=NULL) const;

    /** Base class that can be derived to provide
        configuration options to the traversal.*/
    struct Opts
    {
        Opts() { Reset(); }
        virtual ~Opts() {}

        /** Reset options back to defaults.*/
        virtual void    Reset() {}

        virtual bool    Configure(OP_Parameters& parms, fpreal t) = 0;
    };

};


class GusdUSD_TraverseType
{
public:
    GusdUSD_TraverseType(const GusdUSD_Traverse* traversal,
                         const char* name,
                         const char* label,
                         const PRM_Template* templates=NULL,
                         const char* help=NULL);

    const PRM_Name&         GetName() const         { return _name; }
    const PRM_Template*     GetTemplates() const    { return _templates; }
    const char*             GetHelp() const         { return _help; }

    const GusdUSD_Traverse& operator*() const       { return *_traversal; }
    const GusdUSD_Traverse* operator->() const      { return _traversal; }

private:
    const GusdUSD_Traverse* const   _traversal;
    const PRM_Name                  _name;
    const PRM_Template* const       _templates;
    const UT_String                 _help;
};


/** Helper to provide control over traversal through children.*/
struct GusdUSD_TraverseControl
{
    GusdUSD_TraverseControl() : _visitChildren(true) {}

    bool    GetVisitChildren() const    { return _visitChildren; }
    void    SetVisitChildren(bool tf)   { _visitChildren = tf; }
    void    PruneChildren()             { SetVisitChildren(false); }

private:
    bool               _visitChildren;
};


/** Table for registering custom stage traversals. */
class GusdUSD_TraverseTable : UT_NonCopyable
{
public:
    typedef UT_StringMap<const GusdUSD_TraverseType*>   Map;
    typedef Map::const_iterator                         const_iterator;
    typedef Map::iterator                               iterator;

    static GusdUSD_TraverseTable&   GetInstance();

    void                            Register(const GusdUSD_TraverseType* type);

    const GusdUSD_TraverseType*     Find(const char* name) const;
    
    const GusdUSD_Traverse*         FindTraversal(const char* name) const;

    const char*                     GetDefault() const  
                                    { return _default; }

    void                            SetDefault(const char* name)
                                    { _default.harden(name); }
    
    iterator                        begin()         { return _map.begin(); }
    const_iterator                  begin() const   { return _map.begin(); }

    iterator                        end()           { return _map.end(); }
    const_iterator                  end() const     { return _map.end(); }

private:
    Map         _map;
    UT_String   _default;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif /*_GUSD_USD_TRAVERSE_H_*/
