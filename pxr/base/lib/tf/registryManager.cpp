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
// The registry manager is used to lazily run code needed by consumers of
// some type.  This code is found automatically by the registry manager
// and executed when the consumer "subscribes to" the type and when loading
// a library with subscribed to types.
//
// Finding the code automatically requires some magic.  What we want to do
// is add function/type pairs to the registry.  When a client "subscribes"
// to a type, all registered functions for that type should be executed in
// the order they were added (at least to the extent that all functions in
// a shared library are executed before any registered functions in any
// shared library that depends on it).
//
// To achieve this under GCC/clang we use the __attribute__ extension
// constructor(priority).  This allows us to specify that a function must
// be executed when the shared library (or main program) is loaded.  The
// dynamic loader will ensure that all construtors in a library are run
// before running any code in another library that depends on it (i.e. it
// does a topological sort of library dependencies and fully initializes
// each library before moving on to the next).
//
// Our TF_REGISTRY_FUNCTION() macro will cause a function to be emitted
// that registers the user's function.  This function will have the
// attributes in the TF_REGISTRY_ADD_ATTRIBUTES macro which will cause
// the function to get run when the library is loaded.
//
// For performance reasons, we'd like the above functions to be collected
// before running subscriptions.  To do that we arrange to run a function
// after all functions with TF_REGISTRY_ADD_ATTRIBUTES.  Note that this
// is just for performance:  the topological sort ensures the correctness
// of the order.  The TF_REGISTRY_LOAD_MARKER macro is used to make this
// happen.
//
// Finally, clients can register functions to run when the library is
// unloaded.  We need to detect when a library is unloaded.  The
// TF_REGISTRY_LOAD_MARKER also does this.
//
// Issues:
//
// We'd like to process all functions in a library all at once.  That
// requires exactly one object with TF_REGISTRY_LOAD_ATTRIBUTES per
// library and that it's constructor runs after all functions with
// TF_REGISTRY_ADD_ATTRIBUTES.  Unfortunately this doesn't seem to be
// possible.  Here are the confounding issues:
//
//   1) Static functions in templates with __attribute__((constructor))
//      are run separately from those not in templates.
//      
//   2) Static functions in templates with __attribute__((constructor))
//      are run separately for each template in an object file and
//      object files are processed in the order they appear on the
//      link line.
//
// These two issues mean there is no __attribute__((constructor)) priority
// that we can use to run after all other priorities (even though this is
// what the documentation suggests).  However:
//
//   3) Normal C++ constructors for objects at global scope run after
//      all __attribute__((constructor)) functions for each object file
//      and object files are processed in the order they appear on the
//      link line.
//
// This means TF_REGISTRY_LOAD_MARKER can be an instantiation of a object
// with a C++ constructor that processes registered functions.  Ideally
// we'd have exactly one of these per library, but:
//
//   4) The C++ constructor of an object with __attribute__((weak)) will
//      run with the first object file on the link line that contains
//      an instance.
//
// That means the constructor will run before constructor attribute
// functions in templates in object files later on the link line.  So
// we can't use __attribute__((weak)) to get exactly one instance.
//
// There are two obvious solutions:
//
//   1) Ignore it and allow multiple load markers.  This may have a
//      performance impact.
//
//   2) Require that any shared library or executable program with one
//      or more registry functions (locally, not transitively) put a
//      particular object file last on the link line.
//
// We choose (1) since it doesn't require any extra work on the coder's
// part and (2) is too easy to forget.  We simply try to mitigate the
// extra work:  _ProcessLibraryNoLock() does nothing if no functions
// have been registered and _UpdateSubscribersNoLock() is skipped if no
// registered functions have subscribed-to types.
//
// The TF_REGISTRY_LOAD_MARKER object's destructor is used to detect the
// unload of a library.  We simply ignore unloads on unloaded libraries.
// This means we unload when the first instance is destroyed.  We assume
// the unload code is safe to call at this time.
//

#include <boost/foreach.hpp>
#include <boost/function.hpp>

#include "pxr/base/arch/defines.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/debugCodes.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/dl.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/instantiateSingleton.h"
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

namespace {

// Convenience for moving the contents of one list to the front of another.
template <class L>
void
MoveToFront(L& dst, L& src)
{
    dst.splice(dst.begin(), src);
}

// Convenience for moving the contents of one list to the back of another.
template <class L>
void
MoveToBack(L& dst, L& src)
{
    dst.splice(dst.end(), src);
}

std::string
GetLibraryPath(const char* libraryName,
               TfRegistryManager::RegistrationFunctionType func)
{
    std::string result = libraryName;
    ArchGetAddressInfo(reinterpret_cast<void*>(func), &result, NULL, NULL,NULL);
    return result;
}

class Tf_RegistryManagerImpl : boost::noncopyable {
public:
    typedef int LibraryIdentifier;

    typedef TfRegistryManager::RegistrationFunctionType RegistrationFunction;
    typedef TfRegistryManager::UnloadFunctionType UnloadFunction;

