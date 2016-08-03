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
/// \file pcp/dependencies.h

#ifndef PCP_DEPENDENCIES_H
#define PCP_DEPENDENCIES_H

#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/types.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/site.h"
#include <boost/noncopyable.hpp>
#include <iosfwd>
#include <set>

class PcpLifeboat;
class PcpPrimIndexDependencies;

TF_DECLARE_WEAK_PTRS(PcpLayerStack);

/// \class Pcp_Dependencies
/// \brief Track dependencies between Sdf sites and Pcp sites.
///
/// This object keeps track of which Pcp sites depend on which Sdf sites.
/// Clients can provide a collection of Sdf sites and get back a collection
/// of Pcp sites that use those Sdf sites.
///
/// Relocations can introduce dependencies on prims which are not namespace
/// ancestors.  These are known as "spooky" dependencies because of their
/// non-local effect.  For an example, see the TrickySpookyVariantSelection
/// museum case: the relocated anim scopes have a spooky dependency on the
/// rig scope from under which they were relocated.  We track spooky
/// dependencies separately because they do not contribute directly to
/// a computed prim index the way normal dependencies do, so they
/// require special handling.
///
class Pcp_Dependencies : boost::noncopyable {
public:
    /// Construct with no dependencies.
    Pcp_Dependencies();
    Pcp_Dependencies(const Pcp_Dependencies&);

    ~Pcp_Dependencies();

    /// \name Adding
    /// @{

    /// Records dependencies between \p pcpSitePath and the sites \p sdfSites.
    /// For each site in \p sdfSites, the corresponding node in \p nodes 
    /// indicates the node in the prim index for \p pcpSitePath from which 
    /// the spec originated.
    void Add(const SdfPath& pcpSitePath, 
             const SdfSiteVector& sdfSites, const PcpNodeRefVector& nodes);

    /// Records dependencies between \p pcpSitePath and the sites in
    /// \p dependencies.
    void Add(const SdfPath& pcpSitePath,
             const PcpPrimIndexDependencies& dependencies);

    /// Records spooky dependencies between \p pcpSitePath and the
    /// collection of specs in \p spookySpecs. For each spec in \p spookySpecs,
    /// the corresponding node in \p nodes indicates the node in the prim index
    /// for \p pcpSitePath from which the spec originated.
    void AddSpookySitesUsedByPrim(const SdfPath& pcpSitePath,
                                  const SdfSiteVector& spookySites,
                                  const PcpNodeRefVector& spookyNodes);

    /// Records spooky dependencies between \p pcpSitePath and the sites in
    /// \p deps.
    void AddSpookySitesUsedByPrim(const SdfPath& pcpSitePath,
                                  const PcpPrimIndexDependencies& deps);

    /// @}
    /// \name Removing
    /// @{

    /// Remove all recorded dependencies (including spooky dependencies)
    /// for the site at \p pcpSitePath and all descendant sites.  This is
    /// used when the Pcp graph may have changed for \p pcpSitePath.  That
    /// implies that the graph may have changed for any descendant.  Any
    /// layer stacks used by the site are added to \p lifeboat, if not
    /// \c NULL.
    ///
    /// If \p specsAtPathOnly is \c true then this only removes
    /// non-spooky dependencies due to specs and only those at
    /// \p pcpSitePath (not descendants).
    void Remove(const SdfPath& pcpSitePath, PcpLifeboat* lifeboat,
                bool specsAtPathOnly = false);

    /// Remove all dependencies.  Any layer stacks in use by any site are
    /// added to \p lifeboat, if not \c NULL.  This method implicitly calls
    /// \c Flush().
    void RemoveAll(PcpLifeboat* lifeboat);

    /// Releases any layer stacks that no longer have any dependencies.
    void Flush();

    /// @}
    /// \name Queries
    /// @{

    /// Returns the path of every \c PcpSite that uses the spec in \p layer
    /// at \p path.  If \p recursive is \c true then also returns the path
    /// of every \c PcpSite that uses any descendant of \p path.  If
    /// \p primsOnly is \c true then recursion is limited to prim descendants.
    /// If \p spooky is \c true then spooky dependencies will be returned
    /// instead of normal dependencies.
    ///
    /// If \p sourceNodes is not \c NULL then it has elements corresponding
    /// to the elements in the returned vector. If the vector returned from
    /// this function is called pcpSites, then sourceNodes[i] is the node
    /// in the prim index for pcpSites[i] that introduced the dependency
    /// on the spec at \p layer and \p path.
    ///
    /// If \p spooky is \c true then spooky dependencies will be returned
    /// instead of normal dependencies.
    SdfPathVector Get(const SdfLayerHandle& layer,
                      const SdfPath& path,
                      bool recursive = false,
                      bool primsOnly = false,
                      bool spooky = false,
                      PcpNodeRefVector* sourceNodes = NULL) const;

    /// Returns the path of every \c PcpSite that uses the site given by
    /// \p layerStack and \p path, via a dependency specified by
    /// \p dependencyType. \p dependencyType is a bitwise-OR of
    /// \c PcpDependencyType enum values. If \p recursive is \c true,
    /// then we return the path of every \c PcpSite that uses any descendant 
    /// of \p path, via a dependency specified by \p dependencyType.
    /// If \p spooky is \c true then spooky dependencies will be returned
    /// instead of normal dependencies.
    SdfPathVector Get(const PcpLayerStackPtr& layerStack,
                      const SdfPath& path,
                      unsigned int dependencyType,
                      bool recursive = false,
                      bool spooky = false) const;

    /// Returns every layer that the prim at \p path depends on.  
    /// If \p recursive is \c true then also returns every layer that any 
    /// descendant of \p path depends on.
    SdfLayerHandleSet GetLayersUsedByPrim(const SdfPath& path,
                                          bool recursive = false) const;

    /// Returns all layers from all layer stacks with dependencies recorded
    /// against them.
    SdfLayerHandleSet GetUsedLayers() const;

    /// Returns the root layers of all layer stacks with dependencies
    /// recorded against them.
    SdfLayerHandleSet GetUsedRootLayers() const;

    /// Returns true if there are dependencies recorded against the given
    /// layer stack.
    bool UsesLayerStack(const PcpLayerStackPtr& layerStack) const;

    /// @}
    /// \name Debugging
    /// @{

    /// Write the dependencies to \p s.
    void DumpDependencies(std::ostream& s) const;

    /// Returns a string containing the dependencies.
    std::string DumpDependencies() const;

    /// Verify that the internal data structures are consistent.
    void CheckInvariants() const;

    /// @}

private:
    void _Add(
        const SdfPath& pcpSitePath, SdfSite sdSite, const PcpNodeRef& node,
        bool spooky);

private:
    boost::scoped_ptr<struct Pcp_DependenciesData> _data;
};

#endif
