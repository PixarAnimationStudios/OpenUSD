//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/debugCodes.h"
#include "pxr/base/tf/debugNotice.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/span.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/export.h"

#include <tbb/spin_mutex.h>

#include <atomic>
#include <algorithm>
#include <map>
#include <set>

PXR_NAMESPACE_OPEN_SCOPE

static std::atomic<FILE *> &
_GetOutputFile()
{
    static std::atomic<FILE *> _outputFile {
        TfGetenv("TF_DEBUG_OUTPUT_FILE") == "stderr" ? stderr : stdout
    };
    return _outputFile;
}

static std::atomic<bool> _initialized { false };

static const char* _helpMsg =
"Valid options for the TF_DEBUG environment variable are:\n"
"\n"
"      help               display this help message and exit\n"
"      SYM1 [... SYMn]    enable SYM1 through SYMn for debugging\n"
"\n"
"To disable a symbol for debugging, prepend a '-'; to match all symbols\n"
"beginning with a prefix, use 'PREFIX*' (this is the only matching supported).\n"
"Note that the order of processing matters.  For example, setting TF_DEBUG to\n"
"\n"
"      STAF_* SIC_* -SIC_REGISTRY_ENUMS GPT_IK\n"
"\n"
"enables debugging for any symbol in STAF, all symbols in SIC except for\n"
"SIC_REGISTRY_ENUMS and the symbol GPT_IK.\n";

namespace {
struct _CheckResult {
    bool matched = false;
    bool enabled = false;
};
}

static _CheckResult
_CheckSymbolAgainstPatterns(char const *enumName,
                            TfSpan<const std::string> patterns)
{
    _CheckResult result;

    for (std::string pattern: patterns) {
        if (pattern.empty()) {
            continue;
        }

        bool value = true;
        
        if (pattern[0] == '-') {
            pattern.erase(0,1);
            value = false;
        }

        if (pattern.empty()) {
            continue;
        }

        if (pattern[pattern.size() - 1] == '*') {
            pattern.erase(pattern.size() - 1);      // remove trailing '*'
            if (strncmp(enumName, pattern.c_str(), pattern.size()) == 0) {
                result.matched = true;
                result.enabled = value;
            }
        }
        else if (strcmp(pattern.c_str(), enumName) == 0) {
            result.matched = true;
            result.enabled = value;
        }
    }

    return result;
}

class ARCH_HIDDEN Tf_DebugSymbolRegistry {

    Tf_DebugSymbolRegistry(Tf_DebugSymbolRegistry const &) = delete;
    Tf_DebugSymbolRegistry &operator=(Tf_DebugSymbolRegistry const &) = delete;

public:
    static Tf_DebugSymbolRegistry& _GetInstance() {
        return TfSingleton<Tf_DebugSymbolRegistry>::GetInstance();
    }

    void _Register(const std::string& name, TfDebug::_Node* symbolAddr,
                   const std::string& description) {
        TF_DEBUG(TF_DEBUG_REGISTRY).Msg(
            "%s: %s\n", TF_FUNC_NAME().c_str(), name.c_str());

        tbb::spin_mutex::scoped_lock lock(_tableMutex);
        if (!_registeredNames.emplace(name, description).second) {
            lock.release();
            TF_FATAL_ERROR(
                "[TF_DEBUG_ENVIRONMENT_SYMBOL] multiple debug symbol "
                "definitions for '%s'.  This is usually due to software "
                "misconfiguration, such as multiple versions of the same "
                "shared library loaded simultaneously in the process.  "
                "Please check your build configuration.", name.c_str());
        }
        
        // Ensure that the symbol address is known.  We don't need to check
        // enabled state here and push it out; we can wait until it's needed.
        bool inserted = _namesToNodes[name].insert(symbolAddr).second;

        if (inserted && _initialized) {
            lock.release();
            // Even if the debug registry is initialized, our notice type may
            // not yet be defined with TfType.  This happens when we're loading
            // tf itself.  In that case, just skip notifying about changed debug
            // symbols, since we'll get a fatal error from TfNotice, and nobody
            // can really be depending on detecting changes to debug symbols
            // anyway.
            if (TfType::Find<TfDebugSymbolsChangedNotice>()) {
                TfDebugSymbolsChangedNotice().Send();
            }
        }
    }

    void _InitializeNode(TfDebug::_Node &node, char const *name) {
        // Ensure that node is known.  Then determine its official enabled
        // state, and store that state in all known nodes.
        tbb::spin_mutex::scoped_lock lock(_tableMutex);
        auto &nodesForName = _namesToNodes[name];
        nodesForName.insert(&node);
        bool enabled = _GetEnabledStateNoLock(name);
        for (TfDebug::_Node *n: nodesForName) {
            TfDebug::_NodeState nodeState =
                enabled ? TfDebug::_NodeEnabled : TfDebug::_NodeDisabled;
            n->state.store(nodeState);
        }
    }

