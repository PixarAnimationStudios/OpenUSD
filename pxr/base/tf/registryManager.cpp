//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// The registry manager is used to lazily run code needed by consumers of
// some type.  This code is found automatically by the registry manager
// and executed when the consumer "subscribes to" the type and when loading
// a library with subscribed-to types.
//
// Finding the code automatically requires some magic.  What we want to do
// is add function/type pairs to the registry.  When a client "subscribes"
// to a type, all registered functions for that type should be executed in
// the order they were added (at least to the extent that all functions in
// a shared library are executed before any registered functions in any
// shared library that depends on it).
//
// To achieve this our TF_REGISTRY_FUNCTION() macro will produce a static
// constructor function that is executed when the shared library (or main
// program) is loaded.  It simply advertises the existence of the registry
// function to the registry manager. If a library uses that macro, we also emit
// a single static initializer that runs after all the static constructors to
// let the registry manager know when a library's constructors have completed.
//
// The dynamic loader automatically ensures that all constructors in a library
// are run before running any code in another library that depends on it
// (i.e. it does a topological sort of library dependencies and fully
// initializes each library before moving on to the next).
//
// The underlying mechanisms to achieve this are somewhat
// platform-dependent. Trace back the TF_REGISTRY_DEFINE() macro definitions
// from registryManager.h and look at what those invoke in arch/attributes.h for
// all the gory details.
//
// Note again that this is only about _discovering_ the registry functions that
// exist. We don't _execute_ registry functions until some client code requests
// them via a call to TfRegistryManager::SubscribeTo<SomeRegistryType>(). The
// point of this mechanism is to avoid doing significant work at static
// initialization time, rather just being ready to do initializations when
// requested.
//
#include "pxr/pxr.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/debugCodes.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/dl.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/scoped.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/symbols.h"

#include <tbb/enumerable_thread_specific.h>

#include <cstdlib>
#include <list>
#include <map>
#include <mutex>
#include <set>
#include <vector>

using std::list;
using std::map;
using std::set;
using std::string;
using std::type_info;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// Convenience for moving the contents of one list to the back of another.
template <class L>
void
MoveToBack(L& dst, L& src)
{
    dst.splice(dst.end(), src);
}

std::string
GetLibraryPath(const char* libraryName,
               TfRegistryManager::RegistryFunctionType func)
{
    std::string result = libraryName;
    ArchGetAddressInfo(reinterpret_cast<void*>(func), &result, NULL, NULL,NULL);
    return result;
}

class Tf_RegistryManagerImpl {
    Tf_RegistryManagerImpl(const Tf_RegistryManagerImpl&) = delete;
    Tf_RegistryManagerImpl& operator=(const Tf_RegistryManagerImpl&) = delete;

public:
    typedef size_t LibraryIdentifier;
    typedef TfRegistryManager::RegistryFunctionType RegistryFunction;
    typedef TfRegistryManager::UnloadFunctionType UnloadFunction;

    static Tf_RegistryManagerImpl& GetInstance() {
         return TfSingleton<Tf_RegistryManagerImpl>::GetInstance();
    }

    /// Stores the active library's registry functions and runs those
    /// that are subscribed to then makes no library active.
    void ClearActiveLibrary(const char* libraryName);

    /// Adds a registry function for the library.  Reports an error
    /// and otherwise does nothing if \p libraryName is \c NULL or empty
    /// or if \p typeName is \c NULL or empty.  In addition, this must
    /// be the first call to this method since \c ClearActiveLibrary()
    /// was called or \p libraryName must match the previous call's
    /// \p libraryName.  This happens naturally when loading shared
    /// libraries:  the dynamic loader does a topological sort on the
    /// libraries then loads them one at a time, calling all of the
    /// constructor functions for a given library before calling those
    /// on any other library.  The first call from a library makes that
    /// library active and it remains so until \c ClearActiveLibrary().
    void AddRegistryFunction(const char* libraryName,
                             RegistryFunction, const char* typeName);

    /// Adds a function for unload for the library associated with the
    /// running registry function.  Reports an error and otherwise does
    /// nothing if there is no running registry function.
    bool AddFunctionForUnload(const UnloadFunction& func);

    /// Run the unload functions for library \p libraryName if they haven't
    /// already been run.
    void UnloadLibrary(const char* libraryName);

    /// Subscribe to a type.  This causes all of the registry functions
    /// for the type to be run if they haven't run already and causes any
    /// registry functions added later for that type to run during
    /// library load.
    void SubscribeTo(const string& typeName);

