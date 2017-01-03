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
#include "pxr/base/plug/info.h"
#include "pxr/base/plug/debugCodes.h"
#include "pxr/base/js/json.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/work/threadLimits.h"
#include <tbb/task_arena.h>
#include <tbb/task_group.h>
#include <boost/bind.hpp>
#include <fstream>
#include <regex>
#include <set>

namespace {

typedef boost::function<bool (const std::string&)> AddVisitedPathCallback;
typedef boost::function<void (const Plug_RegistrationMetadata&)> AddPluginCallback;

TF_DEFINE_PRIVATE_TOKENS(_Tokens,
    // Filename tokens
    ((PlugInfoName,         "plugInfo.json"))

    // Top level keys
    ((IncludesKey,          "Includes"))
    ((PluginsKey,           "Plugins"))

    // Plugins keys
    ((TypeKey,              "Type"))
    ((NameKey,              "Name"))
    ((InfoKey,              "Info"))
    ((RootKey,              "Root"))
    ((LibraryPathKey,       "LibraryPath"))
    ((ResourcePathKey,      "ResourcePath"))
    );

struct _ReadContext {
    _ReadContext(Plug_TaskArena& taskArena_,
                 const AddVisitedPathCallback& addVisitedPath_,
                 const AddPluginCallback& addPlugin_) :
        taskArena(taskArena_),
        addVisitedPath(addVisitedPath_),
        addPlugin(addPlugin_)
    {
        // Do nothing
    }

