//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

    /// Writes the arraySize to a function
    HGI_API
    virtual void WriteArraySize(std::ostream& ss) const;

    /// Writes the block instance name of an instance of the section
    HGI_API
    virtual void WriteBlockInstanceIdentifier(std::ostream& ss) const;

    /// Returns the identifier of the section
    const std::string& GetIdentifier() const {
        return _identifierVar;
    }

    /// Returns the attributes of the section
    HGI_API
    const HgiShaderSectionAttributeVector& GetAttributes() const;
    
    /// Returns the arraySize of the section
    const std::string& GetArraySize() const {
        return _arraySize;
    }

    /// Returns whether the section has a block instance identifier
    bool HasBlockInstanceIdentifier() const {
        return !_blockInstanceIdentifier.empty();
    }

protected:
    HGI_API
    explicit HgiShaderSection(
            const std::string &identifier,
            const HgiShaderSectionAttributeVector& attributes = {},
            const std::string &defaultValue = std::string(),
            const std::string &arraySize = std::string(),
            const std::string &blockInstanceIdentifier = std::string());

    HGI_API
    const std::string& _GetDefaultValue() const;

private:
    const std::string _identifierVar;
    const HgiShaderSectionAttributeVector _attributes;
    const std::string _defaultValue;
    const std::string _arraySize;
    const std::string _blockInstanceIdentifier;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
