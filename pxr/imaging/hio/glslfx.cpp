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
#include "pxr/imaging/hio/glslfx.h"
#include "pxr/imaging/hio/glslfxConfig.h"
#include "pxr/imaging/hio/debugCodes.h"

#include "pxr/usd/ar/resolver.h"

#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/pathUtils.h"

#include <boost/functional/hash.hpp>

#include <iostream>
#include <istream>
#include <fstream>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HioGlslfxTokens, HIO_GLSLFX_TOKENS);

#define CURRENT_VERSION 0.1

using std::string;
using std::vector;
using std::list;
using std::istringstream;
using std::istream;
using std::ifstream;

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sectionDelimiter, "--"))
    ((commentDelimiter, "---"))
    (version)
    (configuration)
    (glsl)
    ((import, "#import"))
    ((shaderResources, "ShaderResources"))
    ((toolSubst, "$TOOLS"))
);

using namespace std;

namespace {

// This is a private registry of paths to shader resources installed
// within package bundles. Packages which install glslfx shader source
// files must register the resource subdir where these files will be
// installed within the package bundle using the "ShaderResources"
// metadata key.
class ShaderResourceRegistry {
public:
    ShaderResourceRegistry();

    std::string GetShaderResourcePath(
        std::string const & packageName,
        std::string const & shaderAssetPath) const;

private:
    std::unordered_map< std::string, std::string > _resourceMap;
};

ShaderResourceRegistry::ShaderResourceRegistry()
{
    PlugRegistry& plugReg = PlugRegistry::GetInstance();
    PlugPluginPtrVector plugins = plugReg.GetAllPlugins();

    for (PlugPluginPtr const& plugin : plugins) {
        std::string packageName = plugin->GetName();
        JsObject metadata = plugin->GetMetadata();

        JsValue value;
        if (TfMapLookup(metadata, _tokens->shaderResources, &value)
               && value.Is<std::string>()) {

            string shaderPath =
                TfStringCatPaths(plugin->GetResourcePath(),
                                 value.Get<std::string>());
            _resourceMap[packageName] = shaderPath;
        }
    }
}

std::string
ShaderResourceRegistry::GetShaderResourcePath(
        std::string const & packageName,
        std::string const & shaderAssetPath) const
{
    string resourcePath;
    if (!TfMapLookup(_resourceMap, packageName, &resourcePath)) {
        return std::string();
    }

    return TfStringCatPaths(resourcePath, shaderAssetPath);
}

static TfStaticData<const ShaderResourceRegistry> _shaderResourceRegistry;

};

static string
_ResolveResourcePath(const string& importFile, string *errorStr)
{
    const vector<string> pathTokens = TfStringTokenize(importFile, "/");
    if (pathTokens.size() < 3) {
        if( errorStr )
        {
            *errorStr = TfStringPrintf( "Expected line of the form "
                            "%s/<packageName>/path",
                            _tokens->toolSubst.GetText() );
        }
        return "";
    }

    const string packageName = pathTokens[1];

    const string assetPath = TfStringJoin(
        vector<string>(pathTokens.begin() + 3, pathTokens.end()), "/");

    const string resourcePath =
        _shaderResourceRegistry->GetShaderResourcePath(packageName,
                                                        assetPath);
    if (resourcePath.empty() && errorStr) {
        *errorStr = TfStringPrintf( "Can't find "
                        "resource dir to resolve tools path "
                        "substitution on %s",
                        packageName.c_str());
    }

    return TfPathExists(resourcePath) ? resourcePath : ""; 
}

static string
_ComputeResolvedPath(
    const string &containingFile,
    const string &filename,
    string *errorStr )
{
    // Resolve $TOOLS-prefixed paths.
    if (TfStringStartsWith(filename, _tokens->toolSubst.GetString() + "/")) {
        return _ResolveResourcePath(filename, errorStr);
    }

    ArResolver& resolver = ArGetResolver();

#if AR_VERSION == 1
    if (filename[0] == '/') {
        return resolver.Resolve(filename);
    }

    // Try to resolve as file-relative. Resolve using the repository form of
    // the anchored file path otherwise the relative file must be local to the
    // containing file. If the anchored path cannot be represented as a
    // repository path, perform a simple existence check.
    const string anchoredPath =
        resolver.AnchorRelativePath(containingFile, filename);
    const string repositoryPath =
        resolver.ComputeRepositoryPath(anchoredPath);
    const string resolvedPath = repositoryPath.empty()
        ? (TfPathExists(anchoredPath) ? anchoredPath : "")
        : resolver.Resolve(repositoryPath);
    if (!resolvedPath.empty() || filename[0] == '.') {
        // If we found the path via file-relative search, return it.
        // Otherwise, if we didn't find the file, and it actually has a
        // file-relative prefix (./, ../), return empty to avoid a fruitless
        // search.
        return resolvedPath;
    }

    // Search for the file along the resolver search path.
    return resolver.Resolve(filename);
#else
    // Create an identifier for the specified .glslfx file by combining
    // the containing file and the new file to accommodate relative paths,
    // then resolve it.
    const std::string assetPath =
        resolver.CreateIdentifier(filename, ArResolvedPath(containingFile));
    return assetPath.empty() ? string() : resolver.Resolve(assetPath);
#endif
}

