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
#ifndef _GUSD_USD_PROXY_H_
#define _GUSD_USD_PROXY_H_


#include "gusd/api.h"
#include "gusd/UT_Error.h"
#include "gusd/USD_Holder.h"
#include "gusd/USD_Utils.h"

#include <DEP/DEP_MicroNode.h>
#include <UT/UT_ErrorManager.h>
#include <UT/UT_IntrusivePtr.h>
#include <UT/UT_NonCopyable.h>
#include <UT/UT_WorkBuffer.h>

#include <pxr/pxr.h>
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/usd/ar/resolverContext.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include <boost/unordered_set.hpp>

PXR_NAMESPACE_OPEN_SCOPE

class GusdUSD_Cache;
class GusdUSD_StageCache;


class GusdUSD_StageProxy;
typedef UT_IntrusivePtr<GusdUSD_StageProxy> GusdUSD_StageProxyHandle;


/** Proxy for a cached USD stage.
    This manages safe, deferred access to a stage, including support
    for deferred prim-loading.

    A proxy holds a reference to a key that identifies everything needed
    to be able to open a stage, and contains structures for tracking what
    is loaded on the stage. Once a stage has been loaded, a proxy also
    holds a reference-counted reader-writer lock for the stage, as well
    as a reference to the stage itself.

    @section Thread-Safety
    
    Deferred primitive loading and some additional operations like variant
    selections require stage mutation, which is not thread-safe.
    No reads from a stage should ever be made while another thread is in the
    process of mutating the stage.

    To make this thread-safe, proxies add a mutex to to control access
    to stages through read-writer locks.

    All callers *MUST* acquire locks when accessing the stage in any way.
    Tasks as simple as schema type-checking, or UsdAttribute::GetTypeName()
    involve stage reads, and so may be subject to random failures or crashes
    if called at a time when the stage is being mutated. It is not always
    obvious what operations involve reads, and which do not, so just to be
    clear: *Locks should be acquired for absolutely any operation
    that touches a stage or one of its primitives.

    Note that this extends to invidiual Sdf layers: If an Sdf layer is being
    mutated, it is not safe to read from that layer, or from any stage that
    references that layer.
    Proxies are designed for cache-consumption, and there is an expectation
    that shared Sdf layers are *never* edited directly. The only layers we
    expect to mutate are session layers. Session layers must not be shared
    among different proxies.
    
    @section Stage Loading/Reloading.
    
    Stages held by a proxy are loaded on demand. This happens whenenever
    an Accessor is bound to the proxy -- which also establishes a read
    lock on the proxy.

    The stage held by a proxy may also be unloaded. When this occurs,
    the GusdUSD_StageCache that owns the proxy is informed, so that any caches
    that are holding data for the stage can be flush all corresponding entries.
    
    Some code paths need to hold onto a stage, and require that that stage
    not be invalidated while they hold a reference to it. This can be done by
    holding onto the proxy's GusdUSD_StageHolder, which wraps around the actual
    stage. Note that callers must never retain a reference to the stage itself,
    as the stage should never be accessed without acquiring a lock.
    If a stage is unloaded through the proxy, but is still being held elsewhere
    through a GusdUSD_StageHolder, the previous stage will stay resident
    until all of those older references are dropped. The proxy, however, will
    still be given a new stage.

    Stages may be safely reloaded through stage proxies. Keep in mind, though,
    that reloading may cause some primitives that previously existed to
    be deleted, since they may no longer be defined in one of the reloaded
    layers. There is no way of preventing this, short of requiring reloading
    to create a new stage (impractical). Hence, whereas there is a way for
    caches to retain a protected reference to a stage, no such protected
    reference exists for primitives.

    @section Deferred Primitive Loading

    The stage held by a proxy may be loaded without loading all payloads.
    When binding an Accessor, the caller should pass in the set of primitives
    that they intend to access, in which case they will be loaded if they
    are not already.
    
    Deferred loading comes at a cost of possibly high contention when performing
    initial prim loading.

    At the moment, unloading of individual primitives is not supported
    (it would require more sophisticated load tracking). */
