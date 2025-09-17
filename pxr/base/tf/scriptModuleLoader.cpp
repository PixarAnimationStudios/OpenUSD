//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/scriptModuleLoader.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/debugCodes.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/pyError.h"
#include "pxr/base/tf/pyExceptionState.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/base/arch/fileSystem.h"

#include "pxr/external/boost/python/borrowed.hpp"
#include "pxr/external/boost/python/dict.hpp"
#include "pxr/external/boost/python/handle.hpp"

/*

Notes for those who venture into this dark crevice:

We build C++ libs that have python bindings into two shared libraries: 1, the
shared library with the compiled C++ code (`libFoo.so`), and 2, a shared library
with the C++/Python bindings (`_foo.so`).  We do this so that we can write pure
C++ programs that link `libFoo.so` and do not pay to load the Python bindings.
The bindings libraries (`_foo.so`) have a shared library link dependency on
their corresponding C++ library (`libFoo.so`) so whenever a bindings lib loads,
the shared library loader automatically loads the main C++ lib.

The job of the code in this file is to ensure that the Python bindings for
loaded C++ libraries are loaded whenever they are needed.

When are they needed?  There are a few scenarios.

The most obvious is when a Python module is imported.  Suppose libFoo depends on
libBar, and both have Python bindings.  If we do `import Foo` in Python, we need
to ensure that the `Bar` module is imported and registers its bindings before
Foo registers its bindings, since Foo's bindings may depend on Bar's.  For
example, if Foo wraps a class that derives a base class from Bar, that base
class must be wrapped first.  This scenario could in principle be handled by
manually writing `import Bar` in Foo's __init__.py.  But, that is prone both to
being forgotten, and to going undetected if Bar happens to be already loaded in
most of the scenarios when Foo is imported.  And, as we'll see, that doesn't
solve the other main scenario.

The other main scenario is when a C++ program initializes Python at some point
during its execution.  In this case all currently loaded C++ libraries that have
Python bindings must load them immediately.  Further, any C++ library that loads
in the future (via dlopen()) must also immediately import its bindings at load
time.  This might sound surprising.  Why should we need to eagerly import all
bindings like this?  Surely if Python code needs to use the bindings it will
`import` those modules itself, no?  Yes this is true, but there can be hidden,
indirect, undetectable dependencies that this does not handle due to
type-erasure.

Consider a type-erased opaque 'box' container, analogous to `std::any`.  Let's
call it `Any`.  When an Any is converted to Python, it unboxes its held object
and converts that object to Python.  So for example if an Any holding a string
is returned to Python, Python gets a string.  An Any holding a double unboxes as
a Python float.  An Any holding a FooObject unboxes it and uses its bindings to
convert it to Python.

Now suppose we have libFoo and libBar.  Bar has an Any-valued variable, and
provides a Python binding to access it: `Bar.GetAny()`.  Foo depends on Bar, and
stores an Any holding a FooObject to Bar's Any.  Now suppose a Python script
does `import Bar; Bar.GetAny()`.  The call to `Bar.GetAny()` will fail unless
the bindings for the held FooObject have been imported.

Whose responsibility is it to ensure they have been imported?  Bar cannot know
to import them, since Bar does not depend on Foo -- it has no knowledge of Foo's
existence.  The Python code that imported Bar similarly cannot have done it --
there's no way for it to know ahead of time what type of object is stored in the
Any, and it may not know of Foo's existence either.  The answer is that the
_only_ place that we can know for sure the bindings are needed is at the very
moment that a FooObject is requested to be converted to Python.  Unfortunately,
we do not have a hook available for this today.  It might be possible to build
one now that we have our own copy of boost.python interred, but that's not the
world we currently live in.

In the meantime, we must do as we said above and load _all_ bindings when Python
is initialized, and then continue to load any bindings for any later-loaded C++
libraries when they are loaded.

The Mechanism:

We build a small bit of initialization code into each C++ library with bindings
to advertise themselves to the TfScriptModuleLoader.  These are the
`moduleDeps.cpp` files in `pxr/...`.  The initializer calls
TfScriptModuleLoader::RegisterLibrary(), passing the lib name, its python module
name (suitable for `import`), and a list of the lib's direct dependencies.  This
way the the loader knows both which libraries have bindings, and what they
directly depend on.

Then, in a few key places, code calls LoadModules() or LoadModulesForLibrary()
to achieve the goals above.

- Tf_PyInitWrapModule(), in tf/pyModule.cpp calls LoadModulesForLibrary().  This
  function is called by Python when a bindings library is imported.  This covers
  the "import" scenario above.

- TfPyInitialize(), in tf/pyInterpreter.cpp calls LoadModules() to ensure that
  all bindings are loaded if a C++ program starts up the Python interpreter at
  some point.

- TfDlopen(), in tf/dl.cpp calls LoadModules() to ensure that all bindings are
  loaded for any newly loaded C++ libraries if Python is already initialized.

*/

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(TfScriptModuleLoader);

using std::pair;
using std::string;
using std::vector;

