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
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/debugCodes.h"
#include "pxr/base/tf/debugNotice.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/export.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <tbb/spin_mutex.h>

#include <atomic>
#include <algorithm>
#include <map>
#include <set>

using boost::function;
using boost::bind;

using std::string;
using std::set;
using std::vector;
using std::map;


static FILE*&
_GetOutputFile()
{
    static FILE* _outputFile =
        TfGetenv("TF_DEBUG_OUTPUT_FILE") == "stderr" ? stderr : stdout;
    return _outputFile;
}

static TfStaticData<tbb::spin_mutex> _outputFileMutex;

static bool _initialized = false;

static const char* _helpMsg =
    "Valid options for the TF_DEBUG environment variable are:\n"
    "\n"
    "      help               display this help message and exit\n"
    "      list               display all registered debug symbols and exit\n"
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

class ARCH_HIDDEN Tf_DebugSymbolRegistry : boost::noncopyable {
public:
    static Tf_DebugSymbolRegistry& _GetInstance() {
        return TfSingleton<Tf_DebugSymbolRegistry>::GetInstance();
    }

    void _Add(const string& name, TfDebug::_Node* symbolAddr,
              const string& description) {
        TF_DEBUG(TF_DEBUG_REGISTRY).Msg("%s: %s\n", TF_FUNC_NAME().c_str(), name.c_str());
        _Data data = { symbolAddr, description };

        {
            tbb::spin_mutex::scoped_lock lock(_tableLock);
            if (_table.find(name) != _table.end()) {
                TF_FATAL_ERROR("[TF_DEBUG_ENVIRONMENT_SYMBOL] multiple "
                               "symbol definitions.  This is usually due to "
                               "software misconfiguration.  Contact the build "
                               "team for assistance. (duplicate '%s')",
                               name.c_str());
            }
            _table[name] = data;
        }
        
        function<void ()> fn =
            bind(&Tf_DebugSymbolRegistry::_Remove, this, name);
        TfRegistryManager::GetInstance().AddFunctionForUnload(fn);

        TF_FOR_ALL(i, _debugTokens)
            _Set(*i, &name, NULL);

        if (_initialized) {
            TfDebugSymbolsChangedNotice().Send();
        }
    }

    void _Set(string pattern, const string* singleSymbol, vector<string>* matches) {
        /*
         * If singleSymbol is NULL, it means search the entire _table.  Otherwise,
         * we restrict ourselves only to the table entry.
         */
        
        TF_DEBUG(TF_DEBUG_REGISTRY).Msg(TF_FUNC_NAME() + ": pattern = " + pattern + "\n");

        if (pattern.empty() || pattern == "-")
            return;

        bool value = true;
        if (pattern[0] == '-') {
            pattern.erase(0,1);
            value = false;
        }
        
        if (pattern.empty())
            return;

        if (pattern[pattern.size() - 1] == '*') {
            pattern.erase(pattern.size() - 1);      // remove trailing '*'
            tbb::spin_mutex::scoped_lock lock(_tableLock);

            for (_Table::iterator i =
                 _table.lower_bound(singleSymbol ? *singleSymbol : pattern);
                 i != _table.end(); ++i) {

                if (i->first.find(pattern) != 0 ||
                    (singleSymbol && i->first != *singleSymbol)) {
                    break;
                }
                TF_DEBUG(TF_DEBUG_REGISTRY).Msg(TF_FUNC_NAME() + ": set " + \
                         i->first + (value ? " true\n" : " false\n"));
                TfDebug::_SetNodes(i->second.addr, 1, value);

                if (matches)
                    matches->push_back(i->first);
            }
        }
        else {
            if (singleSymbol && pattern != *singleSymbol)
                return;
            
            tbb::spin_mutex::scoped_lock lock(_tableLock);
            _Table::iterator i = _table.find(pattern);
            if (i != _table.end()) {
                TfDebug::_SetNodes(i->second.addr, 1, value);
                TF_DEBUG(TF_DEBUG_REGISTRY).Msg(TF_FUNC_NAME() + ": set " + \
                         i->first + (value ? " true\n" : " false\n"));
                if (matches)
                    matches->push_back(i->first);
            }
        }

        if (_initialized) {
            TfDebugSymbolsChangedNotice().Send();
        }
    }

    bool _IsEnabled(const string& name) const {
        _Table::const_iterator i = _table.find(name);
        if (i != _table.end()) {
            return i->second.addr->enabled;
        } else {
            return false;
        }
    }