    bool _GetEnabledStateNoLock(char const *name) const {
        // First check explicit enable/disable state.  If none has been set fall
        // back to the environment.
        auto iter = _namesToExplicitEnabledState.find(name);
        if (iter != _namesToExplicitEnabledState.end()) {
            return iter->second;
        }
        return _CheckSymbolAgainstPatterns(
            name, TfSpan<const std::string>(_envTokens)).enabled;
    }

    void _SetByPattern(std::string pattern, std::vector<std::string>* matches) {
        TF_DEBUG(TF_DEBUG_REGISTRY).Msg(
            TF_FUNC_NAME() + ": pattern = " + pattern + "\n");

        if (pattern.empty() || pattern == "-")
            return;

        bool changed = false;
        TfSpan<std::string> patterns(&pattern, 1);

        tbb::spin_mutex::scoped_lock lock(_tableMutex);
        // Go through all of _namesToNodes, looking for matches.  If we match
        // anything, set all the nodes accordingly, and also store the explicit
        // state into _namesToExplicitEnabledState.
        for (auto &nameAndNodes: _namesToNodes) {
            _CheckResult check = _CheckSymbolAgainstPatterns(
                nameAndNodes.first.c_str(), patterns);
            if (check.matched) {
                changed = true;
                if (matches) {
                    matches->push_back(nameAndNodes.first);
                }
                _namesToExplicitEnabledState[nameAndNodes.first] = check.enabled;
                TfDebug::_NodeState nodeState = check.enabled ?
                    TfDebug::_NodeEnabled : TfDebug::_NodeDisabled;
                for (TfDebug::_Node *n: nameAndNodes.second) {
                    n->state.store(nodeState);
                }
                TF_DEBUG(TF_DEBUG_REGISTRY).Msg(
                    "%s: set %s %s\n", TF_FUNC_NAME().c_str(),
                    nameAndNodes.first.c_str(),
                    check.enabled ? "true" : "false");
            }
        }
        
        if (changed && _initialized) {
            lock.release();
            TfDebugSymbolsChangedNotice().Send();
        }
    }

    void _SetByName(TfDebug::_Node &node,
                    char const *name, bool enabled) {
        tbb::spin_mutex::scoped_lock lock(_tableMutex);
        // Ensure node is known.  Then determine its official enabled state, and
        // store that state in all known nodes.
        auto &nodesForName = _namesToNodes[name];
        nodesForName.insert(&node);
        for (TfDebug::_Node *n: nodesForName) {
            TfDebug::_NodeState nodeState =
                enabled ? TfDebug::_NodeEnabled : TfDebug::_NodeDisabled;
            n->state.store(nodeState);
        }
        // Store the state in the _namesToExplicitEnabledState.
        _namesToExplicitEnabledState[name] = enabled;

        if (_initialized) {
            lock.release();
            TfDebugSymbolsChangedNotice().Send();
        }
    }

    bool _IsEnabled(const std::string& name) const {
        tbb::spin_mutex::scoped_lock lock(_tableMutex);
        return _GetEnabledStateNoLock(name.c_str());
    }

    std::string _GetDescriptions() const {
        std::string result;
        tbb::spin_mutex::scoped_lock lock(_tableMutex);

        for (auto const &nameAndDescr: _registeredNames) {
            if (nameAndDescr.first.size() < 25) {
                std::string padding(25 - nameAndDescr.first.size(), ' ');
                result += TfStringPrintf(
                    "%s%s: %s\n", nameAndDescr.first.c_str(),
                    padding.c_str(), nameAndDescr.second.c_str());
            }
            else {
                result += TfStringPrintf(
                    "%s:\n%s  %s\n", nameAndDescr.first.c_str(),
                    std::string(25, ' ').c_str(), nameAndDescr.second.c_str());
            }
        }
        return result;
    }

    std::vector<std::string> _GetSymbolNames() const {
        tbb::spin_mutex::scoped_lock lock(_tableMutex);
        std::vector<std::string> result;
        result.reserve(_namesToNodes.size());
        for (auto const &p: _namesToNodes) {
            result.push_back(p.first);
        }
        return result;
    }

    std::string _GetDescription(const std::string& name) const {
        tbb::spin_mutex::scoped_lock lock(_tableMutex);
        auto iter = _registeredNames.find(name);
        if (iter == _registeredNames.end()) {
            return std::string();
        }
        return iter->second;
    }