    Plug_TaskArena& taskArena;
    AddVisitedPathCallback addVisitedPath;
    AddPluginCallback addPlugin;
};

// Join dirname(ownerPathname) and subpathname.
std::string
_MergePaths(
    const std::string& ownerPathname,
    const std::string& subpathname,
    bool keepTrailingSlash = false)
{
    // Return absolute or empty path as is.
    if (subpathname.empty() || subpathname[0] == '/') {
        return subpathname;
    }

    // Join dirname(ownerPathname) and subpathname.
    const std::string result =
        TfStringCatPaths(TfGetPathName(ownerPathname), subpathname);

    // Retain trailing slash if request and if any.
    return (keepTrailingSlash && *subpathname.rbegin() == '/')
           ? result + "/"
           : result;
}

// Join rootPathname and subpathname.
std::string
_AppendToRootPath(
    const std::string& rootPathname,
    const std::string& subpathname,
    bool keepTrailingSlash = false)
{
    // Return absolute or empty path as is.
    if (subpathname.empty()) {
        return rootPathname;
    }

    // Return absolute or empty path as is.
    if (subpathname[0] == '/') {
        return subpathname;
    }

    // Join rootPathname and subpathname.
    return TfStringCatPaths(rootPathname, subpathname);
}

void
_AddPlugin(
    _ReadContext* context,
    const std::string& pathname,
    const std::string& key,
    size_t index,
    const JsValue& plugInfo)
{
    const std::string location =
        TfStringPrintf("file %s %s[%zd]", pathname.c_str(), key.c_str(), index);
    const Plug_RegistrationMetadata metadata(plugInfo, pathname, location);

    if (metadata.type != Plug_RegistrationMetadata::UnknownType) {
        // Notify via callback.
        context->taskArena.Run(boost::bind(context->addPlugin, metadata));
    }
}

// Return the plug info in pathname into result.  Returns true if the
// file could be opened.
bool
_ReadPlugInfoObject(const std::string& pathname, JsObject* result)
{
    result->clear();

    // The file may not exist or be readable.
    std::ifstream ifs;
    ifs.open(pathname.c_str());
    if (!ifs.is_open()) {
        TF_DEBUG(PLUG_INFO_SEARCH).
            Msg("Failed to open plugin info %s\n", pathname.c_str());
        return false;
    }

    // The Js library doesn't allow comments, but we'd like to allow them.
    // Strip comments, retaining empty lines so line numbers reported in parse
    // errors match line numbers in the original file content.
    // NOTE: Joining a vector of strings and calling JsParseString()
    //       is *much* faster than writing to a stringstream and
    //       calling JsParseStream() as of this writing.
    std::string line;
    std::vector<std::string> filtered;
    while (getline(ifs, line)) {
        if (line.find('#') < line.find_first_not_of(" \t#"))
            line.clear();
        filtered.push_back(line);
    }

    // Read JSON.
    JsParseError error;
    JsValue plugInfo = JsParseString(TfStringJoin(filtered, "\n"), &error);

    // Validate.
    if (plugInfo.IsNull()) {
        TF_RUNTIME_ERROR("Plugin info file %s couldn't be read "
                         "(line %d, col %d): %s", pathname.c_str(),
                         error.line, error.column, error.reason.c_str());
    }
    else if (!plugInfo.IsObject()) {
        // The contents didn't evaluate to a json object....
        TF_RUNTIME_ERROR("Plugin info file %s did not contain a JSON object",
                         pathname.c_str());
    }
    else {
        *result = plugInfo.GetJsObject();
    }
    return true;
}

void
_ReadPlugInfoWithWildcards(_ReadContext* context, const std::string& pathname);

bool
_ReadPlugInfo(_ReadContext* context, std::string pathname)
{
    // Trivial case.
    if (pathname.empty()) {
        return false;
    }

    // Append the default plug info filename if the path ends in a slash.
    if (*pathname.rbegin() == '/') {
        pathname = pathname + _Tokens->PlugInfoName.GetString();
    }

    // Ignore redundant reads.  This also prevents infinite recursion.
    if (!context->addVisitedPath(pathname)) {
        TF_DEBUG(PLUG_INFO_SEARCH).
            Msg("Ignore already read plugin info %s\n", pathname.c_str());
        return true;
    }

    // Read the file, if possible.
    TF_DEBUG(PLUG_INFO_SEARCH).
        Msg("Will read plugin info %s\n", pathname.c_str());
    JsObject top;
    if (!_ReadPlugInfoObject(pathname, &top)) {
        return false;
    }
    TF_DEBUG(PLUG_INFO_SEARCH).
        Msg("Did read plugin info %s\n", pathname.c_str());

    // Look for our expected keys.
    JsObject::const_iterator i;
    i = top.find(_Tokens->PluginsKey);
    if (i != top.end()) {
        if (!i->second.IsArray()) {
            TF_RUNTIME_ERROR("Plugin info file %s key '%s' "
                             "doesn't hold an array",
                             pathname.c_str(), i->first.c_str());
        }
        else {
            const JsArray& plugins = i->second.GetJsArray();
            for (size_t j = 0, n = plugins.size(); j != n; ++j) {
                _AddPlugin(context, pathname, i->first, j, plugins[j]);
            }
        }
    }
    i = top.find(_Tokens->IncludesKey);
    if (i != top.end()) {
        if (!i->second.IsArray()) {
            TF_RUNTIME_ERROR("Plugin info file %s key '%s' "
                             "doesn't hold an array",
                             pathname.c_str(), i->first.c_str());
        }
        else {
            const JsArray& includes = i->second.GetJsArray();
            for (size_t j = 0, n = includes.size(); j != n; ++j) {
                if (!includes[j].IsString()) {
                    TF_RUNTIME_ERROR("Plugin info file %s key '%s' "
                                     "index %zd doesn't hold a string",
                                     pathname.c_str(), i->first.c_str(), j);
                }
                else {
                    static const bool keepTrailingSlash = true;
                    const std::string newPathname =
                        _MergePaths(pathname, includes[j].GetString(),
                                    keepTrailingSlash);
                    context->taskArena.Run(
                        boost::bind(_ReadPlugInfoWithWildcards,
                                    context, newPathname));
                }
            }
        }
    }

    // Report unexpected keys.
    for (const auto& v : top) {
        const JsObject::key_type& key = v.first;
        if (key != _Tokens->PluginsKey && 
            key != _Tokens->IncludesKey) {
            TF_RUNTIME_ERROR("Plugin info file %s has unknown key %s",
                             pathname.c_str(), key.c_str());
        }
    }

    return true;
}

std::string
_TranslateWildcardToRegex(const std::string& wildcard)
{
    std::string result;
    result.reserve(5 * wildcard.size());    // Worst case growth.
    for (std::string::size_type i = 0, n = wildcard.size(); i != n; ++i) {
        char c = wildcard[i];
        switch (c) {
        case '.':
        case '[':
        case ']':
            // Escaped literal.
            result.push_back('\\');
            result.push_back(c);
            break;

        case '*':
            if (i + 1 != n && wildcard[i + 1] == '*') {
                // ** => match anything
                result.append(".*", 2);

                // Eat next character as well.
                ++i;
            }
            else {
                // * => match anything except /
                result.append("[^/]*", 5);
            }
            break;

        default:
            // Literal.
            result.push_back(c);
        }
    }

    return result;
}

void
_TraverseDirectory(
    _ReadContext* context, 
    const std::string& dirname,
    const std::shared_ptr<std::regex> dirRegex)
{
    std::vector<std::string> dirnames, filenames;
    TfReadDir(dirname, &dirnames, &filenames, &filenames);

    // Traverse all files in the directory to see if we have a
    // match first so that we can terminate the recursive walk
    // if we find one.
    for (const auto& f : filenames) {
        const std::string path = TfStringCatPaths(dirname, f);
        if (std::regex_match(path, *dirRegex)) {
            context->taskArena.Run(boost::bind(_ReadPlugInfo, context, path));
            return;
        }
    }

    for (const auto& d : dirnames) {
        const std::string path = TfStringCatPaths(dirname, d);
        context->taskArena.Run(
            boost::bind(_TraverseDirectory, context, path, dirRegex));
    }
}

void
_ReadPlugInfoWithWildcards(_ReadContext* context, const std::string& pathname)
{
    // For simplicity we check if pathname has any wildcards.  If not
    // we check that path.  If it has * but no ** then we do a glob
    // and read all the matched paths.  If it has ** then we translate
    // to a regex, do a full filesystem walk and filter by the regex.
    // We furthermore artificially terminate the recursion for any
    // directory with a match.  (We don't terminate the walk recursion
    // since we've already done that; we just act as if we did.)

    // Trivial case.
    if (pathname.empty()) {
        return;
    }

    // Fail if pathname is not absolute.
    if (pathname[0] != '/') {
        TF_RUNTIME_ERROR("Plugin info file %s is not absolute",
                         pathname.c_str());
        return;
    }

    // Scan pattern for wildcards.
    std::string::size_type i = pathname.find('*');
    if (i == std::string::npos) {
        // No wildcards so try the full path.
        _ReadPlugInfo(context, pathname);
        return;
    }

    // Can we glob?
    i = pathname.find("**");
    if (i == std::string::npos) {
        TF_DEBUG(PLUG_INFO_SEARCH).
            Msg("Globbing plugin info path %s\n", pathname.c_str());

        // Yes, no recursive searches so do the glob.
        for (const auto& match : TfGlob(pathname, 0)) {
            context->taskArena.Run(boost::bind(_ReadPlugInfo, context, match));
        }
        return;
    }

    // Find longest non-wildcarded prefix directory.
    std::string::size_type j = pathname.rfind('/', i);
    std::string dirname = pathname.substr(0, j);
    std::string pattern = pathname.substr(j + 1);

    // Convert to regex.
    pattern = _TranslateWildcardToRegex(pattern);

    // Append implied filename and build full regex string.
    pattern = TfStringPrintf("%s/%s%s",
                             dirname.c_str(), pattern.c_str(),
                             !pattern.empty() && *pattern.rbegin() == '/'
                             ? _Tokens->PlugInfoName.GetText() : "");

    std::shared_ptr<std::regex> re;
    try {
        re.reset(new std::regex(pattern.c_str(), 
                                std::regex_constants::extended));
    }
    catch (const std::regex_error& e) {
        TF_RUNTIME_ERROR("Failed to compile regex for %s: %s (%s)",
                         pathname.c_str(), pattern.c_str(), e.what());
        return;
    }

    // Walk filesystem.
    TF_DEBUG(PLUG_INFO_SEARCH).
        Msg("Recursively walking plugin info path %s\n", pathname.c_str());
    context->taskArena.Run(boost::bind(_TraverseDirectory, 
                                       context, dirname, re));
}

// Helper for running tasks.
template <class Fn>
struct _Run {
    _Run(tbb::task_group *group, const Fn &fn) : group(group), fn(fn) {}
    void operator()() const { group->run(fn); }
    tbb::task_group *group;
    Fn fn;
};

template <class Fn>
_Run<Fn>
_MakeRun(tbb::task_group *group, const Fn& fn)
{
    return _Run<Fn>(group, fn);
}

// A helper dispatcher object that runs tasks in a tbb::task_group inside a
// tbb::task_arena, to ensure that when we wait, we only wait for our own tasks.
// Otherwise if we run an unrelated task in the thread that holds our lock that
// winds up trying to take the lock we get deadlock.
class _TaskArenaImpl : boost::noncopyable {
public:
    _TaskArenaImpl();
    ~_TaskArenaImpl();