    static Tf_RegistryManagerImpl& GetInstance() {
         return TfSingleton<Tf_RegistryManagerImpl>::GetInstance();
    }

    /// Stores the active library's registration functions and runs those
    /// that are subscribed to then makes no library active. Returns an
    /// identifier that can be passed to \c UnloadLibrary().
    void ClearActiveLibrary(const char* libraryName);

    /// Adds a registration function for the library.  Reports an error
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
    void AddRegistrationFunction(const char* libraryName,
                                 RegistrationFunction,
                                 const char* typeName);

    /// Adds a function for unload for the library associated with the
    /// running registry function.  Reports an error and otherwise does
    /// nothing if there is no running registry function.
    bool AddFunctionForUnload(const UnloadFunction& func);

    /// Run the unload functions for library \p libraryName if they haven't
    /// already been run.
    void UnloadLibrary(const char* libraryName);

    /// Subscribe to a type.  This causes all of the registration functions
    /// for the type to be run if they haven't run already and causes any
    /// registration functions added later for that type to run during
    /// library load.
    void SubscribeTo(const string& typeName);

    /// Unsubscribe from a type.  Any registration functions added later
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
    void _RunRegistrationFunctionsNoLock(const string& typeName);
    void _UnloadNoLock(const char* libraryName);

private:

    typedef string TypeName;
    typedef map<LibraryName, LibraryIdentifier> _LibraryNameMap;
    struct _RegistrationValue {
        _RegistrationValue(RegistrationFunction function_,
                           LibraryIdentifier identifier_) :
            function(function_), unloadKey(identifier_) { }

        RegistrationFunction function;
        LibraryIdentifier unloadKey;
    };
    typedef list<_RegistrationValue> _RegistrationValueList;
    typedef TfHashMap<TypeName, _RegistrationValueList,
                      TfHash> _RegistrationFunctionMap;
    typedef list<UnloadFunction> _UnloadFunctionList;
    typedef TfHashMap<LibraryIdentifier,
                      _UnloadFunctionList, TfHash> _UnloadFunctionMap;

    struct _ActiveLibraryState {
        _ActiveLibraryState() : identifier(0) { }

        LibraryIdentifier identifier;
        LibraryName name;
        _RegistrationFunctionMap registrationFunctions;
    };

    // Misc state.
    std::recursive_mutex _mutex;

    // Subscription state.
    _LibraryNameMap _libraryNameMap;
    set<TypeName> _subscriptions;
    list<TypeName> _orderedSubscriptions;
    _RegistrationFunctionMap _registrationFunctions;
    _UnloadFunctionMap _unloadFunctions;

    // Registration state.
    _RegistrationValueList _registrationWorklist;
    tbb::enumerable_thread_specific<_UnloadFunctionList*> _currentUnloadList;

    // Active library registration state.
    tbb::enumerable_thread_specific<_ActiveLibraryState> _active;

