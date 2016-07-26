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
#include "pxr/usd/pcp/propertyIndex.h"

#include "pxr/usd/pcp/cache.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/node_Iterator.h"
#include "pxr/usd/pcp/pathTranslation.h"
#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/pcp/site.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/tracelite/trace.h"
#include "pxr/base/tf/token.h"

#include <boost/optional.hpp>

////////////////////////////////////////////////////////////

PcpPropertyIndex::PcpPropertyIndex()
{
}

bool 
PcpPropertyIndex::IsEmpty() const
{
    return _propertyStack.empty();
}

PcpPropertyIndex::PcpPropertyIndex(const PcpPropertyIndex &rhs)
{
    _propertyStack = rhs._propertyStack;
    if (rhs._localErrors) {
        _localErrors.reset(new PcpErrorVector(*rhs._localErrors.get()));
    } else {
        _localErrors.reset();
    }
}

void 
PcpPropertyIndex::Swap(PcpPropertyIndex& index)
{
    _propertyStack.swap(index._propertyStack);
}

PcpPropertyRange 
PcpPropertyIndex::GetPropertyRange(bool localOnly) const
{
    if (localOnly) {
        size_t startIdx = 0;
        for (; startIdx < _propertyStack.size(); ++startIdx) {
            if (_propertyStack[startIdx].originatingNode.IsDirect())
                break;
        }

        size_t endIdx = startIdx;
        for (; endIdx < _propertyStack.size(); ++endIdx) {
            if (not _propertyStack[endIdx].originatingNode.IsDirect())
                break;
        }

        const bool foundLocalSpecs = (startIdx != endIdx);

        return PcpPropertyRange(
            PcpPropertyIterator(*this, foundLocalSpecs ? startIdx : 0),
            PcpPropertyIterator(*this, foundLocalSpecs ? endIdx : 0));
    }
    else {
        return PcpPropertyRange(
            PcpPropertyIterator(*this, 0),
            PcpPropertyIterator(*this, _propertyStack.size()));
    }
}

size_t 
PcpPropertyIndex::GetNumLocalSpecs() const
{
    size_t numLocalSpecs = 0;
    for (size_t i = 0; i < _propertyStack.size(); ++i) {
        if (_propertyStack[i].originatingNode.IsDirect()) {
            ++numLocalSpecs;
        }
    }

    return numLocalSpecs;
}

////////////////////////////////////////////////////////////

struct Pcp_Permissions
{
    Pcp_Permissions()
        : previous(SdfPermissionPublic)
        , current(SdfPermissionPublic) { }

    SdfPermission previous;
    SdfPermission current;
};

class Pcp_PropertyIndexer
{
public:
    Pcp_PropertyIndexer(PcpPropertyIndex *propIndex,
                        PcpSite propSite,
                        PcpErrorVector *allErrors)
        : _propIndex(propIndex),
          _propSite(propSite),
          _allErrors(allErrors),
          _var(SdfVariabilityVarying),
          _propType(SdfSpecTypeUnknown)
    {
    }

    void GatherPropertySpecs(const PcpPrimIndex& primIndex, bool usd);
    void GatherRelationalAttributeSpecs( const PcpPropertyIndex& relIndex,
                                         bool usd);

private:
    // Returns the property spec with the given name if it is consistent with
    // previously seen specs, otherwise returns NULL.
    // 
    SdfPropertySpecHandle _GetPrimProperty(
        const SdfLayerRefPtr &layer,
        const SdfPath &owningPrimPath,
        const TfToken &name)
    {
        const SdfPath propPath = owningPrimPath.AppendProperty(name);
        SdfPropertySpecHandle propSpec = layer->GetPropertyAtPath(propPath);
        if (not propSpec)
            return TfNullPtr;

        // See if it's an attribute.
        const SdfSpecType propType = propSpec->GetSpecType();
        if (_propType == SdfSpecTypeUnknown) {
            // First one, just record the property type and layer
            _firstSpec = propSpec;
            _propType  = propType;

        } else if (_propType != propType) {
            // This property spec is inconsistent with the type of the
            // specs previously seen.
            PcpErrorInconsistentPropertyTypePtr e =
                PcpErrorInconsistentPropertyType::New();
            e->rootSite = _propSite;
            e->definingLayerIdentifier = 
                _firstSpec->GetLayer()->GetIdentifier();
            e->definingSpecPath = _firstSpec->GetPath();
            e->definingSpecType = _propType;
            e->conflictingLayerIdentifier = 
                propSpec->GetLayer()->GetIdentifier();
            e->conflictingSpecPath = propSpec->GetPath();
            e->conflictingSpecType = propType;
            _RecordError(e);
            return TfNullPtr;
        }

        // For an attribute, check that its type and variability are consistent.
        if (propType == SdfSpecTypeAttribute and
            not _IsConsistentAttribute(propSpec)) {
            return TfNullPtr;
        }
        return propSpec;
    }