    /// Schedule \p fn to run.
    void Run(const boost::function<void()>& fn);

    /// Wait for all scheduled tasks to complete.
    void Wait();

private:
    tbb::task_arena _arena;
    tbb::task_group _group;
};

_TaskArenaImpl::_TaskArenaImpl() : _arena(WorkGetConcurrencyLimit()) 
{
    // Do nothing
}

_TaskArenaImpl::~_TaskArenaImpl()
{
    Wait();
}

void
_TaskArenaImpl::Run(const boost::function<void()>& fn)
{
    _arena.execute(_MakeRun(&_group, fn));
}

void
_TaskArenaImpl::Wait()
{
    _arena.execute(boost::bind(&tbb::task_group::wait, &_group));
}

} // anonymous namespace

class Plug_TaskArena::_Impl : public _TaskArenaImpl { };

Plug_TaskArena::Plug_TaskArena() : _impl(new _Impl)
{
    // Do nothing
}

Plug_TaskArena::Plug_TaskArena(Synchronous)
{
    // Do nothing
}

Plug_TaskArena::~Plug_TaskArena()
{
    // Do nothing
}

void
Plug_TaskArena::Run(const boost::function<void()>& fn)
{
    if (_impl) {
        _impl->Run(fn);
    }
    else {
        fn();
    }
}