class GusdUSD_StageProxy : public UT_IntrusiveRefCounter<GusdUSD_StageProxy>,
                           UT_NonCopyable
{
public:
    class Key;
    typedef UT_IntrusivePtr<Key>            KeyHandle;
    typedef UT_IntrusivePtr<const Key>      KeyConstHandle;
    typedef GusdUSD_StageLock::ScopedLock   ScopedLock;

    typedef boost::unordered_set<SdfPath>   UnorderedPathSet;

    GUSD_API
    GusdUSD_StageProxy(GusdUSD_StageCache& cache, const KeyHandle& key);
    
    GUSD_API
    ~GusdUSD_StageProxy();

    const KeyConstHandle&           GetKey() const  { return _key; }

    const GusdUSD_StageLockHandle&  GetLock() const { return _lock; }

    /** Access a micro node for the proxy.
        The micro node is dirtied for stage reloads.*/
    DEP_MicroNode&                  GetMicroNode()  { return _microNode; }

    /** Mark the stage dirty.
        The stage will be reloaded the next time it is accessed.*/
    void                            MarkDirty() { _microNode.setDirty(true); }

    /** Unload the stage held by this proxy.
        Other caches may still hold onto the stage via a GusdUSD_StageHolder,
        or a GusdUSD_PrimHolder. */
    void                            Unload();

    /** If the file containing th root layer of a stage has changed, dirty the cache */
    GUSD_API
    void                            MarkDirtyIfFileChanged();

    class MultiAccessor;


    /** Helper for acquiring read-only access to the stage held by a proxy.
        A proxy's stage is loaded when an accessor is bound.
        
        Any prims that are going to be accessed should be in as @a pathsToLoad
        in Bind(). This will ensure they are loaded while the stage is loading.*/
    class Accessor  
    {
    public:
        Accessor() {}

        Accessor(const GusdUSD_StageProxyHandle& proxy,
                 UsdStage::InitialLoadSet loadSet,
                 const SdfPath& pathToLoad=SdfPath(),
                 GusdUT_ErrorContext* err=NULL)
            { Bind(proxy, loadSet, pathToLoad, err); }

        Accessor(const GusdUSD_StageProxyHandle& proxy,
                 UsdStage::InitialLoadSet loadSet,
                 const UnorderedPathSet* pathsToLoad=NULL,
                 GusdUT_ErrorContext* err=NULL)
            { Bind(proxy, loadSet, pathsToLoad, err); }

        explicit                operator bool() const   { return (bool)_proxy; }

        bool                    Bind(const GusdUSD_StageProxyHandle& proxy,
                                     UsdStage::InitialLoadSet loadSet,
                                     const SdfPath& pathToLoad=SdfPath(),
                                     GusdUT_ErrorContext* err=NULL);

        bool                    Bind(const GusdUSD_StageProxyHandle& proxy,
                                     UsdStage::InitialLoadSet loadSet,
                                     const UnorderedPathSet* pathsToLoad=NULL,
                                     GusdUT_ErrorContext* err=NULL);

        void                    Release();

        /* Get the loaded stage.
           note: If the accessor is successfully bound, the stage returned
           will always be non-null. */
        const UsdStageRefPtr&   GetStage() const    { return _proxy->_stage; }

        const UsdStageRefPtr&   operator->() const  { return GetStage(); }

        const KeyConstHandle&   GetKey() const      { return _proxy->GetKey(); }

        double                  GetStartTimeCode() const
                                { return _proxy->_stageData.startTimeCode; }

        double                  GetEndTimeCode() const
                                { return _proxy->_stageData.endTimeCode; }

        UsdTimeCode             ClampTime(UsdTimeCode time) const;

        /** Get a prim by path.
            This is a convenience method for error reporting when
            prims are not found. */
        GUSD_API
        UsdPrim                 GetPrimAtPath(
                                    const SdfPath& path,
                                    GusdUT_ErrorContext* err=NULL) const;

        /** Get a prim schema type by path.
            This will report errors if the prim is not found, or
            if the schema type is not matched correctly.*/
        template <class T>
        T                       GetPrimSchemaAtPath(
                                    const SdfPath& path,
                                    GusdUT_ErrorContext* err=NULL) const;

        /** Get a holder for the prim at the given path.
            The holder may be locked on its own, without requiring
            a reference back to the proxy.*/
        GusdUSD_PrimHolder      GetPrimHolderAtPath(
                                    const SdfPath& path,
                                    GusdUT_ErrorContext* err=NULL) const;

        /** Get a holder for a prim schema at the given path,
            reporting errors for unmatched schema types.*/
        template <class T>
        GusdUSD_HolderT<T>      GetPrimSchemaHolderAtPath(
                                    const SdfPath& path,
                                    GusdUT_ErrorContext* err=NULL) const;

        GusdUSD_StageHolder                 GetStageHolder() const;

        const GusdUSD_StageProxyHandle&     GetProxy() const
                                            { return _proxy; }
        const GusdUSD_StageLockHandle&      GetLock() const
                                            { return _proxy->_lock; }

    private:

        friend class MultiAccessor;

        /** Load a set of prim paths.
            This must only ever be called by a single thread.*/
        void                    _Load(const SdfPathSet& paths);

    private:

        /* XXX: Consider making this a raw pointer so we don't have
                to touch reference counts when acquiring access to a proxy.*/
        GusdUSD_StageProxyHandle    _proxy;
        ScopedLock                  _lock;
    };


    /** Accessor for reading from prims across multiple stages. */
    class MultiAccessor
    {
    public:
        MultiAccessor() : _accessors(NULL), _size(0), _numAccessors(0) {}
        ~MultiAccessor() { Release(); }

        GUSD_API
        void    Release();
            
        /** Bind the accessor for the given @a proxies and @a paths.
            If successful, the @a prims array will have an entry for
            each prim.
            If an entry in the @a proxies array is null, or the corresponding
            path from @a paths is empty, the resulting prim will be an
            invalid prim, and no errors will be thrown.*/
        GUSD_API
        bool    Bind(const UT_Array<GusdUSD_StageProxyHandle>& proxies,
                     const UT_Array<SdfPath>& paths,
                     UT_Array<UsdPrim>& prims,
                     UsdStage::InitialLoadSet loadSet,
                     GusdUT_ErrorContext* err=NULL);
        /** @} */

        GUSD_API
        bool    ClampTimes(UT_Array<UsdTimeCode>& times) const;

        GUSD_API
        bool    ClampTimes(GusdUSD_Utils::PrimTimeMap& timeMap) const;

        exint           size() const    { return _size; }

        GUSD_API
        const Accessor* operator()(exint i) const;

        GUSD_API
        Accessor*       operator()(exint i);

    private:
        bool    _Load(const UT_Array<SdfPath>& paths);

        bool    _GetPrims(const UT_Array<SdfPath>& primPaths,
                          UT_Array<UsdPrim>& prims,
                          GusdUT_ErrorContext* err=NULL);
        
    private:
        /* Accessors aren't copyable, which prevents use of
           most array wrappers. Use raw arrays.*/
        Accessor*       _accessors;
        exint           _size, _numAccessors;
        UT_Array<exint> _indexMap;
    };

    
    /** Key for identifying a stage.

        XXX: Since this for use as a cache key, TfToken has been used for
        paths rather than std::string to improve lookup times.*/
    class Key : public UT_IntrusiveRefCounter<Key>
    {
    public:
        static KeyHandle    New(const TfToken& path,
                                const SdfLayerRefPtr& sessionLyr,
                                const ArResolverContext& resolverCtx)
                            { return KeyHandle(
                                    new Key(path, sessionLyr, resolverCtx)); }

        static KeyHandle    New(const ArResolverContext& resolverCtx)
                            { return KeyHandle(new Key(resolverCtx)); }

        Key(const TfToken& path,
            const SdfLayerRefPtr& sessionLyr,
            const ArResolverContext& resolverCtx)
            : path(path), sessionLyr(sessionLyr), resolverCtx(resolverCtx) {}

        Key(const ArResolverContext& resolverCtx)
            : resolverCtx(resolverCtx) {}

        const TfToken& GetPath() const { return path; }

        bool    operator==(const Key& o) const
                {
                    return path == o.path &&
                           sessionLyr == o.sessionLyr &&
                           resolverCtx == o.resolverCtx;
                }

    public:
        TfToken                 path;
        SdfLayerRefPtr          sessionLyr;
        const ArResolverContext resolverCtx;
    };

private:

    /** Load the stage.
        The given @a lock should already be acquired as a read lock.*/
    bool    _Load(ScopedLock& lock,
                  UsdStage::InitialLoadSet loadSet,
                  const UnorderedPathSet* pathsToLoad,
                  GusdUT_ErrorContext* err=NULL);

    /** Reload the stage and update caches.*/
    void    _Reload(const UsdStageRefPtr& stage);

    /** Copy the errors cached by this proxy into the given context.*/
    void    _CopyErrors(GusdUT_ErrorContext& dst);

    /** Initialize the proxy for the given initial load set.
        This will cause _PrimLoader to be instantiated if necessary,
        or removed if not in use.*/
    void    _InitLoadSet(UsdStage::InitialLoadSet loadSet);

    /* Set of data cached for a stage.*/
    struct _StageData
    {
        _StageData();

        void    Update(const UsdStageRefPtr& stage);

        double  startTimeCode, endTimeCode;
    };

private:

    const KeyConstHandle        _key;

    GusdUSD_StageCache&         _cache;     /*! Cache that owns this proxy. */
    GusdUSD_StageLockHandle     _lock;
    UsdStageRefPtr              _stage;
    UT_ErrorManager             _errors;    /*! Stashed copy of load errors.*/
    DEP_MicroNode               _microNode; /*! DEP node that's dirtied when
                                                stage loading is required.*/
    UsdStage::InitialLoadSet    _loadSet;
    struct _PrimLoader;
    /** Helper for managing deferred loading of prims.
        The prim loader is only valid when the stage has been been only
        partially loaded (I.e., UsdStage::LoadNone) */
    _PrimLoader*                _primLoader;

    _StageData                  _stageData; /*! Cached data for stage.*/

    /* Store the real path and the mod time of the stage when it is loaded. 
       This is used to know when a file has changed. */
    TfToken                     _realPath;  
    time_t                      _mtime;  
};