    friend class TfSingleton<Tf_RegistryManagerImpl>;
};

bool Tf_RegistryManagerImpl::runUnloadersAtExit = false;

Tf_RegistryManagerImpl::Tf_RegistryManagerImpl()
{
    /*
     * This is the one place we can't let TfDebug do all the work for us,
     * since TfDebug would end up calling back here.  So we do it manually.
     */

    if (TfDebug::_CheckEnvironmentForMatch("TF_DISCOVERY_TERSE"))
        TfDebug::Enable(TF_DISCOVERY_TERSE);            
    if (TfDebug::_CheckEnvironmentForMatch("TF_DISCOVERY_DETAILED"))
        TfDebug::Enable(TF_DISCOVERY_DETAILED);

    TF_DEBUG(TF_DISCOVERY_TERSE).Msg("TfRegistryManager: "
                                     "initialized\n");
}

Tf_RegistryManagerImpl::~Tf_RegistryManagerImpl()
{
    // Do nothing
}

void
Tf_RegistryManagerImpl::ClearActiveLibrary(const char* libraryName)
{
    TF_AXIOM(libraryName and libraryName[0]);

    // If the name doesn't match then libraryName has already been processed.
    if (_active.local().name == libraryName) {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        _ProcessLibraryNoLock();
    }
}

void
Tf_RegistryManagerImpl::AddRegistrationFunction(
    const char* libraryName,
    RegistrationFunction func,
    const char* typeName)
{
    if (not TF_VERIFY(libraryName and libraryName[0],
                      "TfRegistryManager: "
                      "Ignoring library with no name")) {
        return;
    }
    else if (not TF_VERIFY(typeName and typeName[0],
                      "TfRegistryManager: "
                      "Ignoring registration with no type in %s",
                      libraryName)) {
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

    if (not active.identifier) {
        TF_DEBUG(TF_DISCOVERY_TERSE).
            Msg("TfRegistryManager: "
                "Library %s\n",
                GetLibraryPath(libraryName, func).c_str());

        // Set active.
        active.name = libraryName;

        // Access shared members.
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        active.identifier = _RegisterLibraryNoLock(libraryName);
    }
    TF_AXIOM(active.identifier);

    active.registrationFunctions[typeName].
        push_back(_RegistrationValue(func, active.identifier));
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
    if (Tf_DlCloseIsActive() or runUnloadersAtExit) {
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
        _RunRegistrationFunctionsNoLock(typeName);
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
        identifier = static_cast<int>(_libraryNameMap.size());
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
    BOOST_FOREACH(const TypeName& typeName, _orderedSubscriptions) {
        _RunRegistrationFunctionsNoLock(typeName);
    }
}

bool
Tf_RegistryManagerImpl::_TransferActiveLibraryNoLock()
{
    bool movedAny = false;

    // Move active library functions to non-thread local storage type by type.
    _ActiveLibraryState& active = _active.local();
    BOOST_FOREACH(_RegistrationFunctionMap::value_type& v,
                  active.registrationFunctions) {
        if (not movedAny and not v.second.empty()) {
            movedAny = (_subscriptions.count(v.first) != 0);
        }
        MoveToBack(_registrationFunctions[v.first], v.second);
    }

    // Reset.
    active.identifier = 0;
    active.name.clear();
    active.registrationFunctions.clear();

    return movedAny;
}

void
Tf_RegistryManagerImpl::_RunRegistrationFunctionsNoLock(const string& typeName)
{
    _RegistrationFunctionMap::iterator i = _registrationFunctions.find(typeName);
    if (i == _registrationFunctions.end()) {
        TF_DEBUG(TF_DISCOVERY_TERSE).Msg("TfRegistryManager: "
                                         "no functions to run for %s\n",
                                         typeName.c_str());
        return;
    }
    
    /*
     * Transfer from registration table onto the worklist queue,
     * running *our* stuff first...
     */
    TF_DEBUG(TF_DISCOVERY_TERSE).Msg("TfRegistryManager: "
                                     "running %zd functions for %s\n",
                                     i->second.size(),
                                     typeName.c_str());
    MoveToFront(_registrationWorklist, i->second);
    TF_AXIOM(i->second.empty());

    /*
     * In case someone else is in this function at the same time (i.e. in another
     * thread, let them run functions off the global worklist as well, to prevent
     * deadlock.
     */
    while (!_registrationWorklist.empty()) {
        _RegistrationValue value = _registrationWorklist.front();
        _registrationWorklist.pop_front();

        _UnloadFunctionList* oldUnloadList = _currentUnloadList.local();
        _currentUnloadList.local() = &_unloadFunctions[value.unloadKey];

        _mutex.unlock();
        value.function(NULL, NULL);
        _mutex.lock();

        _currentUnloadList.local() = oldUnloadList;
    }
}

void 
Tf_RegistryManagerImpl::_UnloadNoLock(const char* libraryName)
{
    TF_DEBUG(TF_DISCOVERY_TERSE).Msg("TfRegistryManager: "
                                     "unloading '%s'\n", 
                                     libraryName);

    TF_AXIOM(libraryName and libraryName[0]);

    LibraryIdentifier identifier   = _RegisterLibraryNoLock(libraryName);
    _UnloadFunctionMap::iterator i = _unloadFunctions.find(identifier);
    if (i != _unloadFunctions.end()) {
        // Unload functions were registered.
        // Move the list to avoid worrying about modifications while unloading.
        _UnloadFunctionList unloadFunctions;
        i->second.swap(unloadFunctions);
        TF_AXIOM(i->second.empty());

        // Run the unload functions
        BOOST_FOREACH(const UnloadFunction& func, unloadFunctions) {
            func();
        }
    }

    /*
     * Remove any registration functions for the library.  This prevents
     * crashes where the registry manager could attempt to execute a
     * registry function from the unloaded library.
     */
    BOOST_FOREACH(_RegistrationFunctionMap::value_type& k,
                  _registrationFunctions) {
        _RegistrationValueList& regValues = k.second;
        _RegistrationValueList::iterator regValueIt = regValues.begin();
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
TfRegistryManager::AddFunctionForUnload(const boost::function<void ()>& func)
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

Tf_RegistryInit::Tf_RegistryInit(const char* name) : _name(name)
{
    // Finished registering functions.
    if (TfSingleton<Tf_RegistryManagerImpl>::CurrentlyExists()) {
        Tf_RegistryManagerImpl::GetInstance().ClearActiveLibrary(_name);
    }
}

Tf_RegistryInit::~Tf_RegistryInit()
{
    if (TfSingleton<Tf_RegistryManagerImpl>::CurrentlyExists()) {
        Tf_RegistryManagerImpl::GetInstance().UnloadLibrary(_name);
    }
}

void
Tf_RegistryInit::Add(
    const char* libName,
    TfRegistryManager::RegistrationFunctionType func,
    const char* typeName)
{
    // Note that we can't use _name because the instance hasn't been
    // created yet, which is why this function is static.
    Tf_RegistryManagerImpl::GetInstance().
            AddRegistrationFunction(libName, func, typeName);
}