    /// Unsubscribe from a type.  Any registry functions added later
    /// will not automatically run but they're saved in case \c SubscribeTo()
    /// is called for the type.
    void UnsubscribeFrom(const string& typeName);

    static bool runUnloadersAtExit;

private:
    typedef string LibraryName;

    Tf_RegistryManagerImpl();
    ~Tf_RegistryManagerImpl();

    LibraryIdentifier _RegisterLibraryNoLock(const char* libraryName);
    void _ProcessLibraryNoLock();
    void _UpdateSubscribersNoLock();
    bool _TransferActiveLibraryNoLock();
    void _RunRegistryFunctionsNoLock(const string& typeName);
    void _UnloadNoLock(const char* libraryName);

private:

    using TypeName = string;
    using _LibraryNameMap = map<LibraryName, LibraryIdentifier>;
    struct _RegistryValue {
        _RegistryValue(RegistryFunction function_,
                       LibraryIdentifier identifier_) :
            function(function_), unloadKey(identifier_) {}
        
        RegistryFunction function;
        LibraryIdentifier unloadKey;
    };
    using _RegistryValueList = list<_RegistryValue>;
    using _RegistryFunctionMap =
        TfHashMap<TypeName, _RegistryValueList, TfHash>;
    using _UnloadFunctionList = list<UnloadFunction>;
    using _UnloadFunctionMap = TfHashMap<LibraryIdentifier,
                                         _UnloadFunctionList, TfHash>;

    struct _ActiveLibraryState {
        _ActiveLibraryState() : identifier(0) {}

        LibraryIdentifier identifier;
        LibraryName name;
        _RegistryFunctionMap registryFunctions;
    };

    // Misc state.
    std::recursive_mutex _mutex;

    // Subscription state.
    _LibraryNameMap _libraryNameMap;
    set<TypeName> _subscriptions;
    list<TypeName> _orderedSubscriptions;
    _RegistryFunctionMap _registryFunctions;
    _UnloadFunctionMap _unloadFunctions;

    // Registry state.
    tbb::enumerable_thread_specific<_UnloadFunctionList*> _currentUnloadList;

    // Active library registry state.
    tbb::enumerable_thread_specific<_ActiveLibraryState> _active;

    friend class TfSingleton<Tf_RegistryManagerImpl>;
};

bool Tf_RegistryManagerImpl::runUnloadersAtExit = false;

Tf_RegistryManagerImpl::Tf_RegistryManagerImpl()
{
    // Call SetInstanceConstructed since TfDebug will end up calling back here.
    TfSingleton<Tf_RegistryManagerImpl>::SetInstanceConstructed(*this);

    TF_DEBUG_MSG(TF_DISCOVERY_TERSE, "TfRegistryManager: initialized\n");
}

Tf_RegistryManagerImpl::~Tf_RegistryManagerImpl()
{
    // Do nothing
}

void
Tf_RegistryManagerImpl::ClearActiveLibrary(const char* libraryName)
{
    TF_AXIOM(libraryName && libraryName[0]);

    // If the name doesn't match then libraryName has already been processed.
    if (_active.local().name == libraryName) {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        _ProcessLibraryNoLock();
    }
}

void
Tf_RegistryManagerImpl::AddRegistryFunction(
    const char* libraryName,
    RegistryFunction func,
    const char* typeName)
{
    if (!TF_VERIFY(libraryName && libraryName[0],
                   "TfRegistryManager: Ignoring library with no name")) {
        return;
    }
    else if (!TF_VERIFY(typeName && typeName[0],
                        "TfRegistryManager: Ignoring registry function with "
                        "no type in %s", libraryName)) {
        return;
    }

    // If there's an active library and we're getting a different library
    // then we must have started running constructors for global objects
    // and they're pulling in another library.  Finish up the previous
    // library.
    _ActiveLibraryState& active = _active.local();
    if (active.name != libraryName) {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        _ProcessLibraryNoLock();
    }

    if (!active.identifier) {
        TF_DEBUG_MSG(TF_DISCOVERY_TERSE,
                     "TfRegistryManager: Library %s\n",
                     GetLibraryPath(libraryName, func).c_str());

        // Set active.
        active.name = libraryName;

        // Access shared members.
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        active.identifier = _RegisterLibraryNoLock(libraryName);
    }
    TF_AXIOM(active.identifier);

    active.registryFunctions[typeName].
        push_back(_RegistryValue(func, active.identifier));
}