void
Plug_TaskArena::Wait()
{
    if (_impl) {
        _impl->Wait();
    }
}

Plug_RegistrationMetadata::Plug_RegistrationMetadata(
    const JsValue& value,
    const std::string& valuePathname,
    const std::string& locationForErrorReporting) :
    type(UnknownType)
{
    const char* errorMessage = "<internal error>";
    const TfToken* key;
    JsObject::const_iterator i;

    // Validate
    if (!value.IsObject()) {
        TF_RUNTIME_ERROR("Plugin info %s doesn't hold an object; "
                         "plugin ignored",
                         locationForErrorReporting.c_str());
        return;
    }
    const JsObject& topInfo = value.GetJsObject();

    // Parse type.
    key = &_Tokens->TypeKey;
    i = topInfo.find(*key);
    if (i != topInfo.end()) {
        if (!i->second.IsString()) {
            errorMessage = "doesn't hold a string";
            goto error;
        }
        else {
            const std::string& typeName = i->second.GetString();
            if (typeName == "library") {
                type = LibraryType;
            }
            else if (typeName == "python") {
                type = PythonType;
            }
            else if (typeName == "resource") {
                type = ResourceType;
            }
            else {
                errorMessage = "doesn't hold a valid type";
                goto error;
            }
        }
    }
    else {
        errorMessage = "is missing";
        goto error;
    }

    // Parse name.
    key = &_Tokens->NameKey;
    i = topInfo.find(*key);
    if (i != topInfo.end()) {
        if (!i->second.IsString()) {
            errorMessage = "doesn't hold a string";
            goto error;
        }
        else {
            pluginName = i->second.GetString();
            if (pluginName.empty()) {
                errorMessage = "doesn't hold a valid name";
                goto error;
            }
        }
    }
    else {
        errorMessage = "is missing";
        goto error;
    }

    // Parse root.
    key = &_Tokens->RootKey;
    i = topInfo.find(*key);
    if (i != topInfo.end()) {
        if (!i->second.IsString()) {
            errorMessage = "doesn't hold a string";
            goto error;
        }
        else {
            pluginPath = _MergePaths(valuePathname, i->second.GetString());
            if (pluginPath.empty()) {
                errorMessage = "doesn't hold a valid path";
                goto error;
            }
        }
    }
    else {
        pluginPath = TfGetPathName(valuePathname);
    }

    // Parse library path (relative to pluginPath).
    key = &_Tokens->LibraryPathKey;
    i = topInfo.find(*key);
    if (i != topInfo.end()) {
        if (!i->second.IsString()) {
            errorMessage = "doesn't hold a string";
            goto error;
        }
        else {
            libraryPath = _AppendToRootPath(pluginPath, i->second.GetString());
            if (libraryPath.empty()) {
                errorMessage = "doesn't hold a valid path";
                goto error;
            }
        }
    }
    else if (type == LibraryType) {
        errorMessage = "is missing";
        goto error;
    }

    // Parse resource path (relative to pluginPath).
    key = &_Tokens->ResourcePathKey;
    i = topInfo.find(*key);
    if (i != topInfo.end()) {
        if (!i->second.IsString()) {
            errorMessage = "doesn't hold a string";
            goto error;
        }
        else {
            resourcePath = _AppendToRootPath(pluginPath, i->second.GetString());
            if (resourcePath.empty()) {
                errorMessage = "doesn't hold a valid path";
                goto error;
            }
        }
    }
    else {
        resourcePath = TfGetPathName(pluginPath);
    }

    // Parse info.
    key = &_Tokens->InfoKey;
    i = topInfo.find(*key);
    if (i != topInfo.end()) {
        if (!i->second.IsObject()) {
            errorMessage = "doesn't hold an object";
            goto error;
        }
        else {
            plugInfo = i->second.GetJsObject();
        }
    }
    else {
        errorMessage = "is missing";
        goto error;
    }

    // Report unexpected keys.
    for (const auto& v : topInfo) {
        const JsObject::key_type& subkey = v.first;
        if (subkey != _Tokens->TypeKey && 
            subkey != _Tokens->NameKey && 
            subkey != _Tokens->InfoKey && 
            subkey != _Tokens->RootKey && 
            subkey != _Tokens->LibraryPathKey &&
            subkey != _Tokens->ResourcePathKey) {
            TF_RUNTIME_ERROR("Plugin info %s: ignoring unknown key '%s'",
                             locationForErrorReporting.c_str(),
                             subkey.c_str());
        }
    }

    return;

error:
    TF_RUNTIME_ERROR("Plugin info %s key '%s' %s; plugin ignored",
                     locationForErrorReporting.c_str(),
                     key->GetText(), errorMessage);
    type = UnknownType;
}

void
Plug_ReadPlugInfo(
    const std::vector<std::string>& pathnames,
    const AddVisitedPathCallback& addVisitedPath,
    const AddPluginCallback& addPlugin,
    Plug_TaskArena* taskArena)
{
    TF_DEBUG(PLUG_INFO_SEARCH).Msg("Will check plugin info paths\n");
    _ReadContext context(*taskArena, addVisitedPath, addPlugin);
    for (const auto& pathname : pathnames) {
        // For convenience we allow given paths that are directories but
        // don't end in "/" to be handled as directories.  Includes in
        // plugInfo files must still explicitly append '/' to be handled
        // as directories.
        if (!pathname.empty() && *pathname.rbegin() != '/') {
            context.taskArena.Run(
                boost::bind(_ReadPlugInfoWithWildcards, &context,
                            pathname + "/"));
        }
        else {
            context.taskArena.Run(
                boost::bind(_ReadPlugInfoWithWildcards, &context, pathname));
        }
    }
    context.taskArena.Wait();
    TF_DEBUG(PLUG_INFO_SEARCH).Msg("Did check plugin info paths\n");
}