using pxr_boost::python::borrowed;
using pxr_boost::python::dict;
using pxr_boost::python::handle;
using pxr_boost::python::object;

TfScriptModuleLoader::TfScriptModuleLoader() = default;
TfScriptModuleLoader::~TfScriptModuleLoader() = default;

void
TfScriptModuleLoader::
RegisterLibrary(TfToken const &lib, TfToken const &moduleName,
                vector<TfToken> const &predecessors)
{
    if (TfDebug::IsEnabled(TF_SCRIPT_MODULE_LOADER)) {
        TF_DEBUG(TF_SCRIPT_MODULE_LOADER)
            .Msg("SML: Registering lib %s with %spredecessors%s%s\n",
                 lib.GetText(),
                 predecessors.empty() ? "no " : "",
                 predecessors.empty() ? "" : " ",
                 TfStringJoin(predecessors.begin(),
                              predecessors.end(), ", ").c_str());
    }

    vector<TfToken> mutablePreds = predecessors;
    std::sort(mutablePreds.begin(), mutablePreds.end());
    
    // Add library with predecessors.
    TfSpinRWMutex::ScopedLock lock(_mutex);
    bool success = _libInfo.emplace(
        std::piecewise_construct,
        std::make_tuple(lib),
        std::make_tuple(moduleName, std::move(mutablePreds))).second;
    lock.Release();

    if (!success) {
        TF_WARN("Library %s (with module '%s') already registered, repeated "
                "registration ignored", lib.GetText(), moduleName.GetText());
    }
}

dict
TfScriptModuleLoader::GetModulesDict() const
{
    if (!TfPyIsInitialized()) {
        TF_CODING_ERROR("Python is not initialized.");
        return dict();
    }

    // Subscribe to the registry function so any loaded libraries with script
    // bindings publish to this singleton.
    TfRegistryManager::GetInstance().SubscribeTo<TfScriptModuleLoader>();

    // Collect the libs & module names, then release the lock.
    TfSpinRWMutex::ScopedLock lock(_mutex, /*write=*/false);
    // Make paired lib & module names.
    vector<pair<TfToken, TfToken>> libAndModNames;
    libAndModNames.reserve(_libInfo.size());
    for (auto const &[lib, info]: _libInfo) {
        libAndModNames.emplace_back(lib, info.moduleName);
    }
    lock.Release();

    // Take the python lock and build a dict.
    TfPyLock pyLock;

    // Get the sys.modules dict from python, so we can see if modules are
    // already loaded.
    dict modulesDict(handle<>(borrowed(PyImport_GetModuleDict())));
    dict ret;

    for (auto const &[lib, mod]: libAndModNames) {
        if (modulesDict.has_key(mod.GetText())) {
            handle<> modHandle(PyImport_ImportModule(mod.GetText()));

            // Use the upper-cased form of the library name as
            // the Python module name.
            //
            // XXX This does not seem quite right.  For one thing,
            //     we should be using the Python module names here,
            //     not the C++ library names. However, after the
            //     shared_code unification, some Python modules
            //     are now submodules ("Tf" becomes "pixar.Tf").
            //     To preserve compatibility with eval'ing reprs
            //     of python types via this function, we have two
            //     options:
            //
            //     1) strip the new "pixar." prefix off module names
            //     2) upper-case the library name (tf -> Tf)
            //
            //     Also, neither of these two options correctly
            //     handles prefixed submodules.  This will provide a
            //     binding as "PrestopkgFoo", when really we
            //     should have a binding to the "Prestopkg" module
            //     with "Foo" imported underneath.
            //
            // For now, we just upper-case the library name.
            //
            ret[TfStringCapitalize(lib.GetString())] = object(modHandle);
        }
    }
    return ret;
}

void
TfScriptModuleLoader::WriteDotFile(string const &file) const
{
    FILE *out = ArchOpenFile(file.c_str(), "w");
    if (!out) {
        TF_RUNTIME_ERROR("Could not open '%s' for writing.\n", file.c_str());
        return;
    }
    fprintf(out, "digraph Modules {\n");
    TfSpinRWMutex::ScopedLock lock(_mutex, /*write=*/false);
    for (auto const &[lib, info]: _libInfo) {
        for (TfToken const &pred: info.predecessors) {
            fprintf(out, "\t%s -> %s;\n", lib.GetText(), pred.GetText());
        }
    }
    fprintf(out, "}\n");
    fclose(out);
}