HioGlslfx::HioGlslfx() :
    _valid(false), _hash(0)
{
    // do nothing
}

HioGlslfx::HioGlslfx(string const & filePath, TfToken const & technique)
    : _technique(technique)
    , _valid(true)
    , _hash(0)
{
    string errorStr;
#if AR_VERSION == 1
    // Resolve with the containingFile set to the current working directory
    // with a trailing slash. This ensures that relative paths supplied to the
    // constructor are properly anchored to the containing file's directory.
    const string resolvedPath =
        _ComputeResolvedPath(ArchGetCwd() + "/", filePath, &errorStr);
#else
    const string resolvedPath =
        _ComputeResolvedPath(string(), filePath, &errorStr);
#endif
    if (resolvedPath.empty()) {
        if (!errorStr.empty()) {
            TF_RUNTIME_ERROR(errorStr);
        } else {
            TF_WARN("File doesn't exist: \"%s\"\n", filePath.c_str());
        }
        _valid = false;
        return;
    }

    _globalContext = _ParseContext(resolvedPath);

    TF_DEBUG(HIO_DEBUG_GLSLFX).Msg("Creating GLSLFX data from %s\n",
                                    filePath.c_str());

    _valid = _ProcessFile(_globalContext.filename, _globalContext);

    if (_valid) {
        _valid = _ComposeConfiguration(&_invalidReason);
    }
}

HioGlslfx::HioGlslfx(istream &is, TfToken const & technique)
    : _globalContext("istream")
    , _technique(technique)
    , _valid(true)
    , _hash(0)
{
    TF_DEBUG(HIO_DEBUG_GLSLFX).Msg("Creating GLSLFX data from istream\n");

    _valid = _ProcessInput(&is, _globalContext);

    if (_valid) {
        _valid = _ComposeConfiguration(&_invalidReason);
    }
}

bool
HioGlslfx::IsValid(std::string *reason) const
{
    if (reason && !_valid) {
        *reason = _invalidReason;
    }
    return _valid;
}

bool
HioGlslfx::_ProcessFile(string const & filePath, _ParseContext & context)
{
    if (_seenFiles.count(filePath)) {
        // for now, just ignore files that have already been included
        TF_DEBUG(HIO_DEBUG_GLSLFX).Msg("Multiple import of %s\n",
                                        filePath.c_str());
        return true;
    }

    _seenFiles.insert(filePath);
    ifstream input(filePath.c_str());
    return _ProcessInput(&input, context);
}

// static
vector<string>
HioGlslfx::ExtractImports(const string& filename)
{
    ifstream input(filename);
    if (!input) {
        return {};
    }

    vector<string> imports;

    string line;
    while (getline(input, line)) {
        if (line.find(_tokens->import) == 0) {
            imports.push_back(TfStringTrim(line.substr(_tokens->import.size())));
        }
    }

    return imports;
}

bool
HioGlslfx::_ProcessInput(std::istream * input,
                         _ParseContext & context)
{
    while (getline(*input, context.currentLine)) {
        // trim to avoid issues with cross-platform line endings
        context.currentLine = TfStringTrimRight(context.currentLine);

        // increment the line number
        ++context.lineNo;

        // update hash
        boost::hash_combine(_hash, context.currentLine);

        if (context.lineNo > 1 && context.version < 0) {
            TF_RUNTIME_ERROR("Syntax Error on line 1 of %s. First line in file "
                             "must be version info.", context.filename.c_str());
            return false;
        }

        // simply ignore comments
        if (context.currentLine.find(
                _tokens->commentDelimiter.GetText()) == 0) {
            continue;
        } else
        // we found a section delimiter
        if (context.currentLine.find(
                _tokens->sectionDelimiter.GetText()) == 0) {
            if (!_ParseSectionLine(context)) {
                return false;
            }

            TF_DEBUG(HIO_DEBUG_GLSLFX).Msg("  %s : %d : %s\n",
                TfGetBaseName(context.filename).c_str(), context.lineNo,
                context.currentLine.c_str());

        } else
        if (context.currentSectionType == HioGlslfxTokens->glslfx && 
                context.currentLine.find(_tokens->import.GetText()) == 0) {
            if (!_ProcessImport(context)) {
                return false;
            }
        } else
        if (context.currentSectionType == _tokens->glsl) {
            // don't do any parsing of these lines. this will be compiled
            // and linked with the glsl compiler later.
            _sourceMap[context.currentSectionId].append(
                context.currentLine + "\n");
        } else
        if (context.currentSectionType == _tokens->configuration) {
            // this is added to the config dictionary which we will compose
            // later
            _configMap[context.filename].append(context.currentLine + "\n");
        }
    }

    // If we never found the glslfx version this isn't a valid glslfx.
    if (context.version < 0) {
        return false;
    }

    for (std::string const& importFile : context.imports) {
        TF_DEBUG(HIO_DEBUG_GLSLFX).Msg(" Importing File : %s\n",
                                        importFile.c_str());

        _ParseContext localContext(importFile);
        if (!_ProcessFile(importFile, localContext)) {
            return false;
        }
    }

    return true;
}

