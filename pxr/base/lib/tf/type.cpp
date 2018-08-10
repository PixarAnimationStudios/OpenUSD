//
// Copyright 2016 Pixar
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

#include "pxr/pxr.h"

#include "pxr/base/tf/type.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/typeInfoMap.h"
#include "pxr/base/tf/typeNotice.h"

#ifdef PXR_PYTHON_SUPPORT_ENABLED
#include "pxr/base/tf/pyObjWrapper.h"
#include "pxr/base/tf/pyObjectFinder.h"
#include "pxr/base/tf/pyUtils.h"
#endif // PXR_PYTHON_SUPPORT_ENABLED

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>

#include <tbb/spin_rw_mutex.h>

#include <atomic>
#include <algorithm>
#include <iostream>
#include <map>
#include <vector>

#include <thread>

using std::map;
using std::pair;
using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

typedef vector<TfType> TypeVector;

using RWMutex = tbb::spin_rw_mutex;
using ScopedLock = tbb::spin_rw_mutex::scoped_lock;

TfType::FactoryBase::~FactoryBase()
{
}

#ifdef PXR_PYTHON_SUPPORT_ENABLED
TfType::PyPolymorphicBase::~PyPolymorphicBase()
{
}
#endif // PXR_PYTHON_SUPPORT_ENABLED

// Stored data for a TfType.
// A unique instance of _TypeInfo is allocated for every type declared.
//
struct TfType::_TypeInfo : boost::noncopyable
{
    typedef TfHashMap<string, TfType::_TypeInfo*, TfHash> NameToTypeMap;
    typedef TfHashMap<
        TfType::_TypeInfo*, vector<string>, TfHash> TypeToNamesMap;
    typedef TfHashMap<string, TfType, TfHash> DerivedByNameCache;

    // Unique TfType instance, for returning const references to.
    TfType canonicalTfType;
    
    // Unique type name.
    const string typeName;
    
    // Callback invoked to define this type when first required.
    TfType::DefinitionCallback definitionCallback;

    // C++ type_info.  NULL if no C++ type has been defined.
    std::atomic<std::type_info const *> typeInfo;

    // The size returned by sizeof(type).
    size_t sizeofType;

#ifdef PXR_PYTHON_SUPPORT_ENABLED
    // Python class handle.
    // We use handle<> rather than boost::python::object in case Python
    // has not yet been initialized.
    boost::python::handle<> pyClass;
#endif // PXR_PYTHON_SUPPORT_ENABLED

    // Direct base types.
    TypeVector baseTypes;

    // Direct derived types.
    TypeVector derivedTypes;

    // Factory.
    std::unique_ptr<TfType::FactoryBase> factory;

    // Map of derived type aliases to derived types.
    boost::optional<NameToTypeMap> aliasToDerivedTypeMap;
    // Reverse map of derived types to their aliases.
    boost::optional<TypeToNamesMap> derivedTypeToAliasesMap;

    // Map of functions for converting to other types.
    // This map is keyed by type_info and not TfType because the TfTypes
    // may not have been defined yet at the time we are adding castFuncs.
    // It is expected that the entries here will ultimately have matching
    // entries in our baseTypes, although that is not enforced.
    vector<pair<std::type_info const *, TfType::_CastFunction> > castFuncs;

    boost::scoped_ptr<DerivedByNameCache> derivedByNameCache;

    // Traits about the static type.
    bool isPodType;
    bool isEnumType;

    // True if we have sent a TfTypeWasDeclaredNotice for this type.
    bool hasSentNotice;

    mutable RWMutex mutex;

    ////////////////////////////////////////////////////////////////////////

    // A type is "defined" as soon as it has either type_info or a
    // Python class object.
    inline bool IsDefined() {
#ifdef PXR_PYTHON_SUPPORT_ENABLED
        return typeInfo.load() != nullptr || pyClass.get();
#else
        return typeInfo.load() != nullptr;
#endif // PXR_PYTHON_SUPPORT_ENABLED
    }

    // Caller must hold a write lock on mutex.
    void SetCastFunc(std::type_info const &baseType,
                     TfType::_CastFunction const &func) {
        // check for existing func.
        for (size_t i = 0; i < castFuncs.size(); ++i) {
            if (baseType == *castFuncs[i].first) {
                castFuncs[i].second = func;
                return;
            }
        }
        // need to add a new func.
        castFuncs.push_back(std::make_pair(&baseType, func));
    }

    // Caller must hold at least a read lock on mutex.
    TfType::_CastFunction *GetCastFunc(std::type_info const &baseType) {
        for (size_t i = 0; i < castFuncs.size(); ++i)
            if (TfSafeTypeCompare(baseType, *castFuncs[i].first))
                return &castFuncs[i].second;
        return 0;
    }