bool
Tf_RegistryManagerImpl::AddFunctionForUnload(const UnloadFunction& func)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    if (_UnloadFunctionList* unloadList = _currentUnloadList.local()) {
        unloadList->push_back(func);
        return true;
    }
    else {
        return false;
    }
}

void
Tf_RegistryManagerImpl::UnloadLibrary(const char* libraryName)
{
    if (Tf_DlCloseIsActive() || runUnloadersAtExit) {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        _UnloadNoLock(libraryName);
    }
}

void
Tf_RegistryManagerImpl::SubscribeTo(const string& typeName)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    // It's possible to get here before our indication that we've finished
    // registering functions if it runs after a global constructor that
    // subscribes to a type.  Either way, we've finished registering
    // functions since those all get done before global constructors run.
    // So process the active library, if any.
    _ProcessLibraryNoLock();

    if (_subscriptions.insert(typeName).second) {
        _orderedSubscriptions.push_back(typeName);
        _RunRegistryFunctionsNoLock(typeName);
    }
}

void
Tf_RegistryManagerImpl::UnsubscribeFrom(const string& typeName)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_subscriptions.erase(typeName)) {
        _orderedSubscriptions.remove(typeName);
    }
}

Tf_RegistryManagerImpl::LibraryIdentifier
Tf_RegistryManagerImpl::_RegisterLibraryNoLock(const char* libraryName)
{
    // Return a unique identifier for libraryName.
    LibraryIdentifier& identifier = _libraryNameMap[libraryName];
    if (identifier == 0) {
        identifier = _libraryNameMap.size();
    }
    return identifier;
}

void
Tf_RegistryManagerImpl::_ProcessLibraryNoLock()
{
    if (_active.local().identifier) {
        // Going inactive.  Move active library state over to global state.
        if (_TransferActiveLibraryNoLock()) {
            // Run subscriptions.
            _UpdateSubscribersNoLock();
        }
    }
}

void
Tf_RegistryManagerImpl::_UpdateSubscribersNoLock()
{
    for (const auto& typeName : _orderedSubscriptions) {
        _RunRegistryFunctionsNoLock(typeName);
    }
}

bool
Tf_RegistryManagerImpl::_TransferActiveLibraryNoLock()
{
    bool movedAny = false;

    // Move active library functions to non-thread local storage type by type.
    _ActiveLibraryState& active = _active.local();
    for(auto& v : active.registryFunctions) {
        if (!movedAny && !v.second.empty()) {
            movedAny = (_subscriptions.count(v.first) != 0);
        }
        MoveToBack(_registryFunctions[v.first], v.second);
    }

    // Reset.
    active.identifier = 0;
    active.name.clear();
    active.registryFunctions.clear();

    return movedAny;
}

void
Tf_RegistryManagerImpl::_RunRegistryFunctionsNoLock(const string& typeName)
{
    _RegistryFunctionMap::iterator i = _registryFunctions.find(typeName);
    if (i == _registryFunctions.end()) {
        TF_DEBUG_MSG(TF_DISCOVERY_TERSE,
                     "TfRegistryManager: no functions to run for %s\n",
                     typeName.c_str());
        return;
    }

    // Arrange to restore _currentUnloadList when we leave this function.
    _UnloadFunctionList * &currentUnloadList = _currentUnloadList.local();
    TfScopedVar restoreUnloadList { currentUnloadList, currentUnloadList };

    do {
        _RegistryValueList toRun = std::move(i->second);
        i->second.clear();
        TF_AXIOM(i->second.empty());
        if (toRun.empty()) {
            break;
        }

        TF_DEBUG_MSG(TF_DISCOVERY_TERSE,
                     "TfRegistryManager: running %zd functions for %s\n",
                     toRun.size(), typeName.c_str());

        // Fetch the first unloadKey and set currentUnloadList to the unload
        // functions for it.
        LibraryIdentifier unloadKey = toRun.front().unloadKey;
        currentUnloadList = &_unloadFunctions[unloadKey];

        _mutex.unlock();
        for (_RegistryValue const &regValue: toRun) {
            try {
                if (regValue.unloadKey != unloadKey) {
                    // If the unloadKey changes, temporarily retake the lock and
                    // repoint the currentUnloadList.
                    unloadKey = regValue.unloadKey;
                    std::scoped_lock lock(_mutex);
                    currentUnloadList = &_unloadFunctions[unloadKey];
                }
                regValue.function(nullptr, nullptr);
            }
            catch (std::exception const &exc) {
                TF_RUNTIME_ERROR("Exception thrown running registry functions "
                                 "for %s: %s", typeName.c_str(), exc.what());
            }
            catch (...) { 
                TF_RUNTIME_ERROR("Unknown exception thrown running registry "
                                 "functions for %s", typeName.c_str());
            }
        }
        _mutex.lock();

        // It's possible that while we unlocked the mutex and ran registry
        // functions, more registry functions were added for typeName.  In that
        // case we'll keep running them until the list is empty while we hold
        // the lock.
    } while (!i->second.empty());
}