  private:
    Tf_DebugSymbolRegistry() {
        _envTokens = TfStringTokenize(TfGetenv("TF_DEBUG"));

        if (std::find(_envTokens.begin(),
                      _envTokens.end(), "help") != _envTokens.end()) {
            printf("%s", _helpMsg);
            exit(0);
        }

        TfSingleton<Tf_DebugSymbolRegistry>::SetInstanceConstructed(*this);

#define _ADD(symbol, descrip)                                           \
        TfDebug::_RegisterDebugSymbol(symbol, #symbol, descrip);

	_ADD(TF_DEBUG_REGISTRY, "debug the TfDebug registry");
	_ADD(TF_DISCOVERY_TERSE, "coarse grain debugging of TfRegistryManager");
	_ADD(TF_DISCOVERY_DETAILED, "detailed debugging of TfRegistryManager");
        _ADD(TF_DLOPEN, "show files opened by TfDlopen");
        _ADD(TF_DLCLOSE, "show files closed by TfDlclose");

#undef _ADD
	
        TF_DEBUG(TF_DEBUG_REGISTRY).Msg(TF_FUNC_NAME() + "\n");

        _initialized = true;

        TfRegistryManager::GetInstance().SubscribeTo<TfDebug>();
    }
    
    ~Tf_DebugSymbolRegistry() {
        TF_DEBUG(TF_DEBUG_REGISTRY).Msg(TF_FUNC_NAME() + "\n");
        TfRegistryManager::GetInstance().UnsubscribeFrom<TfDebug>();
    }

    // A mapping of string code names to a vector of currently known node
    // instances.  This is the official list of "known" debug symbols.  Note
    // that _RegisteredNames and _NamesToEnabledState are commonly subsets of
    // this.
    using _NamesToNodes = std::map<std::string, std::set<TfDebug::_Node *>>;
    
    // A mapping of registered debug codes by name to description.  This also
    // lets us catch multiple registrations via TF_DEBUG_ENVIRONMENT_SYMBOL.
    // This often indicates "versionitis" build problems where multiple copies
    // of different shared libraries are loaded simultaneously.
    using _RegisteredNames = std::map<std::string, std::string>;

    // A mapping from string code names to official enabled state.  _Node
    // instances are caches of these states.  Once _Node instances are used
    // (i.e. initialized) the are stored in _NamesToNodes, and any changes to
    // code enabled states get propagated to all known instances.
    using _NamesToExplicitEnabledState = std::map<std::string, bool>;

    mutable tbb::spin_mutex _tableMutex;
    _NamesToExplicitEnabledState _namesToExplicitEnabledState;
    _NamesToNodes _namesToNodes;
    _RegisteredNames _registeredNames;
    std::vector<std::string> _envTokens;
    
    friend class TfSingleton<Tf_DebugSymbolRegistry>;
}; 
TF_INSTANTIATE_SINGLETON(Tf_DebugSymbolRegistry);

std::vector<std::string>
TfDebug::SetDebugSymbolsByName(const std::string& pattern, bool value)
{
    std::vector<std::string> matches;
    Tf_DebugSymbolRegistry::_GetInstance().
        _SetByPattern(std::string(value ? "" : "-") + pattern, &matches);
    return matches;
}

bool
TfDebug::IsDebugSymbolNameEnabled(const std::string& name)
{
    return Tf_DebugSymbolRegistry::_GetInstance()._IsEnabled(name);
}

std::string
TfDebug::GetDebugSymbolDescriptions()
{
    return Tf_DebugSymbolRegistry::_GetInstance()._GetDescriptions();
}

std::vector<std::string>
TfDebug::GetDebugSymbolNames()
{
    return Tf_DebugSymbolRegistry::_GetInstance()._GetSymbolNames();
}

std::string
TfDebug::GetDebugSymbolDescription(const std::string& name)
{
    return Tf_DebugSymbolRegistry::_GetInstance()._GetDescription(name);
}

void
TfDebug::SetOutputFile(FILE *file)
{
    if (file == stdout || file == stderr) {
        _GetOutputFile().store(file);
    } else {
        TF_CODING_ERROR("TfDebug output must go to either stdout or stderr");
    }
}

void
TfDebug::_SetNode(_Node &node, char const *name, bool state)
{
    Tf_DebugSymbolRegistry::_GetInstance()._SetByName(node, name, state);
}

void
TfDebug::_InitializeNode(_Node &node, char const *name)
{
    Tf_DebugSymbolRegistry::_GetInstance()._InitializeNode(node, name);
}    

void
TfDebug::Helper::Msg(const std::string& msg)
{
    FILE *outputFile = _GetOutputFile().load();
    fprintf(outputFile, "%s", msg.c_str());
    fflush(outputFile);
}

void
TfDebug::Helper::Msg(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    Msg(TfVStringPrintf(fmt, ap));
    va_end(ap);
}

void
TfDebug::_ScopedOutput(bool start, const char* str)
{
    /*
     * For multi-threading, you could have each thread keep its
     * own stackDepth.  But if you're going to mix these prints together,
     * you're going to have a mess.  So we'll just do the simple thing
     * of using a global counter, but increment/decrement atomically.
     * If it becomes necessary, we can make the counter per thread,
     * but I doubt it'll be required.
     */
    static std::atomic<int> stackDepth(0);

    FILE *outputFile = _GetOutputFile().load();
    
    if (start) {
        fprintf(outputFile, "%*s%s --{\n", 2 * stackDepth, "", str);
        ++stackDepth;
    }
    else {
        --stackDepth;
        fprintf(outputFile, "%*s}-- %s\n", 2 * stackDepth, "", str);
    }
}

// This is the code path used by TF_DEBUG_ENVIRONMENT_SYMBOL.
void
TfDebug::_RegisterDebugSymbolImpl(_Node *addr, char const *enumNameCstr,
                                  char const *descrip)
{
    std::string enumName = enumNameCstr;

    if (!descrip) {
        TF_FATAL_ERROR("description argument for '%s' "
                       "is NULL", enumName.c_str());
    }
    else if (descrip[0] == '\0') {
        TF_FATAL_ERROR("description argument for '%s' is empty -- "
                       "add description!",  enumName.c_str());
    }

    Tf_DebugSymbolRegistry::_GetInstance()._Register(enumName, addr, descrip);
}

void
TfDebug::_ComplainAboutInvalidSymbol(const char* name)
{
    TF_CODING_ERROR("TF_DEBUG_ENVIRONMENT_SYMBOL(): symbol '%s' "
                    "invalid.  (Check the TF_DEBUG_CODES() macro.)", name);
}

template <bool B>
TfDebug::TimedScopeHelper<B>::TimedScopeHelper(
    bool enabled, const char* fmt, ...)
{
    active = enabled;
    if (active) {
        va_list ap;
        va_start(ap, fmt);
        str = TfVStringPrintf(fmt, ap);
        va_end(ap);

        TfDebug::_ScopedOutput(/* start = */ true, str.c_str());
        stopwatch.Start();
    }
}

template <bool B>
TfDebug::TimedScopeHelper<B>::~TimedScopeHelper()
{
    if (active) {
        stopwatch.Stop();

        const std::string endStr = 
            TfStringPrintf("%s: %.3f ms", str.c_str(), 
                           stopwatch.GetSeconds() * 1000.0);
        TfDebug::_ScopedOutput(/* start = */ false, endStr.c_str());
    }
}

template struct TfDebug::TimedScopeHelper<true>;
    
/*
 * Scan the environment variable TF_DEBUG for debug symbols.
 *
 * Calling this routine causes the environment variable TF_DEBUG to be split
 * into white-space separated strings, and each such string is used to
 * possibly set some number of debug symbols that have been registered via
 * the \c TF_DEBUG_ENVIRONMENT_SYMBOL() macro.  A limited form of wild-card
 * matching is supported, in which a string ending with a '*' will match any
 * debug symbol beginning with that string.  A preceding '-' means that the
 * debug symbol is turned off.
 *
 * For example, setting TF_DEBUG to
 * \code
 *     setenv TF_DEBUG "TM_TRANSACTION_MANAGER"
 * \endcode
 * enables the single symbol \c TM_TRANSACTION_MANAGER.  Setting
 * \code
 *     setenv TF_DEBUG "STAF_* SIC_* -SIC_REGISTRY_ENUMS"
 * \endcode
 * enables debugging for any symbol whose name starts with \c STAF, all
 * symbols in SIC except for \c SIC_REGISTRY_ENUMS and the symbol GPT_IK.
 *
 * Finally, setting TF_DEBUG to "help" prints a help message and the program
 * exits immediately after.
 *
 * Since environment variables are assumed not to change during program
 * execution, only the first call to this function has any effect.
 */
ARCH_HIDDEN
void
Tf_DebugInitFromEnvironment()
{
    // Simply creating the registry forces the initialization we need.
    (void) Tf_DebugSymbolRegistry::_GetInstance();
}

PXR_NAMESPACE_CLOSE_SCOPE