    // Caller must hold at least a read lock on mutex.
    _TypeInfo *FindByAlias(std::string const &alias) const {
        if (aliasToDerivedTypeMap) {
            auto it = aliasToDerivedTypeMap->find(alias);
            return it != aliasToDerivedTypeMap->end() ? it->second : nullptr;
        }
        return nullptr;
    }
    
    // Allocate an empty (undefined) _TypeInfo with the given typeName.
    _TypeInfo(const string &newTypeName) :
        canonicalTfType(this),
        typeName(newTypeName),
        definitionCallback(nullptr),
        typeInfo(nullptr),
        sizeofType(0),
        isPodType(false),
        isEnumType(false),
        hasSentNotice(false)
    {
    }
};

#ifdef PXR_PYTHON_SUPPORT_ENABLED
// Comparison for boost::python::handle.
struct Tf_PyHandleLess
{
    bool operator()(const boost::python::handle<> &lhs,
                    const boost::python::handle<> &rhs) const {
        return lhs.get() < rhs.get();
    }
};
#endif // PXR_PYTHON_SUPPORT_ENABLED

// Registry for _TypeInfos.
//
class Tf_TypeRegistry : boost::noncopyable
{
public:
    static Tf_TypeRegistry& GetInstance() {
        return TfSingleton<Tf_TypeRegistry>::GetInstance();
    }

    RWMutex &GetMutex() const { return _mutex; }

    inline void WaitForInitializingThread() const {
        // If we are the initializing thread or if the registry is initialized,
        // we don't have to wait.
        std::thread::id initId = _initializingThread;
        if (initId == std::thread::id() ||
            initId == std::this_thread::get_id()) {
            return;
        }

        // Otherwise spin until initialization is complete.
        while (_initializingThread != std::thread::id()) {
            std::this_thread::yield();
        }
    }
    
    // Note, callers must hold the registry lock for writing, and base's lock
    // for writing, but need not hold derived's lock.
    void AddTypeAlias(TfType::_TypeInfo *base, TfType::_TypeInfo *derived,
                      const string &alias, string *errMsg) {
        // Aliases cannot conflict with other aliases under the same base.
        if (base->aliasToDerivedTypeMap) {
            TfType::_TypeInfo::NameToTypeMap::const_iterator it =
                base->aliasToDerivedTypeMap->find(alias);
            if (it != base->aliasToDerivedTypeMap->end()) {
                if (it->second == derived) {
                    // Alias already exists; no change.
                    return;
                } else {
                    *errMsg = TfStringPrintf(
                        "Cannot set alias '%s' under '%s', because "
                        "it is already set to '%s', not '%s'.",
                        alias.c_str(),
                        base->typeName.c_str(),
                        it->second->typeName.c_str(),
                        derived->typeName.c_str());
                    return;
                }
            }
        }
        // Aliases cannot conflict with typeNames, either.
        if (_typeNameToTypeMap.count(alias) != 0) {
            *errMsg = TfStringPrintf(
                "There already is a type named '%s'; cannot "
                "create an alias of the same name.",
                alias.c_str());
            return;
        }

        if (!base->aliasToDerivedTypeMap)
            base->aliasToDerivedTypeMap = boost::in_place(0);
        (*base->aliasToDerivedTypeMap)[alias] = derived;

        if (!base->derivedTypeToAliasesMap)
            base->derivedTypeToAliasesMap = boost::in_place(0);
        (*base->derivedTypeToAliasesMap)[derived].push_back(alias);
    }

    TfType::_TypeInfo *NewTypeInfo(const string &typeName) {
        TfType::_TypeInfo *info = new TfType::_TypeInfo(typeName);
        _typeNameToTypeMap[typeName] = info;
        return info;
    }

    void SetTypeInfo(TfType::_TypeInfo *info, const std::type_info & typeInfo,
                     size_t sizeofType, bool isPodType, bool isEnumType) {
        info->typeInfo = &typeInfo;
        info->sizeofType = sizeofType;
        info->isPodType = isPodType;
        info->isEnumType = isEnumType;
        _typeInfoMap.Set(typeInfo, info);
    }

#ifdef PXR_PYTHON_SUPPORT_ENABLED
    void SetPythonClass(TfType::_TypeInfo *info,
                        const boost::python::object & classObj) {
        // Hold a reference to this PyObject in our map.
        boost::python::handle<> handle(
            boost::python::borrowed(classObj.ptr()));

        info->pyClass = handle;
        _pyClassMap[handle] = info;

        // Do not overwrite the size of a C++ type.
        if (!info->sizeofType) {
            info->sizeofType = TfSizeofType<boost::python::object>::value;
        }
    }
#endif // PXR_PYTHON_SUPPORT_ENABLED

    TfType::_TypeInfo *GetUnknownType() const { return _unknownTypeInfo; }

    TfType::_TypeInfo *GetRoot() const { return _rootTypeInfo; }

    TfType::_TypeInfo *FindByName(const string &name) const {
        auto it = _typeNameToTypeMap.find(name);
        return it != _typeNameToTypeMap.end() ? it->second : nullptr;
    }

    template <class Upgrader>
    TfType::_TypeInfo *
    FindByTypeid(const std::type_info &typeInfo, Upgrader upgrader) {
        TfType::_TypeInfo **info = _typeInfoMap.Find(typeInfo, upgrader);
        return info ? *info : nullptr;
    }

#ifdef PXR_PYTHON_SUPPORT_ENABLED
    TfType::_TypeInfo *
    FindByPythonClass(const boost::python::object &classObj) const {
        boost::python::handle<> handle(
            boost::python::borrowed(classObj.ptr()));
        auto it = _pyClassMap.find(handle);
        return it != _pyClassMap.end() ? it->second : nullptr;
    }
#endif // PXR_PYTHON_SUPPORT_ENABLED

private:
    Tf_TypeRegistry();

    mutable RWMutex _mutex;

    // The thread that is currently performing initialization.  This is set to a
    // default-constructed thread::id when initialization is complete.
    mutable std::atomic<std::thread::id> _initializingThread;

    // Map of typeName to _TypeInfo*.
    // This holds all declared types, by unique typename.
    TfType::_TypeInfo::NameToTypeMap _typeNameToTypeMap;
    
    // Map of type_info to _TypeInfo*.
    // This holds info for types that have been defined as C++ types.
    // XXX: change this to regular hash table?
    TfTypeInfoMap<TfType::_TypeInfo*> _typeInfoMap;

#ifdef PXR_PYTHON_SUPPORT_ENABLED
    // Map of python class handles to _TypeInfo*.
    typedef map<boost::python::handle<>,
                TfType::_TypeInfo *, Tf_PyHandleLess> PyClassMap;
    PyClassMap _pyClassMap;
#endif // PXR_PYTHON_SUPPORT_ENABLED

    // _TypeInfo for Unknown type
    TfType::_TypeInfo *_unknownTypeInfo;
    // _TypeInfo for Root type
    TfType::_TypeInfo *_rootTypeInfo;

    // Set true if we should send notification.
    bool _sendDeclaredNotification;

    friend class TfSingleton<Tf_TypeRegistry>;
    friend class TfType;
};

TF_INSTANTIATE_SINGLETON(Tf_TypeRegistry);

// This type is used as the unknown type. Previously, 'void' was used for
// that purpose, but clients want to call TfType::Find<void>();.
struct _TfUnknownType {};

Tf_TypeRegistry::Tf_TypeRegistry() :
    _unknownTypeInfo(0),
    _rootTypeInfo(0),
    _sendDeclaredNotification(false)
{
    // Register root type
    _rootTypeInfo = NewTypeInfo("TfType::_Root");

    // Register unknown type
    _unknownTypeInfo = NewTypeInfo("TfType::_Unknown");
    SetTypeInfo(_unknownTypeInfo, typeid(_TfUnknownType),
                /*sizeofType=*/0, /*isPodType=*/false, /*isEnumType=*/false);

    // Put the registry into an "initializing" state so that racing to get the
    // singleton instance (which will start happening immediately after calling
    // SetInstanceConstructed) will wait until initial type registrations are
    // completed.  Note that we only allow *this* thread to query the registry
    // until initialization is finished.  Others will wait.
    _initializingThread = std::this_thread::get_id();
    TfSingleton<Tf_TypeRegistry>::SetInstanceConstructed(*this);

    // We send TfTypeWasDeclaredNotice() when a type is first declared with
    // bases.  Because TfNotice delivery uses TfType, we first register both
    // TfNotice and TfTypeWasDeclaredNotice -- without sending
    // TfTypeWasDeclaredNotice for them -- before subscribing to the TfType
    // registry.
    TfType::Define<TfNotice>();
    TfType::Define<TfTypeWasDeclaredNotice, TfType::Bases<TfNotice> >();

    // From this point on, we'll send notices as new types are discovered.
    _sendDeclaredNotification = true;

    try {
        TfRegistryManager::GetInstance().SubscribeTo<TfType>();
        _initializingThread = std::thread::id();
    } catch (...) {
        // Ensure we mark initialization completed in the face of an exception.
        _initializingThread = std::thread::id();
        throw;
    }
}

////////////////////////////////////////////////////////////////////////

TfType::TfType() : _info(Tf_TypeRegistry::GetInstance().GetUnknownType())
{
}

TfType const&
TfType::GetRoot()
{
    return Tf_TypeRegistry::GetInstance().GetRoot()->canonicalTfType;
}

TfType const&
TfType::GetCanonicalType() const
{
    return _info->canonicalTfType;
}

TfType const&
TfType::FindByName(const string &name)
{
    return GetRoot().FindDerivedByName(name);
}

TfType const&
TfType::FindDerivedByName(const string &name) const
{
    if (IsUnknown())
        return GetUnknownType();

    TfType result;

    // Note that we cache results in derivedByNameCache, and we never invalidate
    // this cache.  This works because 1) we never remove types and type
    // information from TfType's data structures and 2) we only cache if we find
    // a valid type.
    ScopedLock thisInfoLock(_info->mutex, /*write=*/false);
    if (ARCH_LIKELY(_info->derivedByNameCache &&
                    TfMapLookup(*(_info->derivedByNameCache), name, &result))) {
        // Cache hit.  We're done.
        return result._info->canonicalTfType;
    }
    // Look for a type derived from *this, and has the given name as an alias.
    if (TfType::_TypeInfo *foundInfo = _info->FindByAlias(name)) {
        result = TfType(foundInfo);
    }
    // Finished reading _info data.
    thisInfoLock.release();

    // If we didn't find an alias we now look in the registry.
    if (!result) {
        const auto &r = Tf_TypeRegistry::GetInstance();
        r.WaitForInitializingThread();
        ScopedLock regLock(r.GetMutex(), /*write=*/false);
        TfType::_TypeInfo *foundInfo = r.FindByName(name);
        regLock.release();
        if (foundInfo) {
            // Next look for a type with the given typename.  If a type was 
            // found, verify that it derives from *this. 
            result = TfType(foundInfo);
            if (!result.IsA(*this))
                result = TfType();
        }
    }

    // Populate cache.
    if (result) {
        // It's possible that some other thread has done this already, but it
        // will be the same result so it's okay to do redundantly in that case.
        thisInfoLock.acquire(_info->mutex, /*write=*/true);
        if (!_info->derivedByNameCache) {
            _info->derivedByNameCache.
                reset(new _TypeInfo::DerivedByNameCache(0));
        }
        _info->derivedByNameCache->insert(make_pair(name, result));
    }

    return result._info->canonicalTfType;
}

TfType const&
TfType::GetUnknownType()
{
    return Tf_TypeRegistry::GetInstance().GetUnknownType()->canonicalTfType;
}

TfType const&
TfType::_FindByTypeid(const std::type_info &typeInfo)
{
    // Functor to upgrade the read lock to a write lock.
    struct WriteUpgrader {
        WriteUpgrader(ScopedLock& lock) : lock(lock) { }
        void operator()() { lock.upgrade_to_writer(); }
        ScopedLock& lock;
    };

    auto &r = Tf_TypeRegistry::GetInstance();
    r.WaitForInitializingThread();

    ScopedLock readLock(r.GetMutex(), /*write=*/false);
    TfType::_TypeInfo *info = r.FindByTypeid(typeInfo, WriteUpgrader(readLock));

    return info ? info->canonicalTfType : GetUnknownType();
}

#ifdef PXR_PYTHON_SUPPORT_ENABLED
TfType const&
TfType::FindByPythonClass(const TfPyObjWrapper & classObj)
{
    const auto &r = Tf_TypeRegistry::GetInstance();
    r.WaitForInitializingThread();

    ScopedLock readLock(r.GetMutex(), /*write=*/false);
    TfType::_TypeInfo *info = r.FindByPythonClass(classObj.Get());

    return info ? info->canonicalTfType : GetUnknownType();
}
#endif // PXR_PYTHON_SUPPORT_ENABLED

const string &
TfType::GetTypeName() const
{
    return _info->typeName;
}

const std::type_info &
TfType::GetTypeid() const
{
    std::type_info const *typeInfo = _info->typeInfo;
    return typeInfo ? *typeInfo : typeid(void);
}

#ifdef PXR_PYTHON_SUPPORT_ENABLED
TfPyObjWrapper
TfType::GetPythonClass() const
{
    if (!TfPyIsInitialized())
        TF_CODING_ERROR("Python has not been initialized");

    ScopedLock lock(_info->mutex, /*write=*/false);
    if (_info->pyClass.get())
        return TfPyObjWrapper(boost::python::object(_info->pyClass));
    return TfPyObjWrapper();
}
#endif // PXR_PYTHON_SUPPORT_ENABLED

vector<string>
TfType::GetAliases(TfType derivedType) const
{
    ScopedLock lock(_info->mutex, /*write=*/false);
    if (_info->derivedTypeToAliasesMap) {
        auto i = _info->derivedTypeToAliasesMap->find(derivedType._info);
        if (i != _info->derivedTypeToAliasesMap->end())
            return i->second;
    }
    return vector<string>();
}

vector<TfType>
TfType::GetBaseTypes() const
{
    ScopedLock lock(_info->mutex, /*write=*/false);
    return _info->baseTypes;
}