    // Returns the attribute spec with the given name if it is consistent with
    // previously seen specs, otherwise returns NULL.
    // 
    SdfPropertySpecHandle _GetRelationalAttribute(
        const SdfLayerHandle& layer,
        const SdfPath& relAttrPath)
    {
        SdfPropertySpecHandle attr = layer->GetAttributeAtPath(relAttrPath);

        if (not attr)
            return TfNullPtr;

        if (not _firstSpec) {
            _firstSpec = attr;
        }

        // Check that the type and variability are consistent.
        if (not _IsConsistentAttribute(attr)) {
            return TfNullPtr;
        }
        return attr;
    }

    bool _IsConsistentAttribute(const SdfPropertySpecHandle &attr)
    {
        TfToken valueType;
        SdfVariability var = SdfVariability();

        // This function is performance sensitive, so as an optimization, get
        // the underlying spec pointer to avoid excessive dormancy checks (one
        // per dereference).
        if (SdfSpec *specPtr = boost::get_pointer(attr)) {
            SdfLayer *layer = boost::get_pointer(specPtr->GetLayer());
            SdfPath const &path = specPtr->GetPath();
            valueType = layer->GetFieldAs<TfToken>(path,
                                                   SdfFieldKeys->TypeName);
            var = layer->GetFieldAs<SdfVariability>(
                path, SdfFieldKeys->Variability);
        }

        if (_valueType.IsEmpty()) {
            // First one, just record the type and variability.
            _valueType  = valueType;
            _var        = var;
            return true;
        }

        if (_valueType != valueType) {
            PcpErrorInconsistentAttributeTypePtr e =
                PcpErrorInconsistentAttributeType::New();
            e->rootSite = _propSite;
            e->definingLayerIdentifier = 
                _firstSpec->GetLayer()->GetIdentifier();
            e->definingSpecPath = _firstSpec->GetPath();
            e->definingValueType = _valueType;
            e->conflictingLayerIdentifier = attr->GetLayer()->GetIdentifier();
            e->conflictingSpecPath = attr->GetPath();
            e->conflictingValueType = valueType;
            _RecordError(e);
            return false;
        }

        if (_var != var) {
            PcpErrorInconsistentAttributeVariabilityPtr e =
                PcpErrorInconsistentAttributeVariability::New();
            e->rootSite = _propSite;
            e->definingLayerIdentifier = 
                _firstSpec->GetLayer()->GetIdentifier();
            e->definingSpecPath = _firstSpec->GetPath();
            e->definingVariability = _var;
            e->conflictingLayerIdentifier = attr->GetLayer()->GetIdentifier();
            e->conflictingSpecPath = attr->GetPath();
            e->conflictingVariability = var;
            _RecordError(e);
            // Not returning false here.  We will conform, not ignore.
        }

        return true;
    }

    // Convenience function to record an error both in this property
    // index's local errors vector and the allErrors vector.
    void _RecordError(const PcpErrorBasePtr &err) {
        _allErrors->push_back(err);
        if (not _propIndex->_localErrors) {
            _propIndex->_localErrors.reset(new PcpErrorVector);
        }
        _propIndex->_localErrors->push_back(err);
    }

    void _AddPropertySpecIfPermitted(
        const SdfPropertySpecHandle& propSpec,
        const PcpNodeRef& node,
        Pcp_Permissions* permissions,
        std::vector<Pcp_PropertyInfo>* propertyInfo);

private: // data

    PcpPropertyIndex *_propIndex;
    const PcpSite _propSite;
    PcpErrorVector *_allErrors;
    SdfPropertySpecHandle _firstSpec;
    TfToken _valueType;
    SdfVariability _var;
    SdfSpecType _propType;
};

