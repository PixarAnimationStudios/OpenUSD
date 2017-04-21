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
#ifndef GLF_GLSLFX_CONFIG_H
#define GLF_GLSLFX_CONFIG_H

/// \file glf/glslfxConfig.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/dictionary.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE


/// \class GlfGLSLFXConfig 
///
/// A class representing the configuration of a glslfx file.
///
/// GlfGLSLFXConfig provides an API for querying the configuration of a
/// glslfx file
///
class GlfGLSLFXConfig
{
public:
    /// Enumerates Roles that parameters can have.
    ///
    /// <b>enum Role:</b>
    /// <ul>       
    ///     <li><b><c> RoleNone  = 0</c></b>   None:  the default role
    ///     <li><b><c> RoleColor = 1</c></b>   Color: the role of a color
    /// </ul>       
    ///
    enum Role {
        RoleNone = 0,
        RoleColor = 1,
    };

    /// \class Parameter
    ///
    /// A class representing a parameter.
    ///
    class Parameter {
    public:
        Parameter(std::string const & name,
                  VtValue const & defaultValue,
                  std::string const & docString = "",
                  Role const & role = RoleNone) :
            name(name),
            defaultValue(defaultValue),
            docString(docString),
            role(role) { }

        std::string name;
        VtValue defaultValue;
        std::string docString;
        Role role;
    };

    typedef std::vector<Parameter> Parameters;

    /// \class Texture
    ///
    /// A class representing a texture.
    ///
    class Texture {
    public:
        Texture(std::string const & name,
                VtValue const & defaultValue,
                std::string const & docString = "") :
            name(name),
            defaultValue(defaultValue),
            docString(docString) { }

        std::string name;
        VtValue defaultValue;
        std::string docString;
    };

    typedef std::vector<Texture> Textures;

    /// \class Attribute
    ///
    /// A class representing an attribute.
    ///
    class Attribute {
    public:
        Attribute(std::string const & name,
                  std::string const & docString = "") :
            name(name),
            docString(docString) { }

        std::string name;
        std::string docString;
    };

    typedef std::vector<Attribute> Attributes;

    /// Create a new GlfGLSLFXConfig from an input string
    ///
    /// The \p filename parameter is only used for error reporting.
    ///
    GLF_API
    static GlfGLSLFXConfig * Read(std::string const & input,
                                  std::string const & filename,
                                  std::string *errorStr);

    typedef std::vector<std::string> SourceKeys;

    /// Return the set of source keys for a particular shader stage
    GLF_API
    SourceKeys GetSourceKeys(TfToken const & shaderStageKey) const;

    /// Return the parameters specified in the configuration
    GLF_API
    Parameters GetParameters() const;

    /// Return the textures specified in the configuration
    GLF_API
    Textures GetTextures() const;

    /// Returns the attributes specified in the configuration
    GLF_API
    Attributes GetAttributes() const;

private:
    // private ctor. should only be called by ::Read
    GlfGLSLFXConfig(VtDictionary const & dict, std::string *errorStr);

    void _Init(VtDictionary const & dict, std::string *errorStr);

    Parameters _GetParameters(VtDictionary const & dict,
                              std::string *errorStr) const;
    Textures _GetTextures(VtDictionary const & dict,
                          std::string *errorStr) const;

    Attributes _GetAttributes(VtDictionary const & dict,
                              std::string *errorStr) const;

    typedef std::map<std::string, SourceKeys> _SourceKeyMap;
    _SourceKeyMap _GetSourceKeyMap(VtDictionary const & dict,
                                   std::string *errorStr) const;

    Parameters _params;
    Textures _textures;
    Attributes _attributes;
    _SourceKeyMap _sourceKeyMap;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