vector<TfType>
TfType::GetDirectlyDerivedTypes() const
{
    ScopedLock lock(_info->mutex, /*write=*/false);
    return _info->derivedTypes;
}

void
TfType::GetAllDerivedTypes(std::set<TfType> *result) const
{
    ScopedLock lock(_info->mutex, /*write=*/false);
    for (auto derivedType: _info->derivedTypes) {
        result->insert(derivedType);
        derivedType.GetAllDerivedTypes(result);
    }
}

// Helper for resolving ancestor order in the case of multiple inheritance.
static bool
_MergeAncestors(vector<TypeVector> *seqs, TypeVector *result)
{
    while(true)
    {
        // Find a candidate for the next type.
        TfType cand;

        // Try the first element of each non-empty sequence, in order.
        bool anyLeft = false;
        TF_FOR_ALL(candSeq, *seqs)
        {
            if (candSeq->empty())
                continue;
                
            anyLeft = true;
            cand = candSeq->front();

            // Check that the candidate does not occur in the tail
            // ("cdr", in lisp terms) of any of the sequences.
            TF_FOR_ALL(checkSeq, *seqs)
            {
                if (checkSeq->size() <= 1)
                    continue;

                if (std::find( ++(checkSeq->begin()), checkSeq->end(), cand )
                    != checkSeq->end())
                {
                    // Reject this candidate.
                    cand = TfType();
                    break;
                }
            }

            if (!cand.IsUnknown()) {
                // Found a candidate
                break;
            }
        }


        if (cand.IsUnknown()) {
            // If we were unable to find a candidate, we're done.
            // If we've consumed all the inputs, then we've succeeded.
            // Otherwise, the inheritance hierarchy is inconsistent.
            return !anyLeft;
        }

        result->push_back(cand);

        // Remove candidate from input sequences.
        TF_FOR_ALL(seqIt, *seqs) {
            if (!seqIt->empty() && seqIt->front() == cand)
                seqIt->erase( seqIt->begin() );
        }
    }
}

void
TfType::GetAllAncestorTypes(vector<TfType> *result) const
{
    if (IsUnknown()) {
        TF_CODING_ERROR("Cannot ask for ancestor types of Unknown type");
        return;
    }

    const vector<TfType> &baseTypes = GetBaseTypes();
    const size_t numBaseTypes = baseTypes.size();

    // Simple case: single (or no) inheritance
    if (numBaseTypes <= 1) {
        result->push_back(*this);
        if (numBaseTypes == 1)
            baseTypes.front().GetAllAncestorTypes(result);
        return;
    }

    // Use the C3 algorithm for resolving multiple inheritance;
    // see motivating comments in header.  If this turns out to be a
    // performance problem, consider memoizing this algorithm.

    vector<TypeVector> seqs;
    seqs.reserve(2 + numBaseTypes);

    // 1st input sequence: This class.
    seqs.push_back( TypeVector() );
    seqs.back().push_back(*this);

    // 2nd input sequence: Direct bases, in order.
    seqs.push_back( baseTypes );

    // Remaining sequences: Inherited types for each direct base.
    TF_FOR_ALL(it, baseTypes) {
        // Populate the base's ancestor types directly into a new vector on
        // the back of seqs.
        seqs.push_back( TypeVector() );
        TypeVector &baseSeq = seqs.back();
        it->GetAllAncestorTypes(&baseSeq);
    }

    // Merge the input sequences to resolve final inheritance order.
    bool ok = _MergeAncestors( &seqs, result );

    if (!ok) {
        TF_CODING_ERROR("Cannot resolve ancestor classes for '%s' "
                        "because the inheritance hierarchy is "
                        "inconsistent.  Please check that multiply-"
                        "inherited types are inherited in the same order "
                        "throughout the inherited hierarchy.",
                        GetTypeName().c_str());
    }
}

#ifdef PXR_PYTHON_SUPPORT_ENABLED
TfType const &
TfType::_FindImplPyPolymorphic(PyPolymorphicBase const *ptr) {
    using namespace boost::python;
    TfType ret;
    if (TfPyIsInitialized()) {
        TfPyLock lock;
        // See if we can find a polymorphic python object...
        object pyObj = Tf_FindPythonObject(
            TfCastToMostDerivedType(ptr), typeid(*ptr));
        if (!TfPyIsNone(pyObj))
            ret = FindByPythonClass(
                TfPyObjWrapper(pyObj.attr("__class__")));
    }
    return !ret.IsUnknown() ? ret.GetCanonicalType() : Find(typeid(*ptr));
}
#endif // PXR_PYTHON_SUPPORT_ENABLED

bool
TfType::_IsAImpl(TfType queryType) const
{
    // Iterate until we reach more than one parent.
    for (TfType t = *this; ; ) {
        if (t == queryType)
            return true;

        ScopedLock lock(t._info->mutex, /*write=*/false);
        if (t._info->baseTypes.size() == 1) {
            t = t._info->baseTypes[0];
            continue;
        }
        for (size_t i = 0; i != t._info->baseTypes.size(); i++)
            if (t._info->baseTypes[i]._IsAImpl(queryType))
                return true;
        return false;
    }
}

bool
TfType::IsA(TfType queryType) const
{
    if (queryType.IsUnknown()) {
        // If queryType is unknown, it almost always means a previous
        // type lookup failed, and went unchecked.
        TF_RUNTIME_ERROR("IsA() was given an Unknown base type.  "
                         "This probably means the attempt to look up the "
                         "base type failed.  (Note: to explicitly check if a "
                         "type is unknown, use IsUnknown() instead.)");
        return false;
    }
    if (IsUnknown()) {
        return false;
    }

    if (*this == queryType || queryType.IsRoot()) {
        return true;
    }

    // If the query type doesn't have any child types, then iterating over all
    // our base types wastes time.
    ScopedLock queryLock(queryType._info->mutex, /*write=*/false);
    if (queryType._info->derivedTypes.empty()) {
        return false;
    }
    queryLock.release();

    // printf("--- %s IsA %s { ",
    //        GetTypeName().c_str(), queryType.GetTypeName().c_str());

    bool ret = _IsAImpl(queryType);

    // printf(" } = %d\n", ret);

    return ret;
}

TfType const &
TfType::Declare(const string &typeName)
{
    TfAutoMallocTag2 tag("Tf", "TfType::Declare");

    TfType t = FindByName(typeName);
    if (t.IsUnknown()) {
        auto &r = Tf_TypeRegistry::GetInstance();
        ScopedLock lock(r.GetMutex(), /*write=*/true);
        t = TfType(r.NewTypeInfo(typeName));
        TF_AXIOM(!t._info->IsDefined());
    }
    return t.GetCanonicalType();
}

TfType const&
TfType::Declare(const string &typeName,
                const vector<TfType> &newBases,
                DefinitionCallback definitionCallback)
{
    TfAutoMallocTag2 tag("Tf", "TfType::Declare");

    TfType const& t = Declare(typeName);

    bool sendNotice = false;
    vector<string> errorsToEmit;
    {
        auto &r = Tf_TypeRegistry::GetInstance();
        ScopedLock regLock(r.GetMutex(), /*write=*/true);
        ScopedLock typeLock(t._info->mutex, /*write=*/true);

        if (t.IsUnknown() || t.IsRoot()) {
            errorsToEmit.push_back(
                TfStringPrintf("Cannot declare the type '%s'",
                               t.GetTypeName().c_str()));
            goto errorOut;
        }

        // Update base types.
        vector<TfType> &bases = t._info->baseTypes;

        // If this type already directly inherits from root, then
        // prohibit adding any new bases.
        if (!newBases.empty() && bases == vector<TfType>(1, GetRoot())) {
            errorsToEmit.push_back(
                TfStringPrintf("Type '%s' has been declared to have 0 bases, "
                               "and therefore inherits directly from the root "
                               "type.  Cannot add bases.",
                               t.GetTypeName().c_str()));
            goto errorOut;
        }

        if (newBases.empty()) {
            if (bases.empty()) {
                // If we don't have any bases yet, add the root type.
                t._AddBase(GetRoot());
            }
        } else {
            // Otherwise, add the new bases.

            // First, check that all previously-declared bases are included.
            TF_FOR_ALL(it, bases) {
                if (std::find(newBases.begin(), newBases.end(), *it) ==
                    newBases.end())
                {
                    string newBasesStr;
                    TF_FOR_ALL(it2, newBases)
                        newBasesStr += it2->GetTypeName() + " ";

                    errorsToEmit.push_back(
                        TfStringPrintf("TfType '%s' was previously declared to "
                                       "have '%s' as a base, but subsequent "
                                       "declaration does not include this as a "
                                       "base.  The newly given bases were: "
                                       "(%s).  If this is a type declared in a "
                                       "plugin, check that the plugin metadata "
                                       "is correct.",
                                       t.GetTypeName().c_str(),
                                       it->GetTypeName().c_str(),
                                       newBasesStr.c_str()));
                }
            }

            TF_FOR_ALL(it, newBases)
                t._AddBase(*it);

        }

        if (definitionCallback) {
            // Prohibit re-declaration of definitionCallback.
            if (t._info->definitionCallback) {
                errorsToEmit.push_back(
                    TfStringPrintf("TfType '%s' has already had its "
                                   "definitionCallback set; ignoring 2nd "
                                   "declaration", typeName.c_str()));
                goto errorOut;
            }
            t._info->definitionCallback = definitionCallback;
        }

        // Send a notice about this type if we have not done so yet.
        if (r._sendDeclaredNotification && !t._info->hasSentNotice) {
            t._info->hasSentNotice = sendNotice = true;
        }
    }

    if (sendNotice)
        TfTypeWasDeclaredNotice(t).Send();

errorOut:

    // Emit any errors.
    for (auto const &msg: errorsToEmit)
        TF_CODING_ERROR(msg);

    return t;
}

#ifdef PXR_PYTHON_SUPPORT_ENABLED
void
TfType::DefinePythonClass(const TfPyObjWrapper & classObj) const
{
    if (IsUnknown() || IsRoot()) {
        TF_CODING_ERROR("cannot define Python class because type is unknown");
        return;
    }
    auto &r = Tf_TypeRegistry::GetInstance();
    ScopedLock infoLock(_info->mutex, /*write=*/true);
    ScopedLock regLock(r.GetMutex(), /*write=*/true);
    if (!TfPyIsNone(_info->pyClass)) {
        infoLock.release();
        regLock.release();
        TF_CODING_ERROR("TfType '%s' already has a defined Python type; "
                        "cannot redefine", GetTypeName().c_str());
        return;
    }
    r.SetPythonClass(_info, classObj.Get());
}
#endif // PXR_PYTHON_SUPPORT_ENABLED

void
TfType::_DefineCppType(const std::type_info & typeInfo,
                       size_t sizeofType, bool isPodType, bool isEnumType) const
{
    auto &r = Tf_TypeRegistry::GetInstance();
    ScopedLock infoLock(_info->mutex, /*write=*/true);
    ScopedLock regLock(r.GetMutex(), /*write=*/true);
    if (_info->typeInfo.load() != nullptr) {
        infoLock.release();
        regLock.release();
        TF_CODING_ERROR("TfType '%s' already has a defined C++ type; "
                        "cannot redefine", GetTypeName().c_str());
        return;
    }
    r.SetTypeInfo(_info, typeInfo, sizeofType, isPodType, isEnumType);
}

void
TfType::_AddBase(TfType base) const
{
    // Callers must hold _info write lock.
    if (base.IsUnknown()) {
        TF_CODING_ERROR("Specified base type is unknown, skipping");
        return;
    }
    vector<TfType> & bases = _info->baseTypes;
    if (std::find(bases.begin(), bases.end(), base) == bases.end()) {
        // Add the new base.
        bases.push_back(base);
        // Tell the new base that it has a new derived type.
        ScopedLock baseLock(base._info->mutex, /*write=*/true);
        base._info->derivedTypes.push_back(*this);
    }
}

void
TfType::_AddCppCastFunc( const std::type_info & baseTypeInfo,
                         _CastFunction func ) const
{
    ScopedLock infoLock(_info->mutex, /*write=*/true);
    _info->SetCastFunc(baseTypeInfo, func);
}

void*
TfType::CastToAncestor(TfType ancestor, void* addr) const
{
    if (IsUnknown() || ancestor.IsUnknown())
        return 0;
        
    // Iterate until we reach more than one parent.
    for (TfType t = *this; ; ) {
        if (t == ancestor)
            return addr;
        ScopedLock lock(t._info->mutex, /*write=*/false);
        if (t._info->baseTypes.size() == 1) {
            _CastFunction *castFunc =
                t._info->GetCastFunc(t._info->baseTypes[0].GetTypeid());
            if (castFunc) {
                addr = (*castFunc)(addr, true);
                t = t._info->baseTypes[0];
                continue;
            } else {
                return nullptr;
            }
        }
        for (size_t i = 0; i < t._info->baseTypes.size(); i++) {
            _CastFunction *castFunc =
                t._info->GetCastFunc(t._info->baseTypes[i].GetTypeid());
            if (castFunc) {
                void *pAddr = (*castFunc)(addr, true);
                if (void *final =
                    t._info->baseTypes[i].CastToAncestor(ancestor, pAddr))
                    return final;
            }
        }
        return nullptr;
    }
}

void*
TfType::CastFromAncestor(TfType ancestor, void* addr) const
{
    if (IsUnknown() || ancestor.IsUnknown())
        return 0;

    // No iteration: we have to do the purely recursively, because
    // each cast has to happen on the way back *down* the type tree.
    if (*this == ancestor)
        return addr;

    ScopedLock lock(_info->mutex, /*write=*/false);
    TF_FOR_ALL(it, _info->baseTypes) { 
        if (void* tmp = it->CastFromAncestor(ancestor, addr)) {
            if (_CastFunction *castFunc = _info->GetCastFunc(it->GetTypeid()))
                return (*castFunc)(tmp, false);
        }
    }

    return nullptr;
}