bool
HioGlslfx::_ProcessImport(_ParseContext & context)
{
    const vector<string> tokens = TfStringTokenize(context.currentLine);

    if (tokens.size() != 2) {
        TF_RUNTIME_ERROR("Syntax Error on line %d of %s. #import declaration "
                         "must be followed by a valid file path.",
                         context.lineNo, context.filename.c_str());
        return false;
    }

    string errorStr;
    const string importFile = _ComputeResolvedPath(context.filename, tokens[1],
                                                   &errorStr );

    if (importFile.empty()) {
        if (!errorStr.empty()) {
            TF_RUNTIME_ERROR( "Syntax Error on line %d of %s. %s",
                context.lineNo, context.filename.c_str(), errorStr.c_str() );
            return false;
        }

        TF_WARN("File doesn't exist: \"%s\"\n", tokens[1].c_str());
        return false;
    }

    // stash away imports for later. top down is weakest to strongest
    // and we want the 
    context.imports.push_back(importFile);
    return true;
}

bool
HioGlslfx::_ParseSectionLine(_ParseContext & context)
{

    vector<string> tokens = TfStringTokenize(context.currentLine);
    if (tokens.size() == 1) {
        TF_RUNTIME_ERROR("Syntax Error on line %d of %s. Section delimiter "
                         "must be followed by a valid token.",
                         context.lineNo ,context.filename.c_str());
        return false;
    }

    context.currentSectionType = tokens[1];
    context.currentSectionId.clear();

    if (context.currentSectionType == HioGlslfxTokens->glslfx.GetText()) {
        return _ParseVersionLine(tokens, context);
    }
    if (context.currentSectionType == _tokens->configuration.GetText()) {
        return _ParseConfigurationLine(context);
    } else
    if (context.currentSectionType == _tokens->glsl.GetText()) {
        return _ParseGLSLSectionLine(tokens, context);
    }

    TF_RUNTIME_ERROR("Syntax Error on line %d of %s. Unknown section tag \"%s\"",
                     context.lineNo, context.filename.c_str(), context.currentSectionType.c_str());
    return false;

}

bool
HioGlslfx::_ParseGLSLSectionLine(vector<string> const & tokens,
                                  _ParseContext & context)
{
    if (tokens.size() < 3) {
        TF_RUNTIME_ERROR("Syntax Error on line %d of %s. \"glsl\" tag must"
                         "be followed by a valid identifier.",
                         context.lineNo, context.filename.c_str());
        return false;
    }

    context.currentSectionId = tokens[2];

    // if we already have a section id that is registered in our source map,
    // bail
    _SourceMap::const_iterator cit = _sourceMap.find(context.currentSectionId);
    if (cit != _sourceMap.end()) {
        TF_RUNTIME_ERROR("Syntax Error on line %d of %s. Source for \"%s\" has "
                         "already been defined", context.lineNo,
                         context.filename.c_str(), 
                         context.currentSectionId.c_str());
        return false;
    }

    // Emit a comment for more helpful compile / link diagnostics.
    // note: #line with source file name is not allowed in GLSL.
    _sourceMap[context.currentSectionId].append(
        TfStringPrintf("// line %d \"%s\"\n",
                       context.lineNo, context.filename.c_str()));

    return true;
}

bool
HioGlslfx::_ParseVersionLine(vector<string> const & tokens,
                              _ParseContext & context)
{
    if (context.lineNo != 1) {
        TF_RUNTIME_ERROR("Syntax Error on line %d of %s. Version "
                         "specifier must be on the first line.",
                         context.lineNo, context.filename.c_str());
        return false;
    }

    // verify that the version spec is what we expect
    if (tokens.size() != 4 || tokens[2] != _tokens->version.GetText()) {
        TF_RUNTIME_ERROR("Syntax Error on line %d of %s. Invalid "
                         "version specifier.", context.lineNo, context.filename.c_str());
        return false;
    }

    context.version = TfStringToDouble(tokens[3]);

    // verify this with the global version. for now, mismatch is an error
    if (context.version != _globalContext.version) {
        TF_RUNTIME_ERROR("Version mismatch. %s specifies %2.2f, but %s "
                         "specifies %2.2f", _globalContext.filename.c_str(),
                         _globalContext.version, context.filename.c_str(),
                         context.version);

        return false;
    }

    return true;
}

bool
HioGlslfx::_ParseConfigurationLine(_ParseContext & context)
{
    if (_configMap.find(context.filename) != _configMap.end()) {
        TF_RUNTIME_ERROR("Syntax Error on line %d of %s. "
                         "configuration for this file has already been defined",
                         context.lineNo, context.filename.c_str());
        return false;
    }

    // insert things in the vector in the order of weakest to strongest
    // strongest. this should be the same as our encounter order
    _configOrder.insert(_configOrder.begin(), context.filename);
    _configMap[context.filename] = "";

    return true;
}



bool
HioGlslfx::_ComposeConfiguration(std::string *reason)
{
    // XXX for now, the strongest value just wins. there is no partial
    // composition. so, if you define in an import .glslfx file:
    //
    // { "parameters : { "foo" : 1} }
    //
    // and in your main .glslfx file:
    //
    // { "parameters : { "bar" : 1} }
    //
    // and the import is processed before the configuration section in the main
    // file, you will *NOT* see
    // { "parameters : { "foo" : 1}.
    //                 { "bar" : 1} }
    //
    // but, rather
    // { "parameters : { "bar" : 1} }
    //
    // there is an opportunity to do more powerful dictionary composition here


    for (std::string const& item : _configOrder) {
        TF_AXIOM(_configMap.find(item) != _configMap.end());
        TF_DEBUG(HIO_DEBUG_GLSLFX).Msg("    Parsing config for %s\n",
                                        TfGetBaseName(item).c_str());

        string errorStr;
        _config.reset(HioGlslfxConfig::Read(
            _technique, _configMap[item], item, &errorStr));

        if (!errorStr.empty()) {
            *reason = 
                TfStringPrintf("Error parsing configuration section of %s: %s.",
                               item.c_str(), errorStr.c_str());
            return false;
        }
    }

    return true;
}

HioGlslfxConfig::Parameters
HioGlslfx::GetParameters() const
{
    if (_config) {
        return _config->GetParameters();
    }

    return HioGlslfxConfig::Parameters();
}

HioGlslfxConfig::Textures
HioGlslfx::GetTextures() const
{
    if (_config) {
        return _config->GetTextures();
    }

    return HioGlslfxConfig::Textures();
}

HioGlslfxConfig::Attributes
HioGlslfx::GetAttributes() const
{
    if (_config) {
        return _config->GetAttributes();
    }

    return HioGlslfxConfig::Attributes();
}

HioGlslfxConfig::MetadataDictionary
HioGlslfx::GetMetadata() const
{
    if (_config) {
        return _config->GetMetadata();
    }

    return HioGlslfxConfig::MetadataDictionary();
}

string
HioGlslfx::_GetSource(const TfToken &shaderStageKey) const
{
    if (!_config) {
        return "";
    }

    string errors;
    vector<string> sourceKeys = _config->GetSourceKeys(shaderStageKey);

    string ret;

    for (std::string const& key : sourceKeys) {
        // now look up the keys and concatenate them together..
        _SourceMap::const_iterator cit = _sourceMap.find(key);

        if (cit == _sourceMap.end()) {
            TF_RUNTIME_ERROR("Can't find shader source for <%s> with the key "
                             "<%s>",
                             shaderStageKey.GetText(),
                             key.c_str());
            return string();
        }

        ret += cit->second + "\n";
    }

    return ret;
}

string
HioGlslfx::GetSurfaceSource() const
{
    return _GetSource(HioGlslfxTokens->surfaceShader);
}

string
HioGlslfx::GetDisplacementSource() const
{
    return _GetSource(HioGlslfxTokens->displacementShader);
}

string
HioGlslfx::GetVolumeSource() const
{
    return _GetSource(HioGlslfxTokens->volumeShader);
}

string
HioGlslfx::GetSource(const TfToken &shaderStageKey) const
{
    return _GetSource(shaderStageKey);
}

PXR_NAMESPACE_CLOSE_SCOPE

