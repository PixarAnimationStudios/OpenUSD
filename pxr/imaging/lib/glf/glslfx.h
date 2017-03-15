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
#ifndef GLF_GLSLFX_H
#define GLF_GLSLFX_H

/// \file glf/glslfx.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/imaging/glf/glslfxConfig.h"

#include "pxr/base/tf/token.h"

#include <boost/scoped_ptr.hpp>

#include <string>
#include <vector>
#include <set>
#include <map>

PXR_NAMESPACE_OPEN_SCOPE


/// \class GlfGLSLFX
///
/// A class representing the config and shader source of a glslfx file.
///
/// a GlfGLSLFX object is constructed by providing the path of a file whose
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
class GlfGLSLFX
{
public:
    /// Create an invalid glslfx object
    GLF_API
    GlfGLSLFX();

    /// Create a glslfx object from a file
    GLF_API
    GlfGLSLFX(std::string const & filePath);

    /// Create a glslfx object from a stream
    GLF_API
    GlfGLSLFX(std::istream &is);

    /// Return the parameters specified in the configuration
    GLF_API
    GlfGLSLFXConfig::Parameters GetParameters() const;

    /// Return the textures specified in the configuration
    GLF_API
    GlfGLSLFXConfig::Textures GetTextures() const;

    /// Return the attributes specified in the configuration
    GLF_API
    GlfGLSLFXConfig::Attributes GetAttributes() const;

    /// Returns true if this is a valid glslfx file
    GLF_API
    bool IsValid(std::string *reason=NULL) const;

    /// \name Compatible shader sources
    /// @{

    /// Get the vertex source string
    GLF_API
    std::string GetVertexSource() const;

    /// Get the tess control source string
    GLF_API
    std::string GetTessControlSource() const;

    /// Get the tess eval source string
    GLF_API
    std::string GetTessEvalSource() const;

    /// Get the geometry source string
    GLF_API
    std::string GetGeometrySource() const;

    /// Get the fragment source string
    GLF_API
    std::string GetFragmentSource() const;

    /// @}

    /// \name OpenSubdiv composable shader sources
    /// @{

    /// Get the preamble (osd uniform definitions)
    GLF_API
    std::string GetPreambleSource() const;

    /// Get the surface source string
    GLF_API
    std::string GetSurfaceSource() const;

    /// Get the displacement source string
    GLF_API
    std::string GetDisplacementSource() const;

    /// Get the vertex injection source string
    GLF_API
    std::string GetVertexInjectionSource() const;

    /// Get the geometry injection source string
    GLF_API
    std::string GetGeometryInjectionSource() const;

    /// @}

    /// Get the shader source associated with given key
    GLF_API
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
    bool _ParseVersionLine(std::vector<std::string> const &tokens,
                           _ParseContext & context);
    bool _ParseConfigurationLine(_ParseContext & context);
    bool _ComposeConfiguration(std::string *reason);
    std::string _GetSource(const TfToken &shaderStageKey) const;

private:
    _ParseContext _globalContext;

    std::set<std::string> _importedFiles;

    typedef std::map<std::string, std::string> _SourceMap;

    _SourceMap _sourceMap;
    _SourceMap _configMap;
    std::vector<std::string> _configOrder;
    std::set<std::string> _seenFiles;

    boost::scoped_ptr<GlfGLSLFXConfig> _config;

    bool _valid;
    std::string _invalidReason; // if _valid is false, reason why
    size_t _hash;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif

