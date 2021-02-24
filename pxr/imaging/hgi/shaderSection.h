//
// Copyright 2020 Pixar
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

#ifndef PXR_IMAGING_HGI_SHADERSECTION_H
#define PXR_IMAGING_HGI_SHADERSECTION_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"
#include <memory>
#include <ostream>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

//struct to hold attribute definitions
struct HgiShaderSectionAttribute
{
    std::string identifier;
    std::string index;
};

using HgiShaderSectionAttributeVector =
    std::vector<HgiShaderSectionAttribute>;

/// \class HgiShaderSection
///
/// A base class for a Shader Section.
/// In its simplest form then it is a construct that knows
/// how to declare itself, define and pass as param.
/// Can be subclassed to add more behaviour for complex cases
/// and to hook into the visitor tree.
///
class HgiShaderSection
{
public:
    HGI_API
    virtual ~HgiShaderSection();

    /// Write out the type, shader section does not hold a type
    /// string as how a type is defined is fully controlled
    /// by sub classes and no assumptions are made
    HGI_API
    virtual void WriteType(std::ostream& ss) const;

    /// Writes the unique name of an instance of the section
    HGI_API
    virtual void WriteIdentifier(std::ostream& ss) const;

    /// Writes a decleration statement for a member or in global scope
    HGI_API
    virtual void WriteDeclaration(std::ostream& ss) const;

    /// Writes the section as a parameter to a function
    HGI_API
    virtual void WriteParameter(std::ostream& ss) const;

    HGI_API
    const HgiShaderSectionAttributeVector& GetAttributes() const;

protected:
    HGI_API
    explicit HgiShaderSection(
            const std::string &identifier,
            const HgiShaderSectionAttributeVector& attributes = {},
            const std::string &defaultValue = std::string());

    HGI_API
    const std::string& _GetDefaultValue() const;

private:
    const std::string _identifierVar;
    const HgiShaderSectionAttributeVector _attributes;
    const std::string _defaultValue;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