void 
Tf_RegistryManagerImpl::_UnloadNoLock(const char* libraryName)
{
    TF_DEBUG_MSG(TF_DISCOVERY_TERSE,
                 "TfRegistryManager: unloading '%s'\n", libraryName);

    TF_AXIOM(libraryName && libraryName[0]);

    LibraryIdentifier identifier   = _RegisterLibraryNoLock(libraryName);
    _UnloadFunctionMap::iterator i = _unloadFunctions.find(identifier);
    if (i != _unloadFunctions.end()) {
        // Unload functions were registered.
        // Move the list to avoid worrying about modifications while unloading.
        _UnloadFunctionList unloadFunctions;
        i->second.swap(unloadFunctions);
        TF_AXIOM(i->second.empty());

        // Run the unload functions
        for (const auto& func : unloadFunctions) {
            func();
        }
    }

    /*
     * Remove any registry functions for the library.  This prevents
     * crashes where the registry manager could attempt to execute a
     * registry function from the unloaded library.
     */
    for (auto& k : _registryFunctions) {
        _RegistryValueList& regValues = k.second;
        _RegistryValueList::iterator regValueIt = regValues.begin();
        while (regValueIt != regValues.end()) {
            if (regValueIt->unloadKey == identifier) {
                regValues.erase(regValueIt++);
            }
            else {
                ++regValueIt;
            }
        }
    }
}

} // anonymous namespace

TF_INSTANTIATE_SINGLETON(Tf_RegistryManagerImpl);

TfRegistryManager::TfRegistryManager()
{
    /*
     * Our own debug symbols need to get defined in debug.cpp,
     * because of initialization order issues.
     */
}

TfRegistryManager::~TfRegistryManager()
{
    // Do nothing
}

TfRegistryManager&
TfRegistryManager::GetInstance()
{
    // We don't bother with a TfSingleton here.  The real singleton
    // (Tf_RegistryManagerImpl) is behind the scenes.
    static TfRegistryManager manager;
    return manager;
}

void 
TfRegistryManager::RunUnloadersAtExit()
{
    Tf_RegistryManagerImpl::runUnloadersAtExit = true;
}

bool
TfRegistryManager::AddFunctionForUnload(const UnloadFunctionType& func)
{
    return Tf_RegistryManagerImpl::GetInstance().AddFunctionForUnload(func);
}

void 
TfRegistryManager::_SubscribeTo(const type_info& ti)
{
    Tf_RegistryManagerImpl::GetInstance().SubscribeTo(ArchGetDemangled(ti));
}

void
TfRegistryManager::_UnsubscribeFrom(const type_info& ti)
{
    Tf_RegistryManagerImpl::GetInstance().UnsubscribeFrom(ArchGetDemangled(ti));
}

void
Tf_RegistryInitCtor(char const *name)
{
    // Finished registering functions.
    if (TfSingleton<Tf_RegistryManagerImpl>::CurrentlyExists()) {
        Tf_RegistryManagerImpl::GetInstance().ClearActiveLibrary(name);
    }
}

void
Tf_RegistryInitDtor(char const *name)
{
    if (TfSingleton<Tf_RegistryManagerImpl>::CurrentlyExists()) {
        Tf_RegistryManagerImpl::GetInstance().UnloadLibrary(name);
    }
}

void
Tf_RegistryInit::Add(
    const char* libName,
    TfRegistryManager::RegistryFunctionType func,
    const char* typeName)
{
    // Note that we can't use _name because the instance hasn't been
    // created yet, which is why this function is static.
    Tf_RegistryManagerImpl::GetInstance().
            AddRegistryFunction(libName, func, typeName);
}

PXR_NAMESPACE_CLOSE_SCOPE
