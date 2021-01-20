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
#ifndef PXR_USD_PCP_LAYER_STACK_IDENTIFIER_H
#define PXR_USD_PCP_LAYER_STACK_IDENTIFIER_H

/// \file pcp/layerStackIdentifier.h

#include "pxr/pxr.h"
#include "pxr/usd/pcp/api.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/ar/resolverContext.h"

#include <boost/operators.hpp>

#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfLayer);

/// \class PcpLayerStackIdentifier
///
/// Arguments used to identify a layer stack.
///
/// Objects of this type are immutable.
///
class PcpLayerStackIdentifier :
    boost::totally_ordered<PcpLayerStackIdentifier> {
public:
    typedef PcpLayerStackIdentifier This;

    /// Construct with all empty pointers.
    PCP_API
    PcpLayerStackIdentifier();

    /// Construct with given pointers.  If all arguments are \c TfNullPtr
    /// then the result is identical to the default constructed object.
    PCP_API
    PcpLayerStackIdentifier(const SdfLayerHandle& rootLayer_,
                            const SdfLayerHandle& sessionLayer_ = TfNullPtr,
                            const ArResolverContext& pathResolverContext_ =
                                ArResolverContext());

    // XXX: Allow assignment because there are clients using this
    //      as a member that themselves want to be assignable.
    PCP_API
    PcpLayerStackIdentifier& operator=(const PcpLayerStackIdentifier&);

    // Validity.
#if !defined(doxygen)
    typedef const size_t This::*UnspecifiedBoolType;
#endif
    PCP_API
    operator UnspecifiedBoolType() const;

    // Comparison.
    PCP_API
    bool operator==(const This &rhs) const;
    PCP_API
    bool operator<(const This &rhs) const;

    // Hashing.
    struct Hash {
        size_t operator()(const This & x) const
        {
            return x.GetHash();
        }
    };
    size_t GetHash() const
    {
        return _hash;
    }

public:
    /// The root layer.
    const SdfLayerHandle rootLayer;

    /// The session layer (optional).
    const SdfLayerHandle sessionLayer;

    /// The path resolver context used for resolving asset paths. (optional)
    const ArResolverContext pathResolverContext;

private:
    size_t _ComputeHash() const;

private:
    const size_t _hash;
};


/// \class PcpLayerStackIdentifierStr
///
/// A "string"-based version of PcpLayerStackIdentifier.  These are used in
/// places where they should be stable wrt layer lifetimes.  This is done by
/// storing layer identifiers as strings rather than SdfLayerHandles.
///
class PcpLayerStackIdentifierStr :
    boost::totally_ordered<PcpLayerStackIdentifierStr> {
public:
    typedef PcpLayerStackIdentifierStr This;

    /// Construct with given identifier strings and context.  If all arguments
    /// are empty strings and default-constructed ArResolverContext, then the
    /// result is identical to the default constructed object.
    PCP_API
    explicit PcpLayerStackIdentifierStr(
        std::string const &rootLayerId = std::string(),
        std::string const &sessionLayerId = std::string(),
        ArResolverContext const &resolverContext = ArResolverContext());

    /// Allow implicit conversion from PcpLayerStackIdentifier
    PCP_API
    PcpLayerStackIdentifierStr(PcpLayerStackIdentifier const &lsid);

    // Validity.
#if !defined(doxygen)
    typedef const size_t This::*UnspecifiedBoolType;
#endif
    PCP_API
    operator UnspecifiedBoolType() const;

    // Comparison.
    PCP_API
    bool operator==(const This &rhs) const;
    PCP_API
    bool operator<(const This &rhs) const;

    // Hashing.
    struct Hash {
        size_t operator()(const This & x) const
        {
            return x.GetHash();
        }
    };
    size_t GetHash() const
    {
        return _hash;
    }

public:
    /// The root layer.
    std::string rootLayerId;

    /// The session layer (optional).
    std::string sessionLayerId;

    /// The path resolver context used for resolving asset paths. (optional)
    ArResolverContext pathResolverContext;

private:
    size_t _ComputeHash() const;

private:
    size_t _hash;
};


inline
size_t
hash_value(const PcpLayerStackIdentifier& x)
{
    return x.GetHash();
}

inline
size_t
hash_value(const PcpLayerStackIdentifierStr& x)
{
    return x.GetHash();
}

PCP_API
std::ostream& operator<<(std::ostream&, const PcpLayerStackIdentifier&);
PCP_API
std::ostream& operator<<(std::ostream&, const PcpLayerStackIdentifierStr&);

/// Manipulator to cause the next PcpLayerStackIdentifier or
/// PcpLayerStackIdentifierStr written to the ostream to write the base name of
/// its layers, rather than the full identifier.
PCP_API
std::ostream& PcpIdentifierFormatBaseName(std::ostream&);

/// Manipulator to cause the next PcpLayerStackIdentifier or
/// PcpLayerStackIdentifierStr written to the ostream to write the real path of
/// its layers, rather than the identifier.
PCP_API
std::ostream& PcpIdentifierFormatRealPath(std::ostream&);

/// Manipulator to cause the next PcpLayerStackIdentifier or
/// PcpLayerStackIdentifierStr written to the ostream to write the identifier of
/// its layers.  This is the default state; this manipulator is only to nullify
/// one of the above manipulators.
PCP_API
std::ostream& PcpIdentifierFormatIdentifier(std::ostream&);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_LAYER_STACK_IDENTIFIER_H
