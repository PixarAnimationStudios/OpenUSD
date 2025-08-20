//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_INPUT_SPEC_H
#define PXR_EXEC_VDF_INPUT_SPEC_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"

#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfOutputSpec;

////////////////////////////////////////////////////////////////////////////////
///
/// A VdfInputSpec describes an input connector.  It stores typing 
/// information, access information and the connector's name.
///
class VdfInputSpec
{
public:
    TF_MALLOC_TAG_NEW("Vdf", "new VdfInputSpec");

    /// Access limits the kinds of operations allowed on the connector.
    enum Access {
        READ      = 0x1,
        READWRITE = 0x2
    };

    template <typename T>
    static VdfInputSpec *New(
        const TfToken &inName,
        const TfToken &outName,
        Access access,
        bool prerequisite)
    {
        return new VdfInputSpec(
            TfType::Find<T>(), inName, outName, access, prerequisite);
    }

    static VdfInputSpec *New(
        TfType type,
        const TfToken &inName,
        const TfToken &outName,
        Access access,
        bool prerequisite)
    {
        return new VdfInputSpec(type, inName, outName, access, prerequisite);
    }

    VDF_API
    ~VdfInputSpec();

    /// Returns the type of this spec
    TfType GetType() const { return _type; }

    /// Returns the name of this connector.
    const TfToken &GetName() const { return _name; }

    /// Returns the name of this spec's type.
    VDF_API
    std::string GetTypeName() const;

    /// Returns the access of this connector.
    const Access &GetAccess() const { return _access; }

    /// Returns \c true if this connector spec and \p other have the same 
    /// type and \c false otherwise.
    VDF_API
    bool TypeMatches(const VdfOutputSpec &other) const;

    /// Returns the name of the associated output, if any.  If not set,
    /// returns the empty token.
    ///
    const TfToken &GetAssociatedOutputName() const {
        return _associatedOutputName;
    }

    /// Returns whether or not this connector is a prerequisite connector.
    ///
    /// Prerequisite outputs are the only ones that can be accessed by
    /// VdfNode::GetRequiredReadsIterator(VdfContext).  Once these
    /// have been computed, a node provides dynamic input dependency information
    /// via that method.
    ///
    bool IsPrerequisite() const { return _prerequisite; }

    /// Returns a hash for this instance.
    VDF_API
    size_t GetHash() const;

    /// Returns true, if two input specs are equal.
    ///
    bool operator==(const VdfInputSpec &rhs) const {
        return _type                 == rhs._type                 &&
               _name                 == rhs._name                 &&
               _associatedOutputName == rhs._associatedOutputName &&
               _access               == rhs._access               &&
               _prerequisite         == rhs._prerequisite;
    }
    bool operator!=(const VdfInputSpec &rhs) const {
        return !(*this == rhs);
    }

private:
    VdfInputSpec(
        TfType type,
        const TfToken &inName, 
        const TfToken &outName,
        Access         access,
        bool           prerequisite)
    :   _type(type),
        _name(inName),
        _associatedOutputName(outName),
        _access(access),
        _prerequisite(prerequisite)
    {}

private:
    // The type accepted by the input.
    TfType _type;

    // The name of the connector
    TfToken _name;

    // Access to the connector is limited by this value.
    TfToken _associatedOutputName;

    // Access to the connector is limited by this value.
    Access _access;

    // Whether or not this connector is a prerequisite connector.
    bool _prerequisite;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