inline size_t
hash_value(const GusdUSD_StageProxy::Key& o)
{
    size_t hash = hash_value(o.resolverCtx);
    boost::hash_combine(hash, o.path);
    boost::hash_combine(hash, o.sessionLyr);
    return hash;
}


inline void
GusdUSD_StageProxy::Accessor::Release()
{
    if(_proxy) {
        _lock.Release();
        _proxy.reset();
    }
}


inline UsdTimeCode
GusdUSD_StageProxy::Accessor::ClampTime(UsdTimeCode time) const
{
    if(!time.IsDefault())
        return UsdTimeCode(SYSclamp(time.GetValue(),
                                    GetStartTimeCode(),
                                    GetEndTimeCode()));
    return time;
}


inline GusdUSD_PrimHolder
GusdUSD_StageProxy::Accessor::GetPrimHolderAtPath(
    const SdfPath& path, GusdUT_ErrorContext* err) const
{
    return GusdUSD_PrimHolder(GetPrimAtPath(path, err), GetLock());
}


template <class T>
T
GusdUSD_StageProxy::Accessor::GetPrimSchemaAtPath(
    const SdfPath& path, GusdUT_ErrorContext* err) const
{
    if(auto prim = GetPrimAtPath(path, err)) {
        T schemaObj(prim);
        if(schemaObj) {
            return schemaObj;
        } else if(err) {
            auto type = TfType::Find<T>();
            UT_ASSERT_P(!type.IsUnknown());
            UT_WorkBuffer buf;
            buf.sprintf("Prim <%s> is not a '%s'",
                        path.GetText(), type.GetTypeName().c_str());
            err->AddError(buf.buffer());
        }
    }
    return T();
}


template <class T>
GusdUSD_HolderT<T>
GusdUSD_StageProxy::Accessor::GetPrimSchemaHolderAtPath(
    const SdfPath& path, GusdUT_ErrorContext* err) const
{
    return GusdUSD_HolderT<T>(GetPrimSchemaAtPath<T>(path, err), GetLock());
}


inline GusdUSD_StageHolder
GusdUSD_StageProxy::Accessor::GetStageHolder() const
{
    return GusdUSD_StageHolder(GetStage(), GetLock());
}


inline const GusdUSD_StageProxy::Accessor*
GusdUSD_StageProxy::MultiAccessor::operator()(exint i) const
{
    UT_ASSERT_P(i >= 0 && i < _size);
    int idx = _indexMap(i);
    UT_ASSERT_P(idx < _size);
    return idx >= 0 ? &_accessors[idx] : NULL;
}


inline GusdUSD_StageProxy::Accessor*
GusdUSD_StageProxy::MultiAccessor::operator()(exint i)
{
    UT_ASSERT_P(i >= 0 && i < _size);
    int idx = _indexMap(i);
    UT_ASSERT_P(idx < _size);
    return idx >= 0 ? &_accessors[idx] : NULL;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif /*_GUSD_USD_PROXY_H_*/