void
Pcp_PropertyIndexer::_AddPropertySpecIfPermitted(
    const SdfPropertySpecHandle& propSpec,
    const PcpNodeRef& node,
    Pcp_Permissions* permissions,
    std::vector<Pcp_PropertyInfo>* propertyInfo)
{
    if (permissions->previous == SdfPermissionPublic) {
        // We're allowed to add this property.
        propertyInfo->push_back(Pcp_PropertyInfo(propSpec, node));
        // Accumulate permission.
        permissions->current = propSpec->GetFieldAs(SdfFieldKeys->Permission,
                                                    permissions->current);
    } else {
        // The previous node's property permission was private, and this
        // node also has an opinion about it. This is illegal.
        PcpErrorPropertyPermissionDeniedPtr err =
            PcpErrorPropertyPermissionDenied::New();
        err->rootSite = _propSite;
        err->propPath = propSpec->GetPath();
        err->propType = propSpec->GetSpecType();
        err->layerPath = propSpec->GetLayer()->GetIdentifier();
        _RecordError(err);
    }
}

void
Pcp_PropertyIndexer::GatherPropertySpecs(const PcpPrimIndex& primIndex,
                                         bool usd)
{
    const TfToken &name = _propSite.path.GetNameToken();

    // Add properties in reverse strength order (weak-to-strong).
    std::vector<Pcp_PropertyInfo> propertyInfo;

    if (not usd) {
        // We start with the permission from the last node we visited (or 
        // SdfPermissionPublic, if this is the first node). If the strongest
        // opinion about the property's permission from this node is private,
        // we are not allowed to add opinions from subsequent nodes.
        PcpNodeRef prevNode;
        Pcp_Permissions permissions;
        TF_REVERSE_FOR_ALL(i, primIndex.GetPrimRange()) {
            // Track & enforce permissions as we cross node boundaries.
            PcpNodeRef curNode = i.base().GetNode();
            if (curNode != prevNode) {
                permissions.previous = permissions.current;
                prevNode = curNode;
            }

            const Pcp_SdSiteRef primSite = i.base()._GetSiteRef();
            if (SdfPropertySpecHandle propSpec = 
                _GetPrimProperty(primSite.layer, primSite.path, name)) {
                _AddPropertySpecIfPermitted(
                    propSpec, curNode, &permissions, &propertyInfo);
            }
        }
    } else {
        // In USD mode, the prim index will not contain a prim
        // stack, so we need to do a more expensive traversal to
        // populate the property index.
        TF_REVERSE_FOR_ALL(i, primIndex.GetNodeRange()) {
            PcpNodeRef curNode = *i;
            if (not curNode.CanContributeSpecs()) {
                continue;
            }
            const PcpLayerStackPtr& nodeLayerStack = curNode.GetLayerStack();
            const SdfPath& nodePath = curNode.GetPath();
            TF_REVERSE_FOR_ALL(j, nodeLayerStack->GetLayers()) {
                if (SdfPropertySpecHandle propSpec = 
                    _GetPrimProperty(*j, nodePath, name)) {
                    propertyInfo.push_back(
                        Pcp_PropertyInfo(propSpec, curNode));
                }
            }
        }
    }

    // At this point, the specs have been accumulated in reverse order, 
    // because we needed to do a weak-to-strong traversal for permissions. 
    // Here, we reverse the results to give us the correct order.
    std::reverse(propertyInfo.begin(), propertyInfo.end());

    _propIndex->_propertyStack.swap(propertyInfo);
}

