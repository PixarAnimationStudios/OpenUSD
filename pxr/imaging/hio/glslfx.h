//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HIO_GLSLFX_H
#define PXR_IMAGING_HIO_GLSLFX_H

/// \file hio/glslfx.h

#include "pxr/pxr.h"
#include "pxr/imaging/hio/api.h"
#include "pxr/imaging/hio/glslfxConfig.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/staticTokens.h"

#include <string>
#include <vector>
#include <set>
#include <map>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

// Version 1 - Added HioGlslfx::ExtractImports
//
#define HIO_GLSLFX_API_VERSION 1

#define HIO_GLSLFX_TOKENS       \
    (glslfx)                    \
                                \
    (fragmentShader)            \
    (geometryShader)            \
    (geometryShaderInjection)   \
    (preamble)                  \
    (tessControlShader)         \
    (tessEvalShader)            \
    (postTessControlShader)     \
    (postTessVertexShader)      \
    (vertexShader)              \
    (vertexShaderInjection)     \
                                \
    (surfaceShader)             \
    (displacementShader)        \
    (volumeShader)              \
    ((defVal, "default"))


TF_DECLARE_PUBLIC_TOKENS(HioGlslfxTokens, HIO_API, HIO_GLSLFX_TOKENS);

/// \class HioGlslfx
///
/// A class representing the config and shader source of a glslfx file.
///
/// a HioGlslfx object is constructed by providing the path of a file whose
/// contents look something like this:
///
/// \code
/// -- glslfx version 0.1
/// 
/// -- configuration
/// 
/// {
///
///     'textures' : {
///         'texture_1':{
///             'documentation' : 'a useful texture.',
///         },
///         'texture_2':{
///             'documentation' : 'another useful texture.',
///         },
///     },
///     'parameters': {
///         'param_1' : {
///             'default' : 1.0,
///             'documentation' : 'the first parameter'
///         },
///         'param_2' : {
///             'default' : [1.0, 1.0, 1.0],
///             'documentation' : 'a vec3f parameter'
///         },
///         'param_3' : {
///             'default' : 2.0
///         },
///         'param_4' : {
///             'default' : True
///         },
///         'param_5' : {
///             'default' : [1.0, 1.0, 1.0],
///             'role' : 'color'
///             'documentation' : 'specifies a color for use in the shader'
///         },
///     },
///     'parameterOrder': ['param_1',
///                        'param_2',
///                        'param_3',
///                        'param_4',
///                        'param_5'],
/// 
///     'techniques': {
///         'default': {
///             'fragmentShader': {
///                 'source': [ 'MyFragment' ]
///             }
///         },
///         'metal': {
///             'fragmentShader': {
///                 'source': [ 'MyFragment.Metal' ]
///             }
///         }
///     }
/// }
/// 
/// -- glsl MyFragment
/// 
/// uniform float param_1;
/// uniform float param_2;
/// uniform float param_3;
/// uniform float param_4;
/// uniform float param_5;
/// 
/// void main()
/// {
///     // ...
///     // glsl code which consumes the various uniforms, and perhaps sets
///     // gl_FragColor = someOutputColor;
///     // ...
/// }
/// \endcode
///
class HioGlslfx
{
public:
    /// Create an invalid glslfx object
    HIO_API
    HioGlslfx();

    /// Create a glslfx object from a file
    HIO_API
    HioGlslfx(
        std::string const & filePath,
        TfToken const & technique = HioGlslfxTokens->defVal);

    /// Create a glslfx object from a stream
    HIO_API
    HioGlslfx(
        std::istream &is,
        TfToken const & technique = HioGlslfxTokens->defVal);

    /// Return the parameters specified in the configuration
    HIO_API
    HioGlslfxConfig::Parameters GetParameters() const;

    /// Return the textures specified in the configuration
    HIO_API
    HioGlslfxConfig::Textures GetTextures() const;

    /// Return the attributes specified in the configuration
    HIO_API
    HioGlslfxConfig::Attributes GetAttributes() const;

    /// Return the metadata specified in the configuration
    HIO_API
    HioGlslfxConfig::MetadataDictionary GetMetadata() const;

    /// Returns true if this is a valid glslfx file
    HIO_API
    bool IsValid(std::string *reason=NULL) const;

    /// \name Access to commonly used shader sources.
    /// @{

    /// Get the surface source string
    HIO_API
    std::string GetSurfaceSource() const;

    /// Get the displacement source string
    HIO_API
    std::string GetDisplacementSource() const;

    /// Get the volume source string
    HIO_API
    std::string GetVolumeSource() const;

    /// @}

    /// Get the layout config as a VtDictionary parsed from the JSON
    /// layout config corresponding to the shader source associated
    /// with the given keys.
    HIO_API
    VtDictionary GetLayoutAsDictionary(const TfTokenVector &shaderStageKeys,
                                       std::string *errorStr) const;

    /// Get the shader source associated with given key
    HIO_API
    std::string GetSource(const TfToken &shaderStageKey) const;

    /// Get the original file name passed to the constructor
    const std::string &GetFilePath() const { return _globalContext.filename; }

    /// Return set of all files processed for this glslfx object.
    /// This includes the original file given to the constructor
    /// as well as any other files that were imported. This set
    /// will only contain files that exist.
    const std::set<std::string>& GetFiles() const { return _seenFiles; }

    /// Return the computed hash value based on the string
    size_t GetHash() const { return _hash; }

    /// Extract imported files from the specified glslfx file. The returned
    /// paths are as-authored, in the order of declaration, with possible
    /// duplicates. This function is not recursive -- it only extracts imports
    /// from the specified \p filename.
    HIO_API
    static std::vector<std::string> ExtractImports(const std::string& filename);

private:
    class _ParseContext {
    public:
        _ParseContext() { }

        _ParseContext(std::string const & filePath) :
            filename(filePath), lineNo(0), version(-1.0) { }

        std::string filename;
        int lineNo;
        double version;
        std::string currentLine;
        std::string currentSectionType;
        std::string currentSectionId;
        std::vector<std::string> imports;
    };

private:
    bool _ProcessFile(std::string const & filePath,
                      _ParseContext & context);
    bool _ProcessInput(std::istream * input,
                       _ParseContext & context);
    bool _ProcessImport(_ParseContext & context);
    bool _ParseSectionLine(_ParseContext & context);
    bool _ParseGLSLSectionLine(std::vector<std::string> const &tokens,
                               _ParseContext & context);
    bool _ParseLayoutSectionLine(std::vector<std::string> const &tokens,
                                 _ParseContext & context);
    bool _ParseVersionLine(std::vector<std::string> const &tokens,
                           _ParseContext & context);
    bool _ParseConfigurationLine(_ParseContext & context);
    bool _ComposeConfiguration(std::string *reason);

    std::string _GetLayout(const TfToken &shaderStageKey) const;
    std::string _GetSource(const TfToken &shaderStageKey) const;

    /// Get the layout config as a string formatted as JSON corresponding
    /// to the shader source associated with the given keys.
    std::string _GetLayoutAsString(const TfTokenVector &shaderStageKeys) const;

private:
    _ParseContext _globalContext;

    typedef std::map<std::string, std::string> _SourceMap;

    _SourceMap _sourceMap;
    _SourceMap _layoutMap;
    _SourceMap _configMap;
    std::vector<std::string> _configOrder;
    std::set<std::string> _seenFiles;

    std::unique_ptr<HioGlslfxConfig> _config;

    TfToken _technique;
    
    bool _valid;
    std::string _invalidReason; // if _valid is false, reason why
    size_t _hash;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif

