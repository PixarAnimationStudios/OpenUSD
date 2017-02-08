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
#ifndef PCP_PROPERTY_INDEX_H
#define PCP_PROPERTY_INDEX_H

#include "pxr/pxr.h"
#include "pxr/usd/pcp/errors.h"
#include "pxr/usd/pcp/iterator.h"
#include "pxr/usd/pcp/node.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/propertySpec.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// Forward declarations:
class PcpCache;

/// \class Pcp_PropertyInfo
///
/// Private helper structure containing information about a property in the 
/// property stack.
///
struct Pcp_PropertyInfo
{
    Pcp_PropertyInfo() { }
    Pcp_PropertyInfo(const SdfPropertySpecHandle& prop, const PcpNodeRef& node) 
        : propertySpec(prop), originatingNode(node) { }

    SdfPropertySpecHandle propertySpec;
    PcpNodeRef originatingNode;
};

/// \class PcpPropertyIndex
///
/// PcpPropertyIndex is an index of all sites in scene description that
/// contribute opinions to a specific property, under composition
/// semantics.
///
class PcpPropertyIndex
{
public:
    /// Construct an empty property index.
    PcpPropertyIndex();

    /// Copy-construct a property index.
    PcpPropertyIndex(const PcpPropertyIndex &rhs);

    /// Swap the contents of this property index with \p index.
    void Swap(PcpPropertyIndex& index);

    /// Returns true if this property index contains no opinions, false
    /// otherwise.
    bool IsEmpty() const;

    /// Returns range of iterators that encompasses properties in this
    /// index's property stack.
    /// 
    /// By default, this returns a range encompassing all properties in the
    /// index. If \p localOnly is specified, the range will only include
    /// properties from local nodes in its owning prim's graph.
    PcpPropertyRange GetPropertyRange(bool localOnly = false) const;

    /// Return the list of errors local to this property.
    PcpErrorVector GetLocalErrors() const {
        return _localErrors ? *_localErrors.get() : PcpErrorVector();
    }

    /// Returns the number of local properties in this prim index.
    size_t GetNumLocalSpecs() const;

private:
    friend class PcpPropertyIterator;
    friend class Pcp_PropertyIndexer;

    // The property stack is a list of Pcp_PropertyInfo objects in
    // strong-to-weak order.
    std::vector<Pcp_PropertyInfo> _propertyStack;

    /// List of errors local to this property, encountered during computation.
    /// NULL if no errors were found (the expected common case).
    boost::scoped_ptr<PcpErrorVector> _localErrors;
};

/// Builds a property index for the property at \p path,
/// internally computing and caching an owning prim index as necessary.
/// \p allErrors will contain any errors encountered.
void
PcpBuildPropertyIndex( const SdfPath& propertyPath, 
                       PcpCache *cache,
                       PcpPropertyIndex *propertyIndex,
                       PcpErrorVector *allErrors );

/// Builds a prim property index for the property at \p propertyPath.
/// \p allErrors will contain any errors encountered.
void
PcpBuildPrimPropertyIndex( const SdfPath& propertyPath,
                           const PcpCache& cache,
                           const PcpPrimIndex& owningPrimIndex,
                           PcpPropertyIndex *propertyIndex,
                           PcpErrorVector *allErrors );

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PCP_PROPERTY_INDEX_H