void
Pcp_PropertyIndexer::GatherRelationalAttributeSpecs(
    const PcpPropertyIndex& relIndex, bool usd)
{
    const SdfPath& relAttrPath = _propSite.path;
    TF_VERIFY(relAttrPath.IsRelationalAttributePath());

    // Add relational attributes in reverse strength order (weak-to-strong).
    std::vector<Pcp_PropertyInfo> propertyInfo;

    // We start with the permission from the last node we visited (or 
    // SdfPermissionPublic, if this is the first node). If the strongest
    // opinion about the property's permission from this node is private,
    // we are not allowed to add opinions from subsequent nodes.
    Pcp_Permissions permissions;

    const PcpPropertyRange propRange = relIndex.GetPropertyRange();
    PcpPropertyReverseIterator relIt(propRange.second);
    const PcpPropertyReverseIterator relItEnd(propRange.first);

    while (relIt != relItEnd) {
        const PcpNodeRef curNode = relIt.GetNode();

        const SdfPath relAttrPathInNodeNS = 
            PcpTranslatePathFromRootToNode(curNode, relAttrPath);

        for (; relIt != relItEnd and relIt.GetNode() == curNode; ++relIt) {
            if (relAttrPathInNodeNS.IsEmpty())
                continue;

            const SdfPropertySpecHandle relSpec = *relIt;
            const SdfPropertySpecHandle relAttrSpec = 
                _GetRelationalAttribute(relSpec->GetLayer(),
                                       relAttrPathInNodeNS);
            if (not relAttrSpec)
                continue;

            if (usd) {
                // USD does not enforce permissions.
                propertyInfo.push_back(Pcp_PropertyInfo(relAttrSpec, curNode));
            } else {
                _AddPropertySpecIfPermitted(
                    relAttrSpec, curNode, &permissions, &propertyInfo);
            }
        }

        // Transfer this node's attribute permission.
        permissions.previous = permissions.current;
    }

    // At this point, the specs have been accumulated in reverse order, 
    // because we needed to do a weak-to-strong traversal for permissions. 
    // Here, we reverse the results to give us the correct order.
    std::reverse(propertyInfo.begin(), propertyInfo.end());

    _propIndex->_propertyStack.swap(propertyInfo);
}

void PcpBuildPropertyIndex( const SdfPath &propertyPath,
                            PcpCache *cache,
                            PcpPropertyIndex *propertyIndex,
                            PcpErrorVector *allErrors )
{
    // Verify that the given path is for a property.
    if (not TF_VERIFY(propertyPath.IsPropertyPath())) {
        return;
    }
    if (not propertyIndex->IsEmpty()) {
        TF_CODING_ERROR("Cannot build property index for %s with a non-empty "
                        "property stack.", propertyPath.GetText());
        return;
    }
    
    SdfPath parentPath = propertyPath.GetParentPath();
    if (parentPath.IsTargetPath()) {
        // Immediate parent is a target path, so this is a relational attribute.
        // Step up one more level to the parent relationship itself.
        parentPath = parentPath.GetParentPath();
    }

    if (parentPath.IsPrimPath()) {
        PcpBuildPrimPropertyIndex(
            propertyPath,
            *cache,
            cache->ComputePrimIndex(parentPath, allErrors),
            propertyIndex,
            allErrors);
    }
    else if (parentPath.IsPrimPropertyPath()) {
        const PcpSite propSite(cache->GetLayerStackIdentifier(), propertyPath);
        Pcp_PropertyIndexer indexer(propertyIndex, propSite, allErrors);
        // In USD mode, the PcpCache will not supply any property indexes,
        // so we need to specifically compute one ourselves and use that.
        //
        // XXX: Do we need to support relational attributes in USD? Even
        //      if the USD schema doesn't contain relational attributes,
        //      should Pcp handle this for completeness?
        if (cache->IsUsd()) {
            PcpPropertyIndex relIndex;
            PcpBuildPropertyIndex(parentPath, cache, &relIndex, allErrors);
            indexer.GatherRelationalAttributeSpecs(relIndex, true);
        }
        else {
            const PcpPropertyIndex& relIndex = 
                cache->ComputePropertyIndex(parentPath, allErrors);
            indexer.GatherRelationalAttributeSpecs(relIndex, false);
        }
    }
    else {
        // CODE_COVERAGE_OFF
        // This should not happen.  Owner is not a prim or a 
        // relationship.
        TF_CODING_ERROR(
            "Error, the property <%s> is owned by something "
            "that is not a prim or a relationship.", 
            propertyPath.GetText());
        // CODE_COVERAGE_ON
    }
}

void
PcpBuildPrimPropertyIndex( const SdfPath& propertyPath,
                           const PcpCache& cache,
                           const PcpPrimIndex& primIndex,
                           PcpPropertyIndex *propertyIndex,
                           PcpErrorVector *allErrors )
{
    const PcpSite propSite(cache.GetLayerStackIdentifier(), propertyPath);
    Pcp_PropertyIndexer indexer(propertyIndex, propSite, allErrors);
    indexer.GatherPropertySpecs(primIndex, cache.IsUsd());
}
