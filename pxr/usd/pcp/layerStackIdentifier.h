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
#include "pxr/usd/pcp/expressionVariablesSource.h"

#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/ar/resolverContext.h"

#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfLayer);

/// \class PcpLayerStackIdentifier
///
/// Arguments used to identify a layer stack.
///
/// Objects of this type are immutable.
///
class PcpLayerStackIdentifier {
public:
    typedef PcpLayerStackIdentifier This;

    /// Construct with all empty pointers.
    PCP_API
    PcpLayerStackIdentifier();

    /// Construct with given pointers.  If all arguments are \c TfNullPtr
    /// then the result is identical to the default constructed object.
    PCP_API
    PcpLayerStackIdentifier(
        const SdfLayerHandle& rootLayer,
        const SdfLayerHandle& sessionLayer = TfNullPtr,
        const ArResolverContext& pathResolverContext = ArResolverContext(),
        const PcpExpressionVariablesSource& expressionVariablesOverrideSource = 
            PcpExpressionVariablesSource());

    // XXX: Allow assignment because there are clients using this
    //      as a member that themselves want to be assignable.
    PCP_API
    PcpLayerStackIdentifier& operator=(const PcpLayerStackIdentifier&);

    // Validity.
    PCP_API
    explicit operator bool() const;

    // Comparison.
    PCP_API
    bool operator==(const This &rhs) const;
    bool operator!=(const This &rhs) const
    {
        return !(rhs == *this);
    }

    PCP_API
    bool operator<(const This &rhs) const;
    bool operator<=(const This& rhs) const
    {
        return !(rhs < *this);
    }
    bool operator>(const This& rhs) const
    {
        return rhs < *this;
    }
    bool operator>=(const This& rhs) const
    {
        return !(*this < rhs);
    }

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

    /// The source for expression variables that compose over the expression 
    /// variables in this layer stack. (optional)
    const PcpExpressionVariablesSource expressionVariablesOverrideSource;

private:
    size_t _ComputeHash() const;

private:
    const size_t _hash;
};

template <typename HashState>
inline void
TfHashAppend(HashState& h, const PcpLayerStackIdentifier& x)
{
    h.Append(x.GetHash());
}

inline
size_t
hash_value(const PcpLayerStackIdentifier& x)
{
    return TfHash{}(x);
}

PCP_API
std::ostream& operator<<(std::ostream&, const PcpLayerStackIdentifier&);

/// Manipulator to cause the next PcpLayerStackIdentifier
/// written to the ostream to write the base name of
/// its layers, rather than the full identifier.
PCP_API
std::ostream& PcpIdentifierFormatBaseName(std::ostream&);

/// Manipulator to cause the next PcpLayerStackIdentifier
/// written to the ostream to write the real path of
/// its layers, rather than the identifier.
PCP_API
std::ostream& PcpIdentifierFormatRealPath(std::ostream&);

/// Manipulator to cause the next PcpLayerStackIdentifier
/// written to the ostream to write the identifier of
/// its layers.  This is the default state; this manipulator is only to nullify
/// one of the above manipulators.
PCP_API
std::ostream& PcpIdentifierFormatIdentifier(std::ostream&);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_LAYER_STACK_IDENTIFIER_H