    string _GetDescriptions() const {
        string result;
        tbb::spin_mutex::scoped_lock lock(_tableLock);

        TF_FOR_ALL(i, _table) {
            if (i->first.size() < 25) {
                string padding(25 - i->first.size(), ' ');
                result += TfStringPrintf("%s%s: %s\n", i->first.c_str(),
                                         padding.c_str(),
                                         i->second.description.c_str());
            }
            else {
                result += TfStringPrintf("%s:\n%s  %s\n", i->first.c_str(),
                                         string(25, ' ').c_str(),
                                         i->second.description.c_str());
            }
        }
        return result;
    }

    vector<string> _GetSymbolNames() const {
        tbb::spin_mutex::scoped_lock lock(_tableLock);
        vector<string> result;
        result.reserve(_table.size());

        TF_FOR_ALL(i, _table) {
            result.push_back(i->first);
        }
        return result;
    }

    string _GetDescription(const string& name) const {
        tbb::spin_mutex::scoped_lock lock(_tableLock);
        _Table::const_iterator i = _table.find(name);
        if (i == _table.end())
            return string();
        return i->second.description;
    }

private:
    Tf_DebugSymbolRegistry() {
        _debugTokens = TfStringTokenize(TfGetenv("TF_DEBUG"));
        set<string> debugTokenSet(_debugTokens.begin(), _debugTokens.end());

        if (debugTokenSet.count("help")) {
            printf("%s", _helpMsg);
            exit(0);
        }

        TfSingleton<Tf_DebugSymbolRegistry>::SetInstanceConstructed(*this);

#define _ADD_SYM(symbol, descrip) \
        _Add(#symbol, TfDebug::_GetSymbolAddr(symbol, #symbol), descrip)

	_ADD_SYM(TF_DEBUG_REGISTRY, "debug the TfDebug registry");
	_ADD_SYM(TF_DISCOVERY_TERSE, "coarse grain debugging of TfRegistryManager");
	_ADD_SYM(TF_DISCOVERY_DETAILED, "detailed debugging of TfRegistryManager");
        _ADD_SYM(TF_DLOPEN, "show files opened by TfDlopen");
        _ADD_SYM(TF_DLCLOSE, "show files closed by TfDlclose");

#undef _ADD_SYM
	
        TF_DEBUG(TF_DEBUG_REGISTRY).Msg(TF_FUNC_NAME() + "\n");

        TfRegistryManager::GetInstance().SubscribeTo<TfDebug>();

        if (debugTokenSet.count("list")) {
            printf("%s\n", _GetDescriptions().c_str());
            exit(0);
        }

        _initialized = true;
    }
    
    ~Tf_DebugSymbolRegistry() {
        TF_DEBUG(TF_DEBUG_REGISTRY).Msg(TF_FUNC_NAME() + "\n");
        TfRegistryManager::GetInstance().UnsubscribeFrom<TfDebug>();
    }

    struct _Data {
        TfDebug::_Node* addr;
        string description;
    };

    void _Remove(const string& name) {
        TF_DEBUG(TF_DEBUG_REGISTRY).Msg("%s: %s\n",TF_FUNC_NAME().c_str(),\
                                        name.c_str());
        {
            tbb::spin_mutex::scoped_lock lock(_tableLock);
            _table.erase(name);
        }

        if (_initialized) {
            TfDebugSymbolsChangedNotice().Send();
        }
    }

    typedef map<string, _Data> _Table;

    mutable tbb::spin_mutex _tableLock;
    _Table _table;
    vector<string> _debugTokens;
    
    friend class TfSingleton<Tf_DebugSymbolRegistry>;
}; 
TF_INSTANTIATE_SINGLETON(Tf_DebugSymbolRegistry);

/*
 * Return true if enumName is in getenv("TF_DEBUG"), or if there is some
 * string in debugTokens that ends with '*' and, removing the '*' is
 * a prefix of enumName.  This is only meant to be used by TfDebug itself,
 * and the TfRegistryManager.
 */
bool
TfDebug::_CheckEnvironmentForMatch(const string& enumName)
{
    vector<string> debugTokens = TfStringTokenize(TfGetenv("TF_DEBUG"));
    bool result = false;

    TF_FOR_ALL(i, debugTokens) {
        string pattern = *i;
        if (pattern.empty())
            continue;

        bool value = true;
        
        if (pattern[0] == '-') {
            pattern.erase(0,1);
            value = false;
        }

        if (pattern.empty())
            continue;

        if (pattern[pattern.size() - 1] == '*') {
            pattern.erase(pattern.size() - 1);      // remove trailing '*'

            if (enumName.find(pattern) == 0)
                result = value;
        }
        else if (pattern == enumName)
            result = value;
    }

    return result;
}

vector<string>
TfDebug::SetDebugSymbolsByName(const string& pattern, bool value)
{
    vector<string> matches;
    Tf_DebugSymbolRegistry::_GetInstance()._Set(string(value ? "" : "-") + pattern,
                                                NULL, &matches);
    return matches;
}

bool
TfDebug::IsDebugSymbolNameEnabled(const std::string& name)
{
    return Tf_DebugSymbolRegistry::_GetInstance()._IsEnabled(name);
}

string
TfDebug::GetDebugSymbolDescriptions()
{
    return Tf_DebugSymbolRegistry::_GetInstance()._GetDescriptions();
}

vector<string>
TfDebug::GetDebugSymbolNames()
{
    return Tf_DebugSymbolRegistry::_GetInstance()._GetSymbolNames();
}

string
TfDebug::GetDebugSymbolDescription(const string& name)
{
    return Tf_DebugSymbolRegistry::_GetInstance()._GetDescription(name);
}

void
TfDebug::SetOutputFile(FILE *file)
{
    if (file == stdout or file == stderr) {
        tbb::spin_mutex::scoped_lock lock(*_outputFileMutex);
        _GetOutputFile() = file;
    } else {
        TF_CODING_ERROR("TfDebug output must go to either stdout or stderr");
    }
}

void
TfDebug::_SetNodes(_Node* ptr, size_t n, bool state)
{
    for (size_t i = 0; i < n; i++) {
        (ptr+i)->enabled = state;
    }

    // If we're setting only one node, with children, we recurse:

    if (n == 1 && ptr->children) {
        for (size_t i = 0; i < ptr->children->size(); i++)
            _SetNodes((*ptr->children)[i], 1, state);
    }
}

void
TfDebug::_SetParentChild(_Node* parent, _Node* child)
{
    if (child->children)
        TF_FATAL_ERROR("cannot set parent/child relationship after "
                       "child node has been given children itself");
    
    if (child->hasParent)
        TF_FATAL_ERROR("child node has already been assigned a parent");
    
    if (!parent->children)
        parent->children = new vector<_Node*>;

    parent->children->push_back(child);
    child->hasParent = true;
}

void
TfDebug::Helper::Msg(const string& msg)
{
    FILE *outputFile;
    {
        tbb::spin_mutex::scoped_lock lock(*_outputFileMutex);
        outputFile = _GetOutputFile();
    }
    fprintf(outputFile, "%s", msg.c_str());
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

    tbb::spin_mutex::scoped_lock lock(*_outputFileMutex);
    
    if (start) {
        fprintf(_GetOutputFile(), "%*s%s --{\n", 2 * stackDepth, "", str);
        ++stackDepth;
    }
    else {
        --stackDepth;
        fprintf(_GetOutputFile(), "%*s}-- %s\n", 2 * stackDepth, "", str);
    }
}

void
TfDebug::_RegisterDebugSymbol(TfEnum enumVal, _Node* addr, const char* descrip)
{
    string enumName = TfEnum::GetName(enumVal);

    if (!descrip)
        TF_FATAL_ERROR("description argument for '%s' "
                       "is NULL", enumName.c_str());
    else if (descrip[0] == '\0')
        TF_FATAL_ERROR("description argument for '%s' is empty -- "
                       "add description!",  enumName.c_str());

    if (enumName.empty()) {
        TF_FATAL_ERROR("TF_ADD_ENUM_NAME() failed to add a name for "
                       "enum type %s with value %d [%s]",
                       ArchGetDemangled(enumVal.GetType()).c_str(),
                       enumVal.GetValueAsInt(), descrip);
    }

    Tf_DebugSymbolRegistry::_GetInstance()._Add(enumName, addr, descrip);
}

void
TfDebug::_ComplainAboutInvalidSymbol(const char* name)
{
    TF_FATAL_ERROR("TF_DEBUG_ENVIRONMENT_SYMBOL(): symbol '%s' "
                   "invalid.  (Check the TF_DEBUG_RANGE macro().)", name);
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
 * Finally, setting TF_DEBUG to "help" prints a help message, while setting
 * TF_DEBUG to "list" prints a list of all registered debug symbols.  In both
 * case, the program exits immediately after printing.
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