void
TfScriptModuleLoader::LoadModules()
{
    // Do nothing if Python is not initialized.
    if (!TfPyIsInitialized()) {
        return;
    }
    
    // Subscribe to the registry function so any loaded libraries with script
    // bindings publish to this singleton.
    TfRegistryManager::GetInstance().SubscribeTo<TfScriptModuleLoader>();

    TF_DEBUG(TF_SCRIPT_MODULE_LOADER).Msg("SML: Begin loading all modules\n");

    // Take the lock, then collect all the modules that aren't yet loaded into a
    // vector.  Then release the lock and call _LoadLibModules() to do the work.
    TfSpinRWMutex::ScopedLock lock(_mutex);
    vector<_LibAndInfo const *> toLoad;
    for (auto iter = _libInfo.begin(), end = _libInfo.end();
         iter != end; ++iter) {
        if (!iter->second.isLoaded) {
            toLoad.push_back(std::addressof(*iter));
        }
        else {
            TF_DEBUG(TF_SCRIPT_MODULE_LOADER_EXTRA)
                .Msg("SML: Skipping already-loaded %s\n",
                     iter->first.GetText());
        }
    }
    lock.Release();

    // Sort modules by lib name to provide a consistent load order.  This isn't
    // required for correctness but eliminates a source of nondeterminism.
    std::sort(toLoad.begin(), toLoad.end(),
              [](_LibAndInfo const *l, _LibAndInfo const *r) {
                  return l->first < r->first;
              });
    _LoadLibModules(toLoad);

    TF_DEBUG(TF_SCRIPT_MODULE_LOADER).Msg("SML: End loading all modules\n");
}

void
TfScriptModuleLoader
::_LoadLibModules(vector<_LibAndInfo const *> const &toLoad) const
{
    if (toLoad.empty()) {
        return;
    }
    
    TfPyLock pyLock;
    
    for (_LibAndInfo const *libAndInfo: toLoad) {
        auto const &[lib, info] = *libAndInfo;

        if (info.moduleName.IsEmpty()) {
            TF_DEBUG(TF_SCRIPT_MODULE_LOADER)
                .Msg("SML: Not loading unknown module for lib %s\n",
                     lib.GetText());
            continue;
        }
        if (info.isLoaded) {
            TF_DEBUG(TF_SCRIPT_MODULE_LOADER_EXTRA)
                .Msg("SML: Lib %s's module '%s' is already loaded\n",
                     lib.GetText(), info.moduleName.GetText());
            continue;
        }
        
        TF_DEBUG(TF_SCRIPT_MODULE_LOADER)
            .Msg("SML: Loading lib %s's module '%s'\n",
                 lib.GetText(), info.moduleName.GetText());

        // Try to load the module.
        if (!PyImport_ImportModule(info.moduleName.GetText())) {
            // If an error occurred, warn about it with the python traceback,
            // and continue.
            TF_DEBUG(TF_SCRIPT_MODULE_LOADER)
                .Msg("SML: Error loading lib %s's module '%s'\n",
                     lib.GetText(), info.moduleName.GetText());
            TfPyExceptionState exc = TfPyExceptionState::Fetch();
            string traceback = exc.GetExceptionString();
            TF_WARN("Error loading lib %s's module '%s':\n%s",
                    lib.GetText(), info.moduleName.GetText(),
                    traceback.c_str());
        }
        
        // Mark the module loaded, even if there was an error.  Otherwise we'll
        // keep trying to load it and keep generating the same error.
        info.isLoaded = true;
    }
}

TfScriptModuleLoader::_LibInfo const *
TfScriptModuleLoader::_FindInfo(TfToken const &lib) const
{
    auto iter = _libInfo.find(lib);
    return iter != _libInfo.end() ? &iter->second : nullptr;
}

void
TfScriptModuleLoader::LoadModulesForLibrary(TfToken const &lib)
{
    // Do nothing if Python is not running.
    if (!TfPyIsInitialized()) {
        return;
    }

    // Special-case calling LoadModulesForLibrary with empty token means all.
    if (lib.IsEmpty()) {
        TF_DEBUG(TF_SCRIPT_MODULE_LOADER).Msg(
            "SML: Request to load modules for empty lib name -> load all\n");
        return LoadModules();
    }

    // Subscribe to the registry function so any loaded libraries with script
    // bindings publish to this singleton.
    TfRegistryManager::GetInstance().SubscribeTo<TfScriptModuleLoader>();
    
    // We only load direct dependencies, since when we run the initializer for
    // the python bindings lib, it will call back into here to load _its_
    // dependencies, and we get the transitive loading that way.

    TF_DEBUG(TF_SCRIPT_MODULE_LOADER).Msg(
        "SML: Begin loading %s's predecessors\n", lib.GetText());

    // Take the lock, then collect all the modules that this lib directly
    // depends on that aren't already loaded, add them to `toLoad`, then release
    // the lock and load all the modules.
    TfSpinRWMutex::ScopedLock lock(_mutex);
    vector<_LibAndInfo const *> toLoad;
    if (_LibInfo const *libInfo = _FindInfo(lib)) {
        for (TfToken const &pred: libInfo->predecessors) {
            auto iter = _libInfo.find(pred);
            if (iter != _libInfo.end() && !iter->second.isLoaded) {
                toLoad.push_back(std::addressof(*iter));
            }
        }
    }
    lock.Release();
    
    _LoadLibModules(toLoad);

    TF_DEBUG(TF_SCRIPT_MODULE_LOADER).Msg(
        "SML: End loading %s's predecessors\n", lib.GetText());
}

PXR_NAMESPACE_CLOSE_SCOPE