void
TfType::SetFactory(std::unique_ptr<FactoryBase> factory) const
{
    if (IsUnknown() || IsRoot()) {
        TF_CODING_ERROR("Cannot set factory of %s\n",
                        GetTypeName().c_str());
        return;
    }

    ScopedLock infoLock(_info->mutex, /*write=*/true);
    if (_info->factory) {
        infoLock.release();
        TF_CODING_ERROR("Cannot change the factory of %s\n",
                        GetTypeName().c_str());
        return;
    }

    _info->factory = std::move(factory);
}

TfType::FactoryBase*
TfType::_GetFactory() const
{
    if (IsUnknown() || IsRoot()) {
        TF_CODING_ERROR("Cannot manufacture type %s", GetTypeName().c_str());
        return NULL;
    }

    _ExecuteDefinitionCallback();

    ScopedLock infoLock(_info->mutex, /*write=*/false);
    return _info->factory.get();
}

void
TfType::_ExecuteDefinitionCallback() const
{
    // We don't want to call the definition callback while holding the
    // registry's lock, so first copy it with the lock held then
    // execute it.
    ScopedLock infoLock(_info->mutex, /*write=*/false);
    if (DefinitionCallback definitionCallback = _info->definitionCallback) {
        infoLock.release();
        definitionCallback(*this);
    }
}

string
TfType::GetCanonicalTypeName(const std::type_info &t)
{
    return ArchGetDemangled(t);
}

void
TfType::AddAlias(TfType base, const string & name) const
{
    std::string errMsg;
    {
        auto &r = Tf_TypeRegistry::GetInstance();
        ScopedLock infoLock(base._info->mutex, /*write=*/true);
        ScopedLock regLock(r.GetMutex(), /*write=*/true);
        // We do not need to hold our own lock here.
        r.AddTypeAlias(base._info, this->_info, name, &errMsg);
    }
    
    if (!errMsg.empty())
        TF_CODING_ERROR(errMsg);
}

bool
TfType::IsEnumType() const
{
    ScopedLock lock(_info->mutex, /*write=*/false);
    return _info->isEnumType;
}

bool
TfType::IsPlainOldDataType() const
{
    ScopedLock lock(_info->mutex, /*write=*/false);
    return _info->isPodType;
}

size_t
TfType::GetSizeof() const
{
    ScopedLock lock(_info->mutex, /*write=*/false);
    return _info->sizeofType;
}


TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<void>();
    TfType::Define<bool>();
    TfType::Define<char>();
    TfType::Define<signed char>();
    TfType::Define<unsigned char>();
    TfType::Define<short>();
    TfType::Define<unsigned short>();
    TfType::Define<int>();
    TfType::Define<unsigned int>();
    TfType::Define<long>();
    TfType::Define<unsigned long>().AddAlias( TfType::GetRoot(), "size_t" );
    TfType::Define<long long>();
    TfType::Define<unsigned long long>();    
    TfType::Define<float>();
    TfType::Define<double>();
    TfType::Define<string>();

    TfType::Define< vector<bool> >()
        .AddAlias( TfType::GetRoot(), "vector<bool>" );
    TfType::Define< vector<char> >()
        .AddAlias( TfType::GetRoot(), "vector<char>" );
    TfType::Define< vector<unsigned char> >()
        .AddAlias( TfType::GetRoot(), "vector<unsigned char>" );
    TfType::Define< vector<short> >()
        .AddAlias( TfType::GetRoot(), "vector<short>" );
    TfType::Define< vector<unsigned short> >()
        .AddAlias( TfType::GetRoot(), "vector<unsigned short>" );
    TfType::Define< vector<int> >()
        .AddAlias( TfType::GetRoot(), "vector<int>" );
    TfType::Define< vector<unsigned int> >()
        .AddAlias( TfType::GetRoot(), "vector<unsigned int>" );
    TfType::Define< vector<long> >()
        .AddAlias( TfType::GetRoot(), "vector<long>" );

    TfType ulvec = TfType::Define< vector<unsigned long> >();
    ulvec.AddAlias( TfType::GetRoot(), "vector<unsigned long>" );
    ulvec.AddAlias( TfType::GetRoot(), "vector<size_t>" );

    TfType::Define< vector<long long> >()
        .AddAlias( TfType::GetRoot(), "vector<long long>" );
    TfType::Define< vector<unsigned long long> >()
        .AddAlias( TfType::GetRoot(), "vector<unsigned long long>" );

    TfType::Define< vector<float> >()
        .AddAlias( TfType::GetRoot(), "vector<float>" );
    TfType::Define< vector<double> >()
        .AddAlias( TfType::GetRoot(), "vector<double>" );
    TfType::Define< vector<string> >()
        .AddAlias( TfType::GetRoot(), "vector<string>" );

    // Register TfType itself.
    TfType::Define<TfType>();
}

std::ostream &
operator<<(std::ostream& out, const TfType& t)
{
    return out << t.GetTypeName();
}

PXR_NAMESPACE_CLOSE_SCOPE
